#include "Kaleido3D.h"
#include "Math/IK.hpp"
#include "MetalRHI.h"
#include "MetalEnums.h"

NS_K3D_METAL_BEGIN

CommandContext::CommandContext(rhi::ECommandType const & cmdType, id<MTLCommandBuffer> cmdBuf)
: m_CommandType(cmdType)
, m_CmdBuffer(cmdBuf)
{
}

CommandContext::~CommandContext()
{
    
}

void CommandContext::Detach(rhi::IDevice *)
{
    
}

void CommandContext::Begin()
{
    if(m_CommandType == rhi::ECMD_Compute)
    {
        m_ComputeEncoder = [m_CmdBuffer computeCommandEncoder];
    }
    else if(m_CommandType == rhi::ECMD_Bundle)
    {
        m_ParallelRenderEncoder = [m_CmdBuffer parallelRenderCommandEncoderWithDescriptor:m_RenderpassDesc];
    }
}

void CommandContext::CopyTexture(const rhi::TextureCopyLocation& Dest, const rhi::TextureCopyLocation& Src)
{
    
}

void CommandContext::CopyBuffer(rhi::IGpuResource& Dest, rhi::IGpuResource& Src, rhi::CopyBufferRegion const & Region)
{
    
}

void CommandContext::Execute(bool Wait)
{
    [m_CmdBuffer commit];
}

void CommandContext::Reset()
{
}

void CommandContext::BeginRendering()
{
}

void CommandContext::SetRenderTarget(rhi::RenderTargetRef pRenderTarget)
{
    auto pRt = k3d::StaticPointerCast<RenderTarget>(pRenderTarget);
    m_RenderpassDesc = pRt->m_RenderPassDescriptor;
    m_RenderEncoder = [m_CmdBuffer renderCommandEncoderWithDescriptor:m_RenderpassDesc];
}

void CommandContext::SetIndexBuffer(const rhi::IndexBufferView& IBView)
{
    m_TmpIndexBuffer = (id<MTLBuffer>)IBView.BufferLocation;
}

void CommandContext::SetVertexBuffer(uint32 Slot, const rhi::VertexBufferView& VBView)
{
    [m_RenderEncoder setVertexBuffer:(id<MTLBuffer>)VBView.BufferLocation offset:0 atIndex:Slot];
}

void CommandContext::SetPipelineState(uint32 HashCode, rhi::PipelineStateObjectRef pPipelineState)
{
    if(!pPipelineState)
    {
        MTLLOGE("CommandContext::setPipelineState pPipelineState==null!");
        return;
    }
    auto mtlPs = StaticPointerCast<PipelineState>(pPipelineState);
    if(pPipelineState->GetType() == rhi::EPSO_Graphics)
    {
        [m_RenderEncoder setRenderPipelineState:    mtlPs->m_RenderPipelineState];
        [m_RenderEncoder setDepthStencilState:      mtlPs->m_DepthStencilState];
        [m_RenderEncoder setCullMode:               mtlPs->m_CullMode];
        m_CurPrimType = mtlPs->m_PrimitiveType;
    }
    else
    {
        [m_ComputeEncoder setComputePipelineState:  mtlPs->m_ComputePipelineState];
    }
}

void CommandContext::SetViewport(const rhi::ViewportDesc & vpDesc)
{
    MTLViewport viewport = { vpDesc.Left, vpDesc.Top, vpDesc.Width, vpDesc.Height, vpDesc.MinDepth, vpDesc.MaxDepth };
    [m_RenderEncoder setViewport:viewport];
}

void CommandContext::SetPrimitiveType(rhi::EPrimitiveType Type)
{
    m_CurPrimType = g_PrimitiveType[Type];
}

void CommandContext::DrawInstanced(rhi::DrawInstancedParam param)
{
    [m_RenderEncoder drawPrimitives:m_CurPrimType
                        vertexStart:param.StartVertexLocation
                        vertexCount:param.VertexCountPerInstance
                      instanceCount:param.InstanceCount
                       baseInstance:param.StartInstanceLocation];
}

void CommandContext::DrawIndexedInstanced(rhi::DrawIndexedInstancedParam param)
{
    [m_RenderEncoder drawIndexedPrimitives:m_CurPrimType
                                indexCount:param.IndexCountPerInstance
                                 indexType:MTLIndexTypeUInt32
                               indexBuffer:m_TmpIndexBuffer
                         indexBufferOffset:param.StartIndexLocation];
}

void CommandContext::EndRendering()
{
    [m_RenderEncoder endEncoding];
}

void CommandContext::Dispatch(uint32 x, uint32 y, uint32 z)
{
    [m_ComputeEncoder dispatchThreadgroups:MTLSizeMake(x, y, z) threadsPerThreadgroup:MTLSizeMake(x, y, z)];
}

void CommandContext::End()
{
    
}

void CommandContext::PresentInViewport(rhi::RenderViewportRef rvp)
{
    [m_CmdBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        
    }];
    auto vp = k3d::StaticPointerCast<RenderViewport>(rvp);
    [m_CmdBuffer presentDrawable:vp->m_CurrentDrawable];
    [m_CmdBuffer commit];
}

NS_K3D_METAL_END
