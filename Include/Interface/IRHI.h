#pragma once

#include "ICrossShaderCompiler.h"

namespace rhi {
struct IObject
{
  virtual void Release() = 0;
};

K3D_DEPRECATED("Use Command Buffer Instead")
struct ICommandContext;
typedef ::k3d::SharedPtr<ICommandContext> CommandContextRef;

struct ICommandQueue;
typedef ::k3d::SharedPtr<ICommandQueue> CommandQueueRef;
struct ICommandBuffer;
typedef ::k3d::SharedPtr<ICommandBuffer> CommandBufferRef;
struct IRenderCommandEncoder;
typedef ::k3d::SharedPtr<IRenderCommandEncoder> RenderCommandEncoderRef;
struct IComputeCommandEncoder;
typedef ::k3d::SharedPtr<IComputeCommandEncoder> ComputeCommandEncoderRef;
struct IParallelRenderCommandEncoder;
typedef ::k3d::SharedPtr<IParallelRenderCommandEncoder> ParallelRenderCommandEncoderRef;

K3D_DEPRECATED("Use Factory Instead")
struct IDeviceAdapter;
typedef ::k3d::SharedPtr<IDeviceAdapter> DeviceAdapterRef;
struct IDevice;
typedef ::k3d::SharedPtr<IDevice> DeviceRef;
struct IFactory;
typedef ::k3d::SharedPtr<IFactory> FactoryRef;
struct IGpuResource;
typedef ::k3d::SharedPtr<IGpuResource> GpuResourceRef;
struct ITexture;
typedef ::k3d::SharedPtr<ITexture> TextureRef;
struct IBuffer;
typedef ::k3d::SharedPtr<IBuffer> BufferRef;
struct ITextureView;
typedef ::k3d::SharedPtr<ITextureView> TextureViewRef;
struct IBufferView;
typedef ::k3d::SharedPtr<IBufferView> BufferViewRef;
struct IShaderResourceView;
typedef ::k3d::SharedPtr<IShaderResourceView> ShaderResourceViewRef;
struct IPipelineState;
typedef ::k3d::SharedPtr<IPipelineState> PipelineStateRef;
struct IRenderPipelineState;
typedef ::k3d::SharedPtr<IRenderPipelineState> RenderPipelineStateRef;
struct IComputePipelineState;
typedef ::k3d::SharedPtr<IComputePipelineState> ComputePipelineStateRef;
struct IRenderPipelineEncoder;
typedef ::k3d::SharedPtr<IRenderPipelineEncoder> RenderPipelineEncoderRef;
struct IComputePipelineEncoder;
typedef ::k3d::SharedPtr<IComputePipelineEncoder> ComputePipelineEncoderRef;
struct ISwapChain;
typedef ::k3d::SharedPtr<ISwapChain> SwapChainRef;
K3D_DEPRECATED("Use Swapchain Instead")
struct IRenderViewport;
typedef ::k3d::SharedPtr<IRenderViewport> RenderViewportRef;
struct IShaderBytes;
struct ISampler;
typedef ::k3d::SharedPtr<ISampler> SamplerRef;
typedef const SamplerRef SamplerCRef;
struct IDescriptor;
typedef ::k3d::SharedPtr<IDescriptor> DescriptorRef;
struct IRenderTarget;
typedef ::k3d::SharedPtr<IRenderTarget> RenderTargetRef;
struct IRenderPass;
typedef ::k3d::SharedPtr<IRenderPass> RenderPassRef;
/**
 * Semaphore used for GPU to GPU syncs,
 * Specifically used to sync queue submissions (on the same or different
 * queues), Set on GPU, Wait on GPU (inter-queue) Events: Set anywhere, Wait on
 * GPU (intra-queue)
 */
struct ISemaphore;

/**
 * @see
 * https://www.reddit.com/r/vulkan/comments/47tc3s/differences_between_vkfence_vkevent_and/?
 * Fence is GPU to CPU syncs
 * Set on GPU, Wait on CPU
 */
struct ISyncFence;
typedef ::k3d::SharedPtr<ISyncFence> SyncFenceRef;

/**
 * Vulkan has pipeline layout,
 * D3D12 is root signature.
 * PipelineLayout defines shaders impose constraints on table layout.
 */
struct IPipelineLayout;
typedef ::k3d::SharedPtr<IPipelineLayout> PipelineLayoutRef;

struct IGpuResource
{
  virtual ~IGpuResource() {}
  virtual void* Map(uint64 start, uint64 size) = 0;
  virtual void UnMap() = 0;

