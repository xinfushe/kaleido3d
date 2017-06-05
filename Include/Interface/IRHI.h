#pragma once

#include "../KTL/SharedPtr.hpp"
#include "../Math/kGeometry.hpp"
#include "NGFXStructs.h"
#include "ICrossShaderCompiler.h"
#include "NGFX.h"

K3D_COMMON_NS
{
struct IObject
{
  virtual void SetName(const char*) {}
  virtual void Release() = 0;
};

typedef SharedPtr<struct NGFXCommandQueue> NGFXCommandQueueRef;
typedef SharedPtr<struct NGFXCommandBuffer> NGFXCommandBufferRef;
typedef SharedPtr<struct NGFXRenderCommandEncoder> NGFXRenderCommandEncoderRef;
typedef SharedPtr<struct NGFXComputeCommandEncoder> NGFXComputeCommandEncoderRef;
typedef SharedPtr<struct NGFXParallelRenderCommandEncoder> NGFXParallelRenderCommandEncoderRef;
typedef SharedPtr<struct NGFXDevice> NGFXDeviceRef;
typedef SharedPtr<struct NGFXFactory> NGFXFactoryRef;
typedef SharedPtr<struct NGFXResource> NGFXResourceRef;
typedef SharedPtr<struct NGFXTexture> NGFXTextureRef;
typedef SharedPtr<struct NGFXBuffer> NGFXBufferRef;
typedef SharedPtr<struct NGFXTextureView> TextureViewRef;
typedef SharedPtr<struct NGFXBufferView> BufferViewRef;

// buffer: texel buffer ReadOnly 
// texture: sampled ReadOnly
typedef SharedPtr<struct NGFXShaderResourceView> NGFXSRVRef;

// buffer: RWBuffer, RWStructedBuffer,
// texture: RWTexture
typedef SharedPtr<struct NGFXUnorderedAccessView> NGFXUAVRef;
typedef SharedPtr<struct NGFXPipelineState> NGFXPipelineStateRef;
typedef SharedPtr<struct NGFXRenderPipelineState> RenderPipelineStateRef;
typedef SharedPtr<struct NGFXComputePipelineState> ComputePipelineStateRef;
typedef SharedPtr<struct NGFXRenderPipelineEncoder> RenderPipelineEncoderRef;
typedef SharedPtr<struct NGFXComputePipelineEncoder> ComputePipelineEncoderRef;
typedef SharedPtr<struct NGFXSwapChain> NGFXSwapChainRef;
typedef SharedPtr<struct NGFXSampler> NGFXSamplerRef;
typedef SharedPtr<struct NGFXBindingGroup> NGFXBindingGroupRef;
typedef SharedPtr<struct NGFXRenderTarget> NGFXRenderTargetRef;
typedef SharedPtr<struct NGFXRenderpass> NGFXRenderpassRef;
/**
 * Semaphore used for GPU to GPU syncs,
 * Specifically used to sync queue submissions (on the same or different
 * queues), Set on GPU, Wait on GPU (inter-queue) Events: Set anywhere, Wait on
 * GPU (intra-queue)
 */
struct NGFXSemaphore;

/**
 * @see
 * https://www.reddit.com/r/vulkan/comments/47tc3s/differences_between_vkfence_vkevent_and/?
 * Fence is GPU to CPU syncs
 * Set on GPU, Wait on CPU
 */
typedef SharedPtr<struct NGFXFence> NGFXFenceRef;

/**
  * Vulkan has pipeline layout,
  * D3D12 is root signature.
  * PipelineLayout defines shaders impose constraints on table layout.
  */
typedef SharedPtr<struct NGFXPipelineLayout> NGFXPipelineLayoutRef;

struct NGFXResource : public IObject
{
  virtual ~NGFXResource() {}
  virtual void* Map(uint64 start, uint64 size) = 0;
  virtual void UnMap() = 0;

  virtual uint64 GetLocation() const { return 0; }
  virtual ResourceDesc GetDesc() const = 0;

  /**
   * Vulkan: texture uses image layout as resource state <br/>
   * D3D12: used for transition, maybe used as ShaderVisiblity determination in <br/>
   * STATIC_SAMPLER and descriptor table
   */
  virtual NGFXResourceState GetState() const { return NGFX_RESOURCE_STATE_UNKNOWN; }
  virtual uint64 GetSize() const = 0;
};

struct NGFXSampler
{
  virtual SamplerState GetSamplerDesc() const = 0;
  virtual ~NGFXSampler() {}
};

struct NGFXShaderResourceView
{
  virtual ~NGFXShaderResourceView() {}
  virtual NGFXResourceRef GetResource() const = 0;
  virtual SRVDesc GetDesc() const = 0;
};

struct NGFXTexture : public NGFXResource
{
  virtual ~NGFXTexture() {}
  virtual NGFXSamplerRef GetSampler() const = 0;
  virtual void BindSampler(NGFXSamplerRef) = 0;
  virtual void SetResourceView(NGFXSRVRef) = 0;
  virtual NGFXSRVRef GetResourceView() const = 0;
};

struct NGFXBuffer : public NGFXResource
{
  virtual ~NGFXBuffer() {}
};

struct NGFXUnorderedAccessView
{
  virtual ~NGFXUnorderedAccessView() {}
};

struct NGFXBindingGroup
{
  virtual void Update(uint32 bindSet, NGFXResourceRef) = 0;
  virtual void Update(uint32 bindSet, NGFXUAVRef) = 0;
  virtual void Update(uint32 bindSet, NGFXSamplerRef){};
  virtual uint32 GetSlotNum() const { return 0; }
  virtual ~NGFXBindingGroup() {}
};

/**
 * Vulkan has pipeline layout,
 * D3D12 is root signature.
 * PipelineLayout defines shaders impose constraints on table layout.
 */
struct NGFXPipelineLayout
{
  virtual NGFXBindingGroupRef ObtainBindingGroup() = 0;
  virtual ~NGFXPipelineLayout() {}
};

/**
 * @see
 * https://www.reddit.com/r/vulkan/comments/47tc3s/differences_between_vkfence_vkevent_and/?
 * Fence is GPU to CPU syncs
 * Set on GPU, Wait on CPU
 */
struct NGFXFence
{
  virtual void Signal(int32 fenceVal) = 0;
  virtual void Reset() {}
  virtual void WaitFor(uint64 time) = 0;
  virtual ~NGFXFence() {}
};

struct NGFXRenderTarget
{
  virtual ~NGFXRenderTarget() {}
  virtual void SetClearColor(kMath::Vec4f clrColor) = 0;
  virtual void SetClearDepthStencil(float depth, uint32 stencil) = 0;
  virtual NGFXResourceRef GetBackBuffer() = 0;
};

struct ComputePipelineStateDesc
{
  NGFXShaderBundle ComputeShader;
};

struct NGFXPipelineState
{
  virtual ~NGFXPipelineState() {}
  virtual NGFXPipelineType GetType() const = 0;

  // rebuild pipeline state if is dirty
  virtual void Rebuild() = 0;

  // storage options
  virtual void SavePSO(::k3d::String const& Path) {}
  virtual void LoadPSO(::k3d::String const& Path) {}
};

struct NGFXRenderPipelineState : public NGFXPipelineState
{
  virtual ~NGFXRenderPipelineState() {}

  virtual void SetRasterizerState(const RasterizerState&) = 0;
  virtual void SetBlendState(const BlendState&) = 0;
  virtual void SetDepthStencilState(const DepthStencilState&) = 0;
  virtual void SetPrimitiveTopology(const NGFXPrimitiveType) = 0;
  virtual void SetSampler(NGFXSamplerRef) = 0;
};

struct NGFXComputePipelineState : public NGFXPipelineState
{
  virtual ~NGFXComputePipelineState() {}
};

using PipelineLayoutDesc = NGFXShaderBindingTable;

struct AttachmentDesc
{
  AttachmentDesc()
    : LoadAction(NGFX_LOAD_ACTION_CLEAR)
    , StoreAction(NGFX_STORE_ACTION_STORE)
  {
  }
  NGFXLoadAction LoadAction;
  NGFXStoreAction StoreAction;
  NGFXTextureRef pTexture;
};

struct ColorAttachmentDesc : public AttachmentDesc
{
  kMath::Vec4f ClearColor;
};

using ColorAttachmentArray = k3d::DynArray<ColorAttachmentDesc>;

struct DepthAttachmentDesc : public AttachmentDesc
{
  float ClearDepth;
};

struct StencilAttachmentDesc : public AttachmentDesc
{
  float ClearStencil;
};

struct RenderPassDesc
{
  ColorAttachmentArray ColorAttachments;
  SharedPtr<DepthAttachmentDesc> pDepthAttachment;
  SharedPtr<StencilAttachmentDesc> pStencilAttachment;
};

struct AttachmentDescriptor : public NGFXRefCounted<false>
{
  
};

struct RenderPassDescriptor : public NGFXRefCounted<false>
{
  
};

struct NGFXRenderpass : public IObject
{
  virtual RenderPassDesc GetDesc() const = 0;
};

struct NGFXDevice : public IObject
{
  enum Result
  {
    DeviceNotFound,
    DeviceFound
  };

  virtual ~NGFXDevice() {}

  virtual NGFXResourceRef CreateResource(ResourceDesc const&) = 0;
  virtual NGFXSRVRef CreateShaderResourceView(
    NGFXResourceRef,
    SRVDesc const&) = 0;

  virtual NGFXUAVRef CreateUnorderedAccessView(const NGFXResourceRef&, UAVDesc const&) = 0;

  virtual NGFXSamplerRef CreateSampler(const SamplerState&) = 0;

  virtual NGFXPipelineLayoutRef CreatePipelineLayout(
    PipelineLayoutDesc const& table) = 0;
  virtual NGFXFenceRef CreateFence() = 0;

//  virtual NGFXRenderTargetRef NewRenderTarget(RenderTargetLayout const&) = 0;

  virtual NGFXRenderpassRef CreateRenderPass(RenderPassDesc const&) = 0;

  virtual NGFXPipelineStateRef CreateRenderPipelineState(
    RenderPipelineStateDesc const&,
    NGFXPipelineLayoutRef,
    NGFXRenderpassRef) = 0;

  virtual NGFXPipelineStateRef CreateComputePipelineState(
    ComputePipelineStateDesc const&,
    NGFXPipelineLayoutRef) = 0;

  virtual NGFXCommandQueueRef CreateCommandQueue(NGFXCommandType const&) = 0;

  virtual void WaitIdle() {}

  /**
   * equal with d3d12's getcopyfootprint or vulkan's getImagesubreslayout.
   */
  virtual void QueryTextureSubResourceLayout(
    NGFXTextureRef,
    TextureSpec const& spec,
    SubResourceLayout*) 
  {}
};

struct NGFXFactory : public IObject
{
  virtual void EnumDevices(k3d::DynArray<NGFXDeviceRef>& Devices) = 0;
  virtual NGFXSwapChainRef CreateSwapchain(NGFXCommandQueueRef pCommandQueue,
                                       void* nPtr,
                                       SwapChainDesc&) = 0;
};

struct NGFXSwapChain : public IObject
{
  virtual void Resize(uint32 Width, uint32 Height) = 0;
  virtual void Present() = 0;
  virtual NGFXTextureRef GetCurrentTexture() = 0;
};

struct TextureCopyLocation
{
  typedef ::k3d::DynArray<uint32> ResIds;
  typedef ::k3d::DynArray<PlacedSubResourceFootprint> ResFootprints;

  enum ESubResource
  {
    ESubResourceIndex,
    ESubResourceFootPrints
  };

  TextureCopyLocation(NGFXResourceRef ptrResource, ResIds subResourceIndex)
    : pResource(ptrResource)
    , SubResourceIndexes(subResourceIndex)
  {
  }

  TextureCopyLocation(NGFXResourceRef ptrResource, ResFootprints footprints)
    : pResource(ptrResource)
    , SubResourceFootPrints(footprints)
  {
  }

  TextureCopyLocation() = default;

  ~TextureCopyLocation() {}

  NGFXResourceRef pResource;
  ESubResource SubResourceType = ESubResourceIndex;
  ResIds SubResourceIndexes;
  ResFootprints SubResourceFootPrints;
};

struct NGFXCommandQueue
{
  virtual NGFXCommandBufferRef ObtainCommandBuffer(NGFXCommandReuseType const&) = 0;
};

struct NGFXCommandBuffer : public IObject
{
  virtual void Commit(NGFXFenceRef pFence = nullptr) = 0;
  // For Vulkan, This command will be appended to the tail
  virtual void Present(NGFXSwapChainRef pSwapChain, NGFXFenceRef pFence) = 0;
  virtual void Reset() = 0;
  virtual NGFXRenderCommandEncoderRef RenderCommandEncoder(
    RenderPassDesc const&) = 0;
  virtual NGFXComputeCommandEncoderRef ComputeCommandEncoder() = 0;
  virtual NGFXParallelRenderCommandEncoderRef ParallelRenderCommandEncoder(
    RenderPassDesc const&) = 0;
  // blit
  virtual void CopyTexture(const TextureCopyLocation& Dest,
                           const TextureCopyLocation& Src) = 0;
  //
  virtual void CopyBuffer(NGFXResourceRef Dest,
                          NGFXResourceRef Src,
                          CopyBufferRegion const& Region) = 0;
  virtual void Transition(NGFXResourceRef pResource,
                          NGFXResourceState const&
                            State /*, rhi::EPipelineStage const& Stage*/) = 0;
};

struct NGFXCommandEncoder
{
  virtual void SetPipelineState(uint32 HashCode, NGFXPipelineStateRef const&) = 0;
  virtual void SetBindingGroup(NGFXBindingGroupRef const&) = 0;
  virtual void EndEncode() = 0;
};

struct NGFXRenderCommandEncoder : public NGFXCommandEncoder
{
  virtual void SetScissorRect(const Rect&) = 0;
  virtual void SetViewport(const ViewportDesc&) = 0;
  virtual void SetIndexBuffer(const IndexBufferView& IBView) = 0;
  virtual void SetVertexBuffer(uint32 Slot, const VertexBufferView& VBView) = 0;
  virtual void SetPrimitiveType(NGFXPrimitiveType) = 0;
  virtual void DrawInstanced(DrawInstancedParam) = 0;
  virtual void DrawIndexedInstanced(DrawIndexedInstancedParam) = 0;
};

struct NGFXComputeCommandEncoder : public NGFXCommandEncoder
{
  virtual void Dispatch(uint32 GroupCountX,
                        uint32 GroupCountY,
                        uint32 GroupCountZ) = 0;
};
// Begin with a single rendering pass, encoded from multiple threads
// simultaneously.
struct NGFXParallelRenderCommandEncoder : public NGFXCommandEncoder
{
  virtual NGFXRenderCommandEncoderRef SubRenderCommandEncoder() = 0;
};

}
