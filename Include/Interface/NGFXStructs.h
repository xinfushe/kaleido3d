#ifndef __RHIStructs_h__
#define __RHIStructs_h__

#include "NGFXEnums.h"
#include "ShaderCommon.h"

K3D_COMMON_NS
{
  /* for vulkan
  enum EPipelineStage
  {
  EPS_Top				= 0x1,
  EPS_VertexShader	= 0x8,
  EPS_PixelShader		= 0x80,
  EPS_PixelOutput		= 0x400,
  EPS_ComputeShader	= 0x800,
  EPS_Transfer		= 0x1000,
  EPS_Bottom			= 0x2000,
  EPS_Graphics		= 0x8000,
  EPS_All				= 0x10000
  };*/

  struct BlendState
  {
    bool Enable;
    uint32 ColorWriteMask;

    NGFXBlendFactor Src;
    NGFXBlendFactor Dest;
    NGFXBlendOperation Op;

    NGFXBlendFactor SrcBlendAlpha;
    NGFXBlendFactor DestBlendAlpha;
    NGFXBlendOperation BlendAlphaOp;

    BlendState()
    {
      Enable = false;
      Src = NGFXBlendFactor::NGFX_BLEND_FACTOR_ONE;
      Dest = NGFXBlendFactor::NGFX_BLEND_FACTOR_ZERO;
      Op = NGFXBlendOperation::NGFX_BLEND_OP_ADD;

      SrcBlendAlpha = NGFXBlendFactor::NGFX_BLEND_FACTOR_ONE;
      DestBlendAlpha = NGFXBlendFactor::NGFX_BLEND_FACTOR_ZERO;
      BlendAlphaOp = NGFXBlendOperation::NGFX_BLEND_OP_ADD;
      ColorWriteMask = 0xf;
    }
  };

  struct AttachmentState
  {
    BlendState Blend;
    NGFXPixelFormat Format; // for metal
  };

  struct RasterizerState
  {
    NGFXFillMode FillMode;
    NGFXCullMode CullMode;
    bool FrontCCW;
    int32 DepthBias;
    float DepthBiasClamp;
    bool DepthClipEnable;
    bool MultiSampleEnable;
    NGFXMultiSampleFlag MSFlag;

    RasterizerState()
    {
      FillMode = NGFXFillMode::NGFX_FILL_MODE_SOLID;
      CullMode = NGFXCullMode::NGFX_CULL_MODE_BACK;
      FrontCCW = false;
      DepthBias = 0;
      DepthBiasClamp = 0.0f;
      DepthClipEnable = true;
      MultiSampleEnable = false;
    }
  };

  struct DepthStencilState
  {
    struct Op
    {
      NGFXStencilOp StencilFailOp;
      NGFXStencilOp DepthStencilFailOp;
      NGFXStencilOp StencilPassOp;
      NGFXComparisonFunc StencilFunc;

      Op()
      {
        StencilFailOp = NGFXStencilOp::NGFX_STENCIL_OP_KEEP;
        DepthStencilFailOp = NGFXStencilOp::NGFX_STENCIL_OP_KEEP;
        StencilPassOp = NGFXStencilOp::NGFX_STENCIL_OP_KEEP;
        StencilFunc = NGFXComparisonFunc::NGFX_COMPARISON_FUNCTION_ALWAYS;
      }
    };

    bool DepthEnable;
    NGFXDepthWriteMask DepthWriteMask;
    NGFXComparisonFunc DepthFunc;

    bool StencilEnable;
    uint8 StencilReadMask;
    uint8 StencilWriteMask;
    Op FrontFace;
    Op BackFace;

    DepthStencilState()
    {
      DepthEnable = false;
      DepthWriteMask = NGFXDepthWriteMask::NGFX_DEPTH_WRITE_MASK_ALL;
      DepthFunc = NGFXComparisonFunc::NGFX_COMPARISON_FUNCTION_LESS;

      StencilEnable = false;
      StencilReadMask = 0xff;
      StencilWriteMask = 0xff;
    }
  };

  struct SamplerState
  {
    struct Filter
    {
      NGFXFilterMethod MinFilter;
      NGFXFilterMethod MagFilter;
      NGFXFilterMethod MipMapFilter;
      NGFXFilterReductionType ReductionType;
      Filter()
      {
        MinFilter = NGFXFilterMethod::NGFX_FILTER_METHOD_LINEAR;
        MagFilter = NGFXFilterMethod::NGFX_FILTER_METHOD_LINEAR;
        MipMapFilter = NGFXFilterMethod::NGFX_FILTER_METHOD_LINEAR;
        ReductionType = NGFXFilterReductionType::EFRT_Standard;
      }
    };

    Filter Filter;
    NGFXAddressMode U, V, W;
    float MipLODBias;
    uint32 MaxAnistropy;
    NGFXComparisonFunc ComparisonFunc;
    float BorderColor[4];
    float MinLOD;
    float MaxLOD;

    SamplerState()
      : U(NGFXAddressMode::NGFX_ADDRESS_MODE_WRAP)
      , V(NGFXAddressMode::NGFX_ADDRESS_MODE_WRAP)
      , W(NGFXAddressMode::NGFX_ADDRESS_MODE_WRAP)
      , MipLODBias(0)
      , MaxAnistropy(16)
      , ComparisonFunc(NGFXComparisonFunc::NGFX_COMPARISON_FUNCTION_LESS_EQUAL)
      , MinLOD(0)
      , MaxLOD(3.402823466e+38f)
    {
    }
  };

  struct VertexInputState
  {
    enum
    {
      kInvalidValue = -1,
      kMaxVertexLayouts = 4,
      kMaxVertexBindings = 8,
    };

    struct Attribute
    {
      Attribute(NGFXVertexFormat const& format = NGFX_VERTEX_FORMAT_FLOAT3X32,
        uint32 offset = kInvalidValue,
        uint32 slot = kInvalidValue)
        : Format(format)
        , OffSet(offset)
        , Slot(slot)
      {
      }

      NGFXVertexFormat Format /* = NGFX_VERTEX_FORMAT_FLOAT3X32*/;
      uint32 OffSet /* = kInvalidValue*/;
      uint32 Slot /* = kInvalidValue*/;
    };

    struct Layout
    {
      Layout(NGFXVertexInputRate const& inputRate = NGFX_VERTEX_INPUT_RATE_PER_VERTEX,
        uint32 stride = kInvalidValue)
        : Rate(inputRate)
        , Stride(stride)
      {
      }

      NGFXVertexInputRate Rate /* = NGFX_VERTEX_INPUT_RATE_PER_VERTEX */;
      uint32 Stride /* = kInvalidValue*/;
    };

    VertexInputState() {}

    Attribute Attribs[kMaxVertexBindings];
    Layout Layouts[kMaxVertexLayouts];
  };

  using AttachmentStateArray = k3d::DynArray<AttachmentState>;

  struct RenderPipelineStateDesc
  {
    RenderPipelineStateDesc()
      : PrimitiveTopology(NGFX_PRIMITIVE_TRIANGLES)
      , PatchControlPoints(0)
      , DepthAttachmentFormat()
    {
    }
    RasterizerState Rasterizer;
    AttachmentStateArray AttachmentsBlend;  // 
    DepthStencilState DepthStencil;
    NGFXPixelFormat DepthAttachmentFormat;     // for metal
    VertexInputState InputState;
    // InputAssemblyState
    NGFXPrimitiveType PrimitiveTopology /* = rhi::NGFX_PRIMITIVE_TRIANGLES */;
    // Tessellation Patch
    uint32 PatchControlPoints /* = 0*/;

    NGFXShaderBundle VertexShader;
    NGFXShaderBundle PixelShader;
    NGFXShaderBundle GeometryShader;
    NGFXShaderBundle DomainShader;
    NGFXShaderBundle HullShader;
  };

  struct Rect
  {
    long Left;
    long Top;
    long Right;
    long Bottom;
  };

  struct ViewportDesc
  {
    ViewportDesc()
      : ViewportDesc(0.f, 0.f)
    {
    }

    ViewportDesc(float width,
                 float height,
                 float left = 0.0f,
                 float top = 0.0f,
                 float minDepth = 0.0f,
                 float maxDepth = 1.0f)
      : Left(left)
      , Top(top)
      , Width(width)
      , Height(height)
      , MinDepth(minDepth)
      , MaxDepth(maxDepth)
    {
    }

    float Left;
    float Top;
    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;
  };

  struct VertexBufferView
  {
    uint64 BufferLocation;
    uint32 SizeInBytes;
    uint32 StrideInBytes;
  };

  struct IndexBufferView
  {
    uint64 BufferLocation;
    uint32 SizeInBytes;
  };

  struct SwapChainDesc
  {
    NGFXPixelFormat Format;
    uint32 Width;
    uint32 Height;
    uint32 NumBuffers;
  };

  /**
   * Format, Width, Height, Depth, MipLevel, Layers
   */
  struct TextureDesc
  {
    NGFXPixelFormat Format;
    uint32 Width;
    uint32 Height;
    uint32 Depth;
    uint32 MipLevels;
    uint32 Layers;

    TextureDesc(
      NGFXPixelFormat _Format = NGFX_PIXEL_FORMAT_RGBA8_UNORM
      , uint32 _Width = 0
      , uint32 _Height = 0
      , uint32 _Depth = 1
      , uint32 _MipLevels = 1
      , uint32 _Layers = 1)
      : Format(_Format)
      , Width(_Width)
      , Height(_Height)
      , Depth(_Depth)
      , MipLevels(_MipLevels)
      , Layers(_Layers)
    {
    }

    bool IsTex1D() const { return Depth == 1 && Layers == 1 && Height == 1; }
    bool IsTex2D() const { return Depth == 1 && Layers == 1 && Height > 1; }
    bool IsTex3D() const { return Depth > 1 && Layers == 1; }
  };

  /* in d3d12 and vulkan, each resource has a memory for resource foot print
   * which needs extra memory. */
  struct ResourceDesc
  {
    NGFXResourceType Type;
    NGFXResourceAccessFlag Flag;
    NGFXResourceCreationFlag CreationFlag;
    NGFXResourceViewTypeBits ViewFlags; // for resource view prediction
    NGFXResourceOrigin Origin;
    union
    {
      TextureDesc TextureDesc;
      uint64 Size;
    };
    ResourceDesc()
      : Type(NGFX_BUFFER)
      , Flag(NGFX_ACCESS_READ_AND_WRITE)
      , CreationFlag(NGFX_RESOURCE_DYNAMIC)
      , ViewFlags(NGFX_RESOURCE_SHADER_RESOURCE_VIEW)
      , Origin(NGFX_RESOURCE_ORIGIN_USER)
    {
    }
    ResourceDesc(NGFXResourceType t,
                 NGFXResourceAccessFlag f,
                 NGFXResourceCreationFlag cf,
                 NGFXResourceViewTypeBits viewType)
      : Type(t)
      , Flag(f)
      , CreationFlag(cf)
      , ViewFlags(viewType)
      , Origin(NGFX_RESOURCE_ORIGIN_USER)
    {
    }
  };

  /*same as VkImageSubresource */
  struct TextureSpec
  {
    NGFXTextureAspectFlag Aspect;
    uint32 MipLevel;
    uint32 ArrayLayer;
  };

  struct BufferSpec
  {
    uint64 FirstElement; // for vulkan, offset = FirstElement * StructureByteStride;
    uint64 NumElements; // range = NumElements * StructureByteStride
    uint64 StructureByteStride;
  };

  /**
   * Format
   * BufferSpec(First, NumElement, Stride)
   * TextureSpec
   */
  struct UAVDesc
  {
    NGFXViewDimension Dim;
    NGFXPixelFormat Format;
    union
    {
      BufferSpec Buffer;
      TextureSpec Texture;
    };
  };

  /**
   * ReadOnly
   */
  struct SRVDesc
  {
    NGFXViewDimension Dim;
    NGFXPixelFormat Format;
    union
    {
      BufferSpec Buffer;
      TextureSpec Texture;
    };
  };

  struct ResourceViewDesc
  {
    NGFXResourceViewTypeBits ViewType;
    union 
    {
      TextureSpec TextureSpec;
    };
  };
  /**
   * Same as VkSubresourceLayout
   */
  struct SubResourceLayout
  {
    uint64 Offset = 0;
    uint64 Size = 0;
    uint64 RowPitch = 0;
    uint64 ArrayPitch = 0;
    uint64 DepthPitch = 0;
  };

  struct CopyBufferRegion
  {
    uint64 SrcOffSet;
    uint64 DestOffSet;
    uint64 CopySize;
  };

  /* see also VkBufferImageCopy(vk) and
   * D3D12_PLACED_SUBRESOURCE_FOOTPRINT(d3d12)
   */
  struct SubResourceFootprint
  {
    TextureDesc Dimension;
    SubResourceLayout SubLayout;
  };

  struct PlacedSubResourceFootprint
  {
    uint32 BufferOffSet = 0; // VkBufferImageCopy#bufferOffset
    int32 TOffSetX = 0, TOffSetY = 0,
          TOffSetZ = 0;                  // VkBufferImageCopy#imageOffset
    SubResourceFootprint Footprint = {}; // VkBufferImageCopy#imageExtent
  };

  /**
   * IndexCountPerInstance
   * InstanceCount
   * StartLocation
   * BaseVertexLocation
   * StartInstanceLocation
   */
  struct DrawIndexedInstancedParam
  {
    DrawIndexedInstancedParam(uint32 indexCountPerInst,
                              uint32 instCount,
                              uint32 startLoc = 0,
                              uint32 baseVerLoc = 0,
                              uint32 sInstLoc = 1)
      : IndexCountPerInstance(indexCountPerInst)
      , InstanceCount(instCount)
      , StartIndexLocation(startLoc)
      , BaseVertexLocation(baseVerLoc)
      , StartInstanceLocation(sInstLoc)
    {
    }
    uint32 IndexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartIndexLocation;
    uint32 BaseVertexLocation;
    uint32 StartInstanceLocation;
  };

  struct DrawInstancedParam
  {
    DrawInstancedParam(uint32 vertexPerInst,
                       uint32 instances,
                       uint32 startLocation = 0,
                       uint32 startInstLocation = 0)
      : VertexCountPerInstance(vertexPerInst)
      , InstanceCount(instances)
      , StartVertexLocation(startLocation)
      , StartInstanceLocation(startInstLocation)
    {
    }
    uint32 VertexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartVertexLocation;
    uint32 StartInstanceLocation;
  };
}

#endif