  virtual uint64 GetLocation() const { return 0; }
  virtual ResourceDesc GetDesc() const = 0;

  /**
   * Vulkan: texture uses image layout as resource state
   * D3D12: used for transition, maybe used as ShaderVisiblity determination in
   * STATIC_SAMPLER and descriptor table
   */
  virtual EResourceState GetState() const { return ERS_Unknown; }
  virtual uint64 GetSize() const = 0;
};

struct ISampler
{
  virtual SamplerState GetSamplerDesc() const = 0;
  virtual ~ISampler() {}
};

struct IShaderResourceView
{
  virtual ~IShaderResourceView() {}
  virtual GpuResourceRef GetResource() const = 0;
  virtual ResourceViewDesc GetDesc() const = 0;
};

struct ITexture : public IGpuResource
{
  virtual ~ITexture() {}
  virtual SamplerCRef GetSampler() const = 0;
  virtual void BindSampler(SamplerRef) = 0;
  virtual void SetResourceView(ShaderResourceViewRef) = 0;
  virtual ShaderResourceViewRef GetResourceView() const = 0;
};

struct IBuffer : public IGpuResource
{
  virtual ~IBuffer() {}
};

struct IDescriptor
{
  virtual void Update(uint32 bindSet, GpuResourceRef) = 0;
  virtual void Update(uint32 bindSet, SamplerRef){};
  virtual uint32 GetSlotNum() const { return 0; }
  virtual ~IDescriptor() {}
};

struct IDeviceAdapter
{
  virtual DeviceRef GetDevice() = 0;
  virtual ~IDeviceAdapter() {}
};

/**
 * Vulkan has pipeline layout,
 * D3D12 is root signature.
 * PipelineLayout defines shaders impose constraints on table layout.
 */
struct IPipelineLayout
{
  virtual DescriptorRef GetDescriptorSet() const = 0;
  virtual ~IPipelineLayout() {}
};

/**
 * @see
 * https://www.reddit.com/r/vulkan/comments/47tc3s/differences_between_vkfence_vkevent_and/?
 * Fence is GPU to CPU syncs
 * Set on GPU, Wait on CPU
 */
struct ISyncFence
{
  virtual void Signal(int32 fenceVal) = 0;
  virtual void Reset() {}
  virtual void WaitFor(uint64 time) = 0;
  virtual ~ISyncFence() {}
};

struct RenderTargetLayout
{
  struct Attachment
  {
    int Binding = -1;
    EPixelFormat Format = EPF_RGBA8Unorm;
  };
  ::k3d::DynArray<Attachment> Attachments;
  bool HasDepthStencil;
  EPixelFormat DepthStencilFormat;
};

struct IRenderTarget
{
  virtual ~IRenderTarget() {}
  virtual void SetClearColor(kMath::Vec4f clrColor) = 0;
  virtual void SetClearDepthStencil(float depth, uint32 stencil) = 0;
  virtual GpuResourceRef GetBackBuffer() = 0;
};

struct RenderPipelineStateDesc
{
  RenderPipelineStateDesc() {}
  RasterizerState Rasterizer;
  BlendState Blend;
  DepthStencilState DepthStencil;
  VertexInputState InputState;
  // InputAssemblyState
  EPrimitiveType PrimitiveTopology /* = rhi::EPT_Triangles */;
  // Tessellation Patch
  uint32 PatchControlPoints /* = 0*/;

