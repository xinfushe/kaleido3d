#include "Kaleido3D.h"
#include "MetalRHI.h"
#include "MetalEnums.h"
#include <Core/LogUtil.h>

NS_K3D_METAL_BEGIN

MTLPrimitiveTopologyClass ConvertPrimTopology(rhi::EPrimitiveType const& type)
{
    switch(type)
    {
        case rhi::EPT_Triangles:
        case rhi::EPT_TriangleStrip:
            return MTLPrimitiveTopologyClassTriangle;
        case rhi::EPT_Points:
            return MTLPrimitiveTopologyClassPoint;
        case rhi::EPT_Lines:
            return MTLPrimitiveTopologyClassLine;
    }
    return MTLPrimitiveTopologyClassUnspecified;
}

PipelineState::PipelineState(id<MTLDevice> pDevice, rhi::PipelineDesc const & desc, rhi::EPipelineType const& type)
: m_DepthStencilDesc(nil)
, m_RenderPipelineDesc(nil)
, m_Type(type)
, m_Device(pDevice)
{
    InitPSO(desc);
    Finalize();
}

PipelineState::~PipelineState()
{
    
}

void PipelineState::InitPSO(const rhi::PipelineDesc &desc)
{
    if(m_Type==rhi::EPSO_Graphics)
    {
        m_RenderPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        // depth stencil setup
        m_DepthStencilDesc = [[MTLDepthStencilDescriptor alloc] init];
        m_DepthStencilDesc.depthCompareFunction = g_ComparisonFunc[desc.DepthStencil.DepthFunc];
        m_DepthStencilDesc.depthWriteEnabled = desc.DepthStencil.DepthEnable;
        
        MTLStencilDescriptor * stencilFront = m_DepthStencilDesc.frontFaceStencil;
        stencilFront.depthStencilPassOperation = g_StencilOp[desc.DepthStencil.FrontFace.StencilPassOp];
        stencilFront.depthFailureOperation = g_StencilOp[desc.DepthStencil.FrontFace.DepthStencilFailOp];
        stencilFront.stencilFailureOperation = g_StencilOp[desc.DepthStencil.FrontFace.StencilFailOp];
        stencilFront.stencilCompareFunction = g_ComparisonFunc[desc.DepthStencil.FrontFace.StencilFunc];
        
        MTLStencilDescriptor * stencilBack = m_DepthStencilDesc.backFaceStencil;
        stencilBack.depthStencilPassOperation = g_StencilOp[desc.DepthStencil.BackFace.StencilPassOp];
        stencilBack.depthFailureOperation = g_StencilOp[desc.DepthStencil.BackFace.DepthStencilFailOp];
        stencilBack.stencilFailureOperation = g_StencilOp[desc.DepthStencil.BackFace.StencilFailOp];
        stencilBack.stencilCompareFunction = g_ComparisonFunc[desc.DepthStencil.BackFace.StencilFunc];
        
        m_DepthBias = desc.Rasterizer.DepthBias;
        m_DepthBiasClamp = desc.Rasterizer.DepthBiasClamp;
        m_CullMode = g_CullMode[desc.Rasterizer.CullMode];
        m_RenderPipelineDesc.inputPrimitiveTopology = ConvertPrimTopology(desc.PrimitiveTopology);
        m_PrimitiveType = g_PrimitiveType[desc.PrimitiveTopology];
        // blending setup
        MTLRenderPipelineColorAttachmentDescriptor *renderbufferAttachment = m_RenderPipelineDesc.colorAttachments[0];
        
        renderbufferAttachment.pixelFormat = MTLPixelFormatBGRA8Unorm;
        
        renderbufferAttachment.blendingEnabled = desc.Blend.Enable?YES:NO;
        renderbufferAttachment.rgbBlendOperation = g_BlendOperation[desc.Blend.Op];
        renderbufferAttachment.alphaBlendOperation = g_BlendOperation[desc.Blend.BlendAlphaOp];
        renderbufferAttachment.sourceRGBBlendFactor = g_BlendFactor[desc.Blend.Src];
        renderbufferAttachment.sourceAlphaBlendFactor = g_BlendFactor[desc.Blend.SrcBlendAlpha];
        renderbufferAttachment.destinationRGBBlendFactor = g_BlendFactor[desc.Blend.Dest];
        renderbufferAttachment.destinationAlphaBlendFactor = g_BlendFactor[desc.Blend.DestBlendAlpha];
        
        m_RenderPipelineDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        
        
        // vertex descriptor setup
        for(uint32 i = 0; i<rhi::VertexInputState::kMaxVertexBindings; i++)
        {
            auto attrib = desc.InputState.Attribs[i];
            if(attrib.Slot==rhi::VertexInputState::kInvalidValue)
                break;
            m_RenderPipelineDesc.vertexDescriptor.attributes[i].format = g_VertexFormats[attrib.Format];
            m_RenderPipelineDesc.vertexDescriptor.attributes[i].offset = attrib.OffSet;
            m_RenderPipelineDesc.vertexDescriptor.attributes[i].bufferIndex = attrib.Slot;
        }
        
        for(uint32 i = 0; i<rhi::VertexInputState::kMaxVertexBindings; i++)
        {
            auto layout = desc.InputState.Layouts[i];
            if(layout.Stride == rhi::VertexInputState::kInvalidValue)
                break;
            m_RenderPipelineDesc.vertexDescriptor.layouts[i].stride = layout.Stride;
            m_RenderPipelineDesc.vertexDescriptor.layouts[i].stepRate = 1;
            m_RenderPipelineDesc.vertexDescriptor.layouts[i].stepFunction = g_VertexInputRates[layout.Rate];
        }
        
        // shader setup
        AssignShader(desc.Shaders[rhi::ES_Vertex]);
        AssignShader(desc.Shaders[rhi::ES_Fragment]);
    }
    else
    {
        m_ComputePipelineDesc = [[MTLComputePipelineDescriptor alloc] init];
    }
//    m_RenderPipelineDesc.
    
}