  ShaderBundle VertexShader;
  ShaderBundle PixelShader;
  ShaderBundle GeometryShader;
  ShaderBundle DomainShader;
  ShaderBundle HullShader;
};

struct ComputePipelineStateDesc
{
  ShaderBundle ComputeShader;
};

struct IPipelineState
{
  virtual ~IPipelineState() {}
  virtual EPipelineType GetType() const = 0;

  // rebuild pipeline state if is dirty
  virtual void Rebuild() = 0;

  // storage options
  virtual void SavePSO(::k3d::String const& Path) {}
  virtual void LoadPSO(::k3d::String const& Path) {}
};

struct IRenderPipelineState : public IPipelineState
{
  virtual ~IRenderPipelineState() {}

  virtual void SetRasterizerState(const RasterizerState&) = 0;
  virtual void SetBlendState(const BlendState&) = 0;
  virtual void SetDepthStencilState(const DepthStencilState&) = 0;
  virtual void SetPrimitiveTopology(const EPrimitiveType) = 0;
  virtual void SetVertexInputLayout(rhi::VertexDeclaration const*,
                                    uint32 Count) = 0;
  virtual void SetRenderTargetFormat(const RenderTargetFormat&) = 0;
  virtual void SetSampler(SamplerRef) = 0;
};

struct IComputePipelineState : public IPipelineState
{
  virtual ~IComputePipelineState() {}
};

#if 0
struct IPipelineEncoder
{
  virtual PipelineStateRef GetResult() = 0;
  virtual IPipelineEncoder& SetPipelineLayout(PipelineLayoutRef const&) = 0;
};

struct IRenderPipelineEncoder : public IPipelineEncoder
{
  virtual void SetVertexShader(ShaderBundle const&) = 0;
  virtual void SetPixelShader(ShaderBundle const&) = 0;
  virtual void SetGeometryShader(ShaderBundle const&) = 0;
  virtual void SetDomainShader(ShaderBundle const&) = 0;
  virtual void SetHullShader(ShaderBundle const&) = 0;
  virtual void SetRenderPass(RenderPassRef const&) = 0;
  virtual void SetRasterizerState(const RasterizerState&) = 0;
  virtual void SetBlendState(const BlendState&) = 0;
  virtual void SetDepthStencilState(const DepthStencilState&) = 0;
  virtual void SetPrimitiveTopology(const EPrimitiveType&) = 0;
  virtual void SetVertexInputLayout(rhi::VertexInputState const&) = 0;
  virtual void SetRenderTargetFormat(const RenderTargetFormat&) = 0;
  virtual void SetSampler(SamplerRef) = 0;
};

struct IComputePipelineEncoder : public IPipelineEncoder
{
  virtual IComputePipelineEncoder& SetComputeShader(ShaderBundle const&) = 0;
};
#endif

using PipelineLayoutDesc = shc::BindingTable;

struct PipelineLayoutKey
{
  uint32 BindingKey = 0;
  uint32 SetKey = 0;
  uint32 UniformKey = 0;
  bool operator==(PipelineLayoutKey const& rhs)
  {
    return BindingKey == rhs.BindingKey && SetKey == rhs.SetKey &&
           UniformKey == rhs.UniformKey;
  }
  bool operator<(PipelineLayoutKey const& rhs) const
  {
    return BindingKey < rhs.BindingKey || SetKey < rhs.SetKey ||
           UniformKey < rhs.UniformKey;
  }
};

struct IDevice : public IObject
{
  enum Result
  {
    DeviceNotFound,
    DeviceFound
  };

  virtual ~IDevice() {}
#if 0
  K3D_DEPRECATED("Use CommandQueue & CommandBuffer Instead")
  virtual CommandContextRef NewCommandContext(ECommandType) = 0;
#endif
  virtual GpuResourceRef NewGpuResource(ResourceDesc const&) = 0;
  virtual ShaderResourceViewRef NewShaderResourceView(
    GpuResourceRef,
    ResourceViewDesc const&) = 0;
  virtual SamplerRef NewSampler(const SamplerState&) = 0;

  virtual PipelineLayoutRef NewPipelineLayout(
    PipelineLayoutDesc const& table) = 0;
  virtual SyncFenceRef CreateFence() = 0;
#if 0
  K3D_DEPRECATED("Use Swapchain Instead")
  virtual RenderViewportRef NewRenderViewport(void* winHandle,
                                              RenderViewportDesc&) = 0;
#endif
  virtual RenderTargetRef NewRenderTarget(RenderTargetLayout const&) = 0;

  virtual PipelineStateRef CreateRenderPipelineState(
    RenderPipelineStateDesc const&,
    PipelineLayoutRef) = 0;

  virtual PipelineStateRef CreateComputePipelineState(
    ComputePipelineStateDesc const&,
    PipelineLayoutRef) = 0;

  virtual CommandQueueRef CreateCommandQueue(ECommandType const&) = 0;

  virtual void WaitIdle() {}

  /* equal with d3d12's getcopyfootprint or vulkan's getImagesubreslayout.
   */
  virtual void QueryTextureSubResourceLayout(TextureRef,
                                             TextureResourceSpec const& spec,
                                             SubResourceLayout*)
  {
  }
};

struct IFactory : public IObject
{
  virtual void EnumDevices(k3d::DynArray<DeviceRef>& Devices) = 0;
  virtual SwapChainRef CreateSwapchain(CommandQueueRef pCommandQueue,
                                       void* nPtr,
                                       SwapChainDesc&) = 0;
};

struct ISwapChain : public IObject
{
  virtual void Resize(uint32 Width, uint32 Height) = 0;
  virtual void Present() = 0;
  virtual TextureRef GetCurrentTexture() = 0;
};

#if 0
K3D_DEPRECATED("Use Swapchain Instead")
struct IRenderViewport
{
  virtual ~IRenderViewport() {}

  virtual bool InitViewport(void* windowHandle,
                            IDevice* pDevice,
                            RenderViewportDesc&) = 0;

  virtual void PrepareNextFrame() {}

  /**
   * @param	vSync	true to synchronise.
   * @return	true if it succeeds, false if it fails.
   */
  virtual bool Present(bool vSync) = 0;

  virtual RenderTargetRef GetRenderTarget(uint32 index) = 0;
  virtual RenderTargetRef GetCurrentBackRenderTarget() = 0;

  virtual uint32 GetSwapChainCount() = 0;
  virtual uint32 GetSwapChainIndex() = 0;

  virtual uint32 GetWidth() const = 0;
  virtual uint32 GetHeight() const = 0;
};
#endif

struct TextureCopyLocation
{
  typedef ::k3d::DynArray<uint32> ResIds;
  typedef ::k3d::DynArray<PlacedSubResourceFootprint> ResFootprints;

  enum ESubResource
  {
    ESubResourceIndex,
    ESubResourceFootPrints
  };

  TextureCopyLocation(GpuResourceRef ptrResource, ResIds subResourceIndex)
    : pResource(ptrResource)
    , SubResourceIndexes(subResourceIndex)
  {
  }

  TextureCopyLocation(GpuResourceRef ptrResource, ResFootprints footprints)
    : pResource(ptrResource)
    , SubResourceFootPrints(footprints)
  {
  }

  TextureCopyLocation() = default;

  ~TextureCopyLocation() {}