void PipelineState::AssignShader(const rhi::ShaderBundle &shaderBundle)
{
    if(m_Type == rhi::EPSO_Graphics)
    {
        if(m_RenderPipelineDesc)
        {
            const auto & shaderData = shaderBundle.RawData;
            if (shaderData.Length() == 0)
                return;
            NSError * error = nil;
            dispatch_data_t data = dispatch_data_create(shaderData.Data(), shaderData.Length(), nil, nil);
            id<MTLLibrary> lib = [m_Device newLibraryWithData:data error:&error];
            if(error)
            {
                MTLLOGE("Failed to AssignShader, error: %s, occured %s@%d.", [[error localizedFailureReason] UTF8String], __FILE__, __LINE__);
            }
            NSString *entryName = [NSString stringWithFormat:@"%@0",
                                  [NSString stringWithUTF8String:shaderBundle.Desc.EntryFunction.CStr()]];
            id<MTLFunction> function = [lib newFunctionWithName:entryName];
            switch(shaderBundle.Desc.Stage)
            {
                case rhi::ES_Vertex:
                    m_RenderPipelineDesc.vertexFunction = function;
                    break;
                case rhi::ES_Fragment:
                    m_RenderPipelineDesc.fragmentFunction = function;
                    break;
            }
        }
    }
}

void PipelineState::SetLayout(rhi::PipelineLayoutRef)
{
    
}

void PipelineState::SetShader(rhi::EShaderType, rhi::ShaderBundle const&)
{
    
}

void PipelineState::SetRasterizerState(const rhi::RasterizerState&)
{
    
}

void PipelineState::SetBlendState(const rhi::BlendState&)
{
    
}

void PipelineState::SetDepthStencilState(const rhi::DepthStencilState&)
{
    
}

void PipelineState::SetPrimitiveTopology(const rhi::EPrimitiveType)
{
    
}

void PipelineState::SetVertexInputLayout(rhi::VertexDeclaration const*, uint32 Count)
{
    
}

void PipelineState::SetRenderTargetFormat(const rhi::RenderTargetFormat &)
{
    
}

void PipelineState::SetSampler(rhi::SamplerRef)
{
    
}

void PipelineState::Finalize()
{
    NSError *error = NULL;
    if(m_Type==rhi::EPSO_Graphics)
    {
        m_DepthStencilState = [m_Device newDepthStencilStateWithDescriptor:m_DepthStencilDesc];
        m_RenderPipelineState = [m_Device newRenderPipelineStateWithDescriptor:m_RenderPipelineDesc error:&error];
        if (error)
        {
            MTLLOGE("Failed to created render pipeline state, error: %s", [error.localizedDescription cStringUsingEncoding:NSASCIIStringEncoding]);
        }
    }
    else
    {
//        m_ComputePipelineState = [m_Device newCom]
    }
}

NS_K3D_METAL_END