  GpuResourceRef pResource;
  ESubResource SubResourceType = ESubResourceIndex;
  ResIds SubResourceIndexes;
  ResFootprints SubResourceFootPrints;
};

struct ICommandQueue
{
  virtual CommandBufferRef ObtainCommandBuffer(ECommandUsageType const&) = 0;
};

struct ICommandBuffer : public IObject
{
  virtual void Commit() = 0;
  // For Vulkan, This command will be appended to the tail
  virtual void Present(SwapChainRef pSwapChain, SyncFenceRef pFence) = 0;
  virtual void Reset() = 0;
  virtual RenderCommandEncoderRef RenderCommandEncoder(
    RenderTargetRef const&,
    RenderPipelineStateRef const&) = 0;
  virtual ComputeCommandEncoderRef ComputeCommandEncoder(
    ComputePipelineStateRef const&) = 0;
  virtual ParallelRenderCommandEncoderRef ParallelRenderCommandEncoder(
    RenderTargetRef const&,
    RenderPipelineStateRef const&) = 0;
  // blit 
  virtual void CopyTexture(const TextureCopyLocation& Dest,
                           const TextureCopyLocation& Src) = 0;
  //
  virtual void CopyBuffer(GpuResourceRef Dest,
                          GpuResourceRef Src,
                          CopyBufferRegion const& Region) = 0;
  virtual void Transition(GpuResourceRef pResource, rhi::EResourceState const& State/*, rhi::EPipelineStage const& Stage*/) = 0;
};

struct ICommandEncoder
{
  virtual void SetPipelineState(uint32 HashCode, PipelineStateRef const&) = 0;
  virtual void SetPipelineLayout(PipelineLayoutRef const&) = 0;
  virtual void EndEncode() = 0;
};

struct IRenderCommandEncoder : public ICommandEncoder
{
  virtual void SetScissorRect(const Rect&) = 0;
  virtual void SetViewport(const ViewportDesc&) = 0;
  virtual void SetIndexBuffer(const IndexBufferView& IBView) = 0;
  virtual void SetVertexBuffer(uint32 Slot, const VertexBufferView& VBView) = 0;
  virtual void SetPrimitiveType(EPrimitiveType) = 0;
  virtual void DrawInstanced(DrawInstancedParam) = 0;
  virtual void DrawIndexedInstanced(DrawIndexedInstancedParam) = 0;
};

struct IComputeCommandEncoder : public ICommandEncoder
{
  virtual void Dispatch(uint32 GroupCountX,
                        uint32 GroupCountY,
                        uint32 GroupCountZ) = 0;
};
//Begin with a single rendering pass, encoded from multiple threads simultaneously.
struct IParallelRenderCommandEncoder : public ICommandEncoder
{
  virtual RenderCommandEncoderRef SubRenderCommandEncoder() = 0;
};

#if 0
K3D_DEPRECATED("Use Command Buffer And Command Encoder Instead")
struct ICommandContext
{
  virtual ~ICommandContext() {}

  virtual void Detach(IDevice*) = 0;

  /**
   * Like D3D12 Do
   */
  virtual void CopyTexture(const TextureCopyLocation& Dest,
                           const TextureCopyLocation& Src) = 0;

  virtual void CopyBuffer(IGpuResource& Dest,
                          IGpuResource& Src,
                          CopyBufferRegion const& Region) = 0;
  virtual void Execute(bool Wait) = 0;
  virtual void Reset() = 0;
  virtual void TransitionResourceBarrier(
    GpuResourceRef resource,
    /*EPipelineStage stage,*/ EResourceState dstState) = 0;

  virtual void Begin() {}
  virtual void End() {}
  virtual void PresentInViewport(RenderViewportRef) = 0;

  virtual void ClearColorBuffer(GpuResourceRef, kMath::Vec4f const&) = 0;

  virtual void BeginRendering() = 0;
  virtual void SetRenderTarget(RenderTargetRef) = 0;
  virtual void SetScissorRects(uint32, const Rect*) = 0;
  virtual void SetViewport(const ViewportDesc&) = 0;
  virtual void SetIndexBuffer(const IndexBufferView& IBView) = 0;
  virtual void SetVertexBuffer(uint32 Slot, const VertexBufferView& VBView) = 0;
  virtual void SetPipelineState(uint32 HashCode, PipelineStateRef const&) = 0;
  virtual void SetPipelineLayout(PipelineLayoutRef) = 0;
  virtual void SetPrimitiveType(EPrimitiveType) = 0;
  virtual void DrawInstanced(DrawInstancedParam) = 0;
  virtual void DrawIndexedInstanced(DrawIndexedInstancedParam) = 0;
  virtual void EndRendering() = 0;

  virtual void Dispatch(uint32 GroupCountX,
                        uint32 GroupCountY,
                        uint32 GroupCountZ) = 0;

  virtual void ExecuteBundle(ICommandContext*) {}
};
#endif
}
