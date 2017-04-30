#ifndef __VkRHI_h__
#define __VkRHI_h__
#pragma once
#include "VkObjects.h"
#include <Core/Os.h>
#include <list>
#include <tuple>

K3D_VK_BEGIN

class Device;
using VkDeviceRef = SharedPtr<Device>;
using RefDevice = std::shared_ptr<Device>;

class ResourceManager;
using PtrResManager = std::unique_ptr<ResourceManager>;

class CommandAllocator;
using PtrCmdAlloc = std::shared_ptr<CommandAllocator>;

class Semaphore;
using PtrSemaphore = std::shared_ptr<Semaphore>;

class Fence;
using PtrFence = SharedPtr<Fence>;

class CommandContext;
using PtrContext = std::shared_ptr<CommandContext>;
using PtrContextList = std::list<PtrContext>;

class CommandContextPool;

class SwapChain;
using SwapChainRef = SharedPtr<SwapChain>;
using PtrSwapChain = std::unique_ptr<SwapChain>;

class CommandQueue;
using SpCmdQueue = SharedPtr<CommandQueue>;

class CommandBuffer;
using SpCmdBuffer = SharedPtr<CommandBuffer>;

class PipelineLayout;
class DescriptorAllocator;
class DescriptorSetLayout;

class FrameBuffer;
using SpFramebuffer = SharedPtr<FrameBuffer>;

class RenderPass;
using SpRenderpass = SharedPtr<RenderPass>;
using UpRenderpass = std::unique_ptr<RenderPass>;

class Texture;
using SpTexture = SharedPtr<Texture>;

class RenderTarget;
using SpRenderTarget = SharedPtr<RenderTarget>;

class RenderViewport;
class Sampler;

class ShaderResourceView;
using BindingArray = DynArray<VkDescriptorSetLayoutBinding>;

class DescriptorAllocator;
using DescriptorAllocRef = SharedPtr<DescriptorAllocator>;

class DescriptorSetLayout;
using DescriptorSetLayoutRef = SharedPtr<DescriptorSetLayout>;

class DescriptorSet;
using DescriptorSetRef = SharedPtr<DescriptorSet>;

class RenderViewport;
using RenderViewportSp = SharedPtr<RenderViewport>;

typedef std::map<rhi::PipelineLayoutKey, rhi::PipelineLayoutRef>
  MapPipelineLayout;
typedef std::unordered_map<uint32, DescriptorSetLayoutRef>
  MapDescriptorSetLayout;
typedef std::unordered_map<uint32, DescriptorAllocRef> MapDescriptorAlloc;

template<class TVkResObj>
struct ResTrait
{
};

template<>
struct ResTrait<VkBuffer>
{
  typedef VkBufferView View;
  typedef VkBufferUsageFlags UsageFlags;
  typedef VkBufferCreateInfo CreateInfo;
  typedef VkBufferViewCreateInfo ViewCreateInfo;
  typedef VkDescriptorBufferInfo DescriptorInfo;
  static decltype(vkCreateBufferView)* CreateView;
  static decltype(vkDestroyBufferView)* DestroyView;
  static decltype(vkCreateBuffer)* Create;
  static decltype(vkDestroyBuffer)* Destroy;
  static decltype(vkGetBufferMemoryRequirements)* GetMemoryInfo;
  static decltype(vkBindBufferMemory)* BindMemory;
};

template<>
struct ResTrait<VkImage>
{
  typedef VkImageView View;
  typedef VkImageUsageFlags UsageFlags;
  typedef VkImageCreateInfo CreateInfo;
  typedef VkImageViewCreateInfo ViewCreateInfo;
  typedef VkDescriptorImageInfo DescriptorInfo;
  static decltype(vkCreateImageView)* CreateView;
  static decltype(vkDestroyImageView)* DestroyView;
  static decltype(vkCreateImage)* Create;
  static decltype(vkDestroyImage)* Destroy;
  static decltype(vkGetImageMemoryRequirements)* GetMemoryInfo;
  static decltype(vkBindImageMemory)* BindMemory;
};

struct RHIRoot
{
  static void AddViewport(RenderViewportSp);
  static RenderViewportSp GetViewport(int index);

private:
  static RenderViewportSp s_Vp;

  static void EnumLayersAndExts();
  friend class Device;
};

class Device
  : public rhi::IDevice
  , public k3d::EnableSharedFromThis<Device>
{
public:
  typedef k3d::SharedPtr<Device> Ptr;

  Device();
  ~Device() override;
  void Release() override;
#if 0
  rhi::CommandContextRef NewCommandContext(rhi::ECommandType) override;
#endif
  rhi::GpuResourceRef NewGpuResource(rhi::ResourceDesc const&) override;

  rhi::ShaderResourceViewRef NewShaderResourceView(
    rhi::GpuResourceRef,
    rhi::ResourceViewDesc const&) override;

  rhi::SamplerRef NewSampler(const rhi::SamplerState&) override;

  rhi::PipelineLayoutRef NewPipelineLayout(
    rhi::PipelineLayoutDesc const& table) override;

  rhi::PipelineStateRef CreateRenderPipelineState(
    rhi::RenderPipelineStateDesc const&,
    rhi::PipelineLayoutRef) override;

  rhi::PipelineStateRef CreateComputePipelineState(
    rhi::ComputePipelineStateDesc const&,
    rhi::PipelineLayoutRef) override;

  rhi::SyncFenceRef CreateFence() override;

  rhi::CommandQueueRef CreateCommandQueue(rhi::ECommandType const&) override;
#if 0
  rhi::RenderViewportRef NewRenderViewport(void* winHandle,
                                           rhi::RenderViewportDesc&) override;
#endif
  rhi::RenderTargetRef NewRenderTarget(
    rhi::RenderTargetLayout const& layout) override;

  void WaitIdle() override { vkDeviceWaitIdle(m_Device); }

  void QueryTextureSubResourceLayout(rhi::TextureRef,
                                     rhi::TextureResourceSpec const& spec,
                                     rhi::SubResourceLayout*) override;

  VkDevice const& GetRawDevice() const { return m_Device; }
  // PtrResManager const &		GetMemoryManager() const { return
  // m_ResourceManager; }

  PtrCmdAlloc NewCommandAllocator(bool transient);
  bool FindMemoryType(uint32 typeBits,
                      VkFlags requirementsMask,
                      uint32* typeIndex) const;
  PtrSemaphore NewSemaphore();

  uint32 GetQueueCount() const { return m_Gpu->m_QueueProps.Count(); }

  SpRenderpass const& GetTopPass() const { return m_PendingPass.back(); }

  void PushRenderPass(SpRenderpass renderPass)
  {
    m_PendingPass.push_back(renderPass);
  }

  DescriptorAllocRef NewDescriptorAllocator(uint32 maxSets,
                                            BindingArray const& bindings);
  DescriptorSetLayoutRef NewDescriptorSetLayout(BindingArray const& bindings);

  uint64 GetMaxAllocationCount();
  VkShaderModule CreateShaderModule(rhi::ShaderBundle const& Bundle);

  friend class DeviceChild;

protected:
  SpCmdQueue InitCmdQueue(VkQueueFlags queueTypes,
                          uint32 queueFamilyIndex,
                          uint32 queueIndex);

  Result Create(GpuRef const& gpu, bool withDebug);

  friend class Instance;
  friend class SwapChain;

private:
  CmdBufManagerRef m_CmdBufManager;
  // PtrResManager
  // m_ResourceManager;  std::unique_ptr<CommandContextPool>
  // m_ContextPool;
  std::vector<SpRenderpass> m_PendingPass;

  // MapPipelineLayout
  // m_CachedPipelineLayout;  MapDescriptorAlloc
  // m_CachedDescriptorPool;  MapDescriptorSetLayout
  // m_CachedDescriptorSetLayout;

private:
  VkPhysicalDeviceMemoryProperties m_MemoryProperties = {};
  VkDevice m_Device = VK_NULL_HANDLE;
  GpuRef m_Gpu;
};

#define __VK_GLOBAL_LEVEL_PROC__(name) PFN_vk##name gp##name = NULL;
#define __VK_INSTANCE_LEVEL_PROC__(name) PFN_vk##name fp##name = NULL

class Instance
  : public EnableSharedFromThis<Instance>
  , public rhi::IFactory
{
public:
  Instance(const ::k3d::String& engineName,
           const ::k3d::String& appName,
           bool enableValidation = true);
  ~Instance();

  void Release() override;
  // IFactory::EnumDevices
  void EnumDevices(DynArray<rhi::DeviceRef>& Devices) override;
  // IFactory::CreateSwapChain
  rhi::SwapChainRef CreateSwapchain(rhi::CommandQueueRef pCommandQueue,
                                    void* nPtr,
                                    rhi::SwapChainDesc&) override;

  void SetupDebugging(VkDebugReportFlagsEXT flags,
                      PFN_vkDebugReportCallbackEXT callBack);
  void FreeDebugCallback();

  bool WithValidation() const { return m_EnableValidation; }

  ::k3d::DynArray<GpuRef> EnumGpus();

#ifdef VK_NO_PROTOTYPES
  VkResult CreateSurfaceKHR(const
#if K3DPLATFORM_OS_WIN
                            VkWin32SurfaceCreateInfoKHR
#elif K3DPLATFORM_OS_ANDROID
                            VkAndroidSurfaceCreateInfoKHR
#endif
                              * pCreateInfo,
                            const VkAllocationCallbacks* pAllocator,
                            VkSurfaceKHR* pSurface)
  {
#if K3DPLATFORM_OS_WIN
    return fpCreateWin32SurfaceKHR(
      m_Instance, pCreateInfo, pAllocator, pSurface);
#elif K3DPLATFORM_OS_ANDROID
    return fpCreateAndroidSurfaceKHR(
      m_Instance, pCreateInfo, pAllocator, pSurface);
#endif
  }

  void DestroySurfaceKHR(VkSurfaceKHR surface,
                         VkAllocationCallbacks* pAllocator)
  {
    fpDestroySurfaceKHR(m_Instance, surface, pAllocator);
  }
#endif

  friend class Gpu;
  friend class Device;
  friend class SwapChain;
  friend struct RHIRoot;

private:
  void LoadGlobalProcs();
  void EnumExtsAndLayers();
  void ExtractEnabledExtsAndLayers();
  void LoadInstanceProcs();

  bool m_EnableValidation;
  ::k3d::DynArray<::k3d::String> m_EnabledExts;
  ::k3d::DynArray<char*> m_EnabledExtsRaw;
  ::k3d::DynArray<::k3d::String> m_EnabledLayers;
  ::k3d::DynArray<const char*> m_EnabledLayersRaw;

  VkInstance m_Instance;
  VkDebugReportCallbackEXT m_DebugMsgCallback;

#ifdef VK_NO_PROTOTYPES

  PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
    fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
    fpGetPhysicalDeviceSurfacePresentModesKHR;

  __VK_GLOBAL_LEVEL_PROC__(CreateInstance);
  __VK_GLOBAL_LEVEL_PROC__(EnumerateInstanceExtensionProperties);
  __VK_GLOBAL_LEVEL_PROC__(EnumerateInstanceLayerProperties);
  __VK_GLOBAL_LEVEL_PROC__(GetInstanceProcAddr);
  __VK_GLOBAL_LEVEL_PROC__(DestroyInstance);

  __VK_INSTANCE_LEVEL_PROC__(CreateDebugReportCallbackEXT);
  __VK_INSTANCE_LEVEL_PROC__(DestroyDebugReportCallbackEXT);

#if K3DPLATFORM_OS_WIN
  __VK_INSTANCE_LEVEL_PROC__(CreateWin32SurfaceKHR);
#elif K3DPLATFORM_OS_ANDROID
  __VK_INSTANCE_LEVEL_PROC__(CreateAndroidSurfaceKHR);
#endif
  __VK_INSTANCE_LEVEL_PROC__(DestroySurfaceKHR);

  __VK_INSTANCE_LEVEL_PROC__(EnumeratePhysicalDevices);
  __VK_INSTANCE_LEVEL_PROC__(GetPhysicalDeviceProperties);
  __VK_INSTANCE_LEVEL_PROC__(GetPhysicalDeviceFeatures);
  __VK_INSTANCE_LEVEL_PROC__(GetPhysicalDeviceMemoryProperties);
  __VK_INSTANCE_LEVEL_PROC__(GetPhysicalDeviceQueueFamilyProperties);
  __VK_INSTANCE_LEVEL_PROC__(GetPhysicalDeviceFormatProperties);
  __VK_INSTANCE_LEVEL_PROC__(CreateDevice);
  __VK_INSTANCE_LEVEL_PROC__(GetDeviceProcAddr);

  dynlib::LibRef m_VulkanLib;
#endif
};

class DeviceChild
{
public:
  explicit DeviceChild(Device::Ptr pDevice)
    : m_pDevice(pDevice)
  {
  }
  virtual ~DeviceChild() {}

  VkDevice const& GetRawDevice() const { return m_pDevice->GetRawDevice(); }
  GpuRef GetGpuRef() const { return m_pDevice->m_Gpu; }

  Device::Ptr const GetDevice() const { return m_pDevice; }

private:
  Device::Ptr m_pDevice;
};

template<class TVkObj, class TRHIObj>
class TVkRHIObjectBase
  : public RHIRoot
  , public TRHIObj
{
public:
  using ThisObj = TVkRHIObjectBase<TVkObj, TRHIObj>;

  explicit TVkRHIObjectBase(VkDeviceRef const& refDevice)
    : m_Device(refDevice)
    , m_NativeObj(VK_NULL_HANDLE)
  {
  }

  virtual ~TVkRHIObjectBase() {}

  SharedPtr<Device> SharedDevice() { return m_Device; }

  TVkObj NativeHandle() const { return m_NativeObj; }
  VkDevice NativeDevice() const { return m_Device->GetRawDevice(); }

protected:
  SharedPtr<Device> m_Device;
  TVkObj m_NativeObj;
};

#define DEVICE_CHILD_CONSTRUCT(className)                                      \
  explicit className(Device::Ptr ptr)                                          \
    : DeviceChild(ptr)                                                         \
  {                                                                            \
  }
#define DEVICE_CHILD_CONSTRUCT_DECLARE(className)                              \
  explicit className(Device::Ptr ptr = nullptr);

/**
 * Fences are signaled by the system when work invoked by vkQueueSubmit
 * completes.
 */
class Fence : public TVkRHIObjectBase<VkFence, rhi::ISyncFence>
{
public:
  Fence(Device::Ptr pDevice,
        VkFenceCreateInfo const& info = FenceCreateInfo::Create())
    : ThisObj(pDevice)
  {
    if (pDevice) {
      K3D_VK_VERIFY(
        vkCreateFence(NativeDevice(), &info, nullptr, &m_NativeObj));
    }
  }

  ~Fence() override
  {
    if (m_NativeObj) {
      vkDestroyFence(NativeDevice(), m_NativeObj, nullptr);
      VKLOG(Info, "Fence Destroyed. -- %0x.", m_NativeObj);
      m_NativeObj = VK_NULL_HANDLE;
    }
  }

  void Signal(int32 val) override {}

  bool IsSignaled()
  {
    return VK_SUCCESS == vkGetFenceStatus(NativeDevice(), m_NativeObj);
  }

  void Reset() override { vkResetFences(NativeDevice(), 1, &m_NativeObj); }

  void WaitFor(uint64 time) override
  {
    vkWaitForFences(NativeDevice(), 1, &m_NativeObj, VK_TRUE, time);
  }

private:
  friend class SwapChain;
  friend class CommandContext;
};

class Sampler : public TVkRHIObjectBase<VkSampler, rhi::ISampler>
{
public:
  explicit Sampler(Device::Ptr pDevice, rhi::SamplerState const& sampleDesc);
  ~Sampler() override;
  rhi::SamplerState GetSamplerDesc() const;

protected:
  VkSamplerCreateInfo m_SamplerCreateInfo = {};
  rhi::SamplerState m_SamplerState;
};

/**
 * @param binding
 */
VkDescriptorSetLayoutBinding
RHIBinding2VkBinding(rhi::shc::Binding const& binding);
/**
 * Vulkan has DescriptorPool, DescriptorSet, DescriptorSetLayout and
 * PipelineLayout DescriptorSet is allocated from DescriptorPool with
 * DescriptorSetLayout, PipelineLayout depends on DescriptorSetLayout,
 */
class DescriptorAllocator : public DeviceChild
{
public:
  struct Options
  {
    VkDescriptorPoolCreateFlags CreateFlags = 0;
    VkDescriptorPoolResetFlags ResetFlags = 0;
  };

  /**
   * @param maxSets
   * @param bindings
   */
  explicit DescriptorAllocator(Device::Ptr pDevice,
                               Options const& option,
                               uint32 maxSets,
                               BindingArray const& bindings);
  ~DescriptorAllocator();

protected:
  void Initialize(uint32 maxSets, BindingArray const& bindings);
  void Destroy();

private:
  friend class DescriptorSet;

  Options m_Options;
  VkDescriptorPool m_Pool;
};

class DescriptorSetLayout : public DeviceChild
{
public:
  DescriptorSetLayout(Device::Ptr pDevice, BindingArray const& bindings);
  ~DescriptorSetLayout();

  VkDescriptorSetLayout GetNativeHandle() const
  {
    return m_DescriptorSetLayout;
  }

protected:
  void Initialize(BindingArray const& bindings);
  void Destroy();

private:
  friend class PipelineLayout;
  VkDescriptorSetLayout m_DescriptorSetLayout;
  std::vector<VkWriteDescriptorSet> m_UpdateDescSet;
};

class DescriptorSet
  : public DeviceChild
  , public rhi::IDescriptor
{
public:
  static DescriptorSet* CreateDescSet(DescriptorAllocRef descriptorPool,
                                      VkDescriptorSetLayout layout,
                                      BindingArray const& bindings,
                                      Device::Ptr pDevice);
  virtual ~DescriptorSet();
  void Update(uint32 bindSet, rhi::SamplerRef) override;
  void Update(uint32 bindSet, rhi::GpuResourceRef) override;
  uint32 GetSlotNum() const override;
  VkDescriptorSet GetNativeHandle() const { return m_DescriptorSet; }

private:
  DescriptorSet(DescriptorAllocRef descriptorPool,
                VkDescriptorSetLayout layout,
                BindingArray const& bindings,
                Device::Ptr pDevice);

  VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
  DescriptorAllocRef m_DescriptorAllocator = nullptr;
  BindingArray m_Bindings;
  std::vector<VkWriteDescriptorSet> m_BoundDescriptorSet;

  void Initialize(VkDescriptorSetLayout layout, BindingArray const& bindings);
  void Destroy();
};

class Resource
  : virtual public rhi::IGpuResource
  , public DeviceChild
{
public:
  typedef void* Ptr;
  typedef void const* CPtr;

  explicit Resource(Device::Ptr pDevice)
    : DeviceChild(pDevice)
    , m_HostMem(nullptr)
    , m_DeviceMem{}
    , m_Desc{}
  {
  }
  Resource(Device::Ptr pDevice, rhi::ResourceDesc const& desc)
    : DeviceChild(pDevice)
    , m_HostMem(nullptr)
    , m_DeviceMem{}
    , m_Desc(desc)
  {
  }
  virtual ~Resource();

  Resource::CPtr GetHostMemory(uint64 OffSet) const { return m_HostMem; }
  VkDeviceMemory GetDeviceMemory() const { return m_DeviceMem; }

  Resource::Ptr Map(uint64 offset, uint64 size) override;
  void UnMap() override { vkUnmapMemory(GetRawDevice(), m_DeviceMem); }
  uint64 GetSize() const override { return m_Size; }
  rhi::ResourceDesc GetDesc() const override { return m_Desc; }

protected:
  VkMemoryAllocateInfo m_MemAllocInfo;
  VkDeviceMemory m_DeviceMem;
  Resource::Ptr m_HostMem;

  VkDeviceSize m_Size = 0;
  VkDeviceSize m_AllocationSize = 0;
  VkDeviceSize m_AllocationOffset = 0;

  rhi::ResourceDesc m_Desc;
};

template<class TVkResObj, class TRHIResObj>
class TResource : public TVkRHIObjectBase<TVkResObj, TRHIResObj>
{
public:
  using ThisResourceType = TResource<TVkResObj, TRHIResObj>;
  using TVkRHIObjectBase<TVkResObj, TRHIResObj>::m_NativeObj;
  using TVkRHIObjectBase<TVkResObj, TRHIResObj>::NativeDevice;
  using TVkRHIObjectBase<TVkResObj, TRHIResObj>::m_Device;

  explicit TResource(VkDeviceRef const& refDevice, bool SelfOwned = true)
    : TVkRHIObjectBase<TVkResObj, TRHIResObj>(refDevice)
    , m_MemAllocInfo{}
    , m_HostMemAddr(nullptr)
    , m_DeviceMem{ VK_NULL_HANDLE }
    , m_ResView(VK_NULL_HANDLE)
    , m_ResDesc{}
    , m_SelfOwned(SelfOwned)
  {
  }

  TResource(VkDeviceRef const& refDevice, rhi::ResourceDesc const& desc)
    : TVkRHIObjectBase<TVkResObj, TRHIResObj>(refDevice)
    , m_MemAllocInfo{}
    , m_HostMemAddr(nullptr)
    , m_DeviceMem{}
    , m_ResView(VK_NULL_HANDLE)
    , m_ResDesc(desc)
  {
  }

  virtual ~TResource()
  {
    if (VK_NULL_HANDLE != m_ResView) {
      ResTrait<TVkResObj>::DestroyView(NativeDevice(), m_ResView, nullptr);
      VKLOG(Info, "TResourceView Destroying.. -- %p.", m_ResView);
      m_ResView = VK_NULL_HANDLE;
    }
    if (m_SelfOwned && VK_NULL_HANDLE != m_NativeObj) {
      VKLOG(Info, "TResource Destroying.. -- %p.", m_NativeObj);
      ResTrait<TVkResObj>::Destroy(NativeDevice(), m_NativeObj, nullptr);
      m_NativeObj = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_DeviceMem) {
      VKLOG(Info,
            "TResource Freeing Memory. -- 0x%0x, tid:%d",
            m_DeviceMem,
            Os::Thread::GetId());
      vkFreeMemory(NativeDevice(), m_DeviceMem, nullptr);
      m_DeviceMem = VK_NULL_HANDLE;
    }
  }

  void* Map(uint64 offset, uint64 size) override
  {
    K3D_VK_VERIFY(vkMapMemory(
      NativeDevice(), m_DeviceMem, offset, size, 0, &m_HostMemAddr));
    return m_HostMemAddr;
  }

  void UnMap() override { vkUnmapMemory(NativeDevice(), m_DeviceMem); }

  uint64 GetLocation() const override { return (uint64)m_NativeObj; }
  uint64 GetSize() const override { return m_MemAllocInfo.allocationSize; }
  rhi::ResourceDesc GetDesc() const override { return m_ResDesc; }

protected:
  typedef typename ResTrait<TVkResObj>::CreateInfo CreateInfo;
  typedef typename ResTrait<TVkResObj>::View ResourceView;
  typedef typename ResTrait<TVkResObj>::DescriptorInfo ResourceDescriptorInfo;
  typedef typename ResTrait<TVkResObj>::UsageFlags ResourceUsageFlags;

  VkMemoryAllocateInfo m_MemAllocInfo;
  VkMemoryRequirements m_MemReq;
  VkDeviceMemory m_DeviceMem;
  VkMemoryPropertyFlags m_MemoryBits = 0;
  void* m_HostMemAddr;
  ResourceView m_ResView;
  ResourceUsageFlags m_ResUsageFlags = 0;
  ResourceDescriptorInfo m_ResDescInfo{};
  rhi::ResourceDesc m_ResDesc;
  bool m_SelfOwned = true;

protected:
  void Allocate(CreateInfo const& info)
  {
    K3D_VK_VERIFY(ResTrait<TVkResObj>::Create(
      NativeDevice(), &info, nullptr, &m_NativeObj));
    m_MemAllocInfo = {};
    m_MemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ResTrait<TVkResObj>::GetMemoryInfo(NativeDevice(), m_NativeObj, &m_MemReq);

    m_MemAllocInfo.allocationSize = m_MemReq.size;
    m_Device->FindMemoryType(
      m_MemReq.memoryTypeBits, m_MemoryBits, &m_MemAllocInfo.memoryTypeIndex);
    K3D_VK_VERIFY(
      vkAllocateMemory(NativeDevice(), &m_MemAllocInfo, nullptr, &m_DeviceMem));
    m_MemAllocInfo.allocationSize;

    /*K3D_VK_VERIFY(vkBindBufferMemory(NativeDevice(), m_Buffer, m_DeviceMem,
     * m_AllocationOffset));*/
  }
};

class Buffer : public TResource<VkBuffer, rhi::IGpuResource>
{
public:
  typedef Buffer* Ptr;
  explicit Buffer(Device::Ptr pDevice)
    : ThisResourceType(pDevice)
  {
  }
  Buffer(Device::Ptr pDevice, rhi::ResourceDesc const& desc);
  virtual ~Buffer();

  void Create(size_t size);
};

class Texture : public TResource<VkImage, rhi::ITexture>
{
public:
  typedef ::k3d::SharedPtr<Texture> TextureRef;

  explicit Texture(Device::Ptr pDevice)
    : ThisResourceType(pDevice)
  {
  }
  Texture(Device::Ptr pDevice, rhi::ResourceDesc const&);
  Texture(VkImage image,
          VkImageView imageView,
          VkImageViewCreateInfo info,
          Device::Ptr pDevice,
          bool selfOwnShip = true);
  ~Texture() override;

  static TextureRef CreateFromSwapChain(VkImage image,
                                        VkImageView view,
                                        VkImageViewCreateInfo info,
                                        Device::Ptr pDevice);

  const VkImageViewCreateInfo& GetViewInfo() const { return m_ImageViewInfo; }
  const VkImageView& GetView() const { return m_ResView; }
  const VkImage& Get() const { return m_NativeObj; }
  VkImageLayout GetImageLayout() const { return m_ImageLayout; }
  VkImageSubresourceRange GetSubResourceRange() const { return m_SubResRange; }

  void BindSampler(rhi::SamplerRef sampler) override;
  rhi::SamplerCRef GetSampler() const override;
  rhi::ShaderResourceViewRef GetResourceView() const override { return m_SRV; }
  void SetResourceView(rhi::ShaderResourceViewRef srv) override { m_SRV = srv; }
  rhi::EResourceState GetState() const override { return m_UsageState; }

  void CreateResourceView();
  friend class SwapChain;
  friend class CommandBuffer;
  friend class CommandContext;

private:
  /**
   * Create texture for Render
   */
  void CreateRenderTexture(rhi::TextureDesc const& desc);
  void CreateDepthStencilTexture(rhi::TextureDesc const& desc);
  /**
   * Create texture for Sampler
   */
  void CreateSampledTexture(rhi::TextureDesc const& desc);

private:
  ::k3d::SharedPtr<Sampler> m_ImageSampler;
  VkImageViewCreateInfo m_ImageViewInfo = {};
  rhi::ShaderResourceViewRef m_SRV;
  ImageInfo m_ImageInfo;
  rhi::EResourceState m_UsageState = rhi::ERS_Unknown;
  VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkImageMemoryBarrier m_Barrier;

  VkSubresourceLayout m_SubResourceLayout = {};
  VkImageSubresourceRange m_SubResRange = {};
};

class ResourceManager : public DeviceChild
{
public:
  struct Allocation
  {
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkDeviceSize Offset = 0;
    VkDeviceSize Size = 0;
  };

  template<typename VkObjectT>
  struct ResDesc
  {
    VkObjectT Object;
    bool IsTransient = false;
    uint32_t MemoryTypeIndex;
    VkMemoryPropertyFlags MemoryProperty;
    VkMemoryRequirements MemoryRequirements;
  };

  template<typename VkObjectT>
  class PoolManager;

  template<typename VkObject>
  class Pool
  {
  public:
    typedef std::unique_ptr<Pool<VkObject>> PoolPtr;
    virtual ~Pool() {}

    static PoolPtr Create(
      VkDevice device,
      const VkDeviceSize poolSize,
      const typename ResourceManager::ResDesc<VkObject>& objDesc);

    uint32 GetMemoryTypeIndex() const { return m_MemoryTypeIndex; }
    VkDeviceSize GetSize() const { return m_Size; }
    bool HasAvailable(VkMemoryRequirements memReqs) const;

    Allocation Allocate(
      const typename ResourceManager::ResDesc<VkObject>& objDesc);
    friend class PoolManager<VkObject>;

  private:
    Pool(uint32 memTypeIndex, VkDeviceMemory mem, VkDeviceSize sz);

    std::vector<Allocation> m_Allocations;

    uint32 m_MemoryTypeIndex = UINT32_MAX;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkDeviceSize m_Size = 0;
    VkDeviceSize m_Offset = 0;
  };

  template<typename VkObjectT>
  class PoolManager : public DeviceChild
  {
  public:
    using PoolRef = std::unique_ptr<Pool<VkObjectT>>;

    PoolManager(Device::Ptr pDevice, VkDeviceSize poolSize);
    virtual ~PoolManager();
    void Destroy();
    Allocation Allocate(
      const typename ResourceManager::ResDesc<VkObjectT>& objDesc);

    VkDeviceSize GetBlockSize() const { return m_PoolSize; }

  private:
    VkDevice m_Device = VK_NULL_HANDLE;
    size_t m_PoolSize = 0;
    ::Os::Mutex m_Mutex;
    std::vector<PoolRef> m_Pools;
  };

  explicit ResourceManager(Device::Ptr pDevice,
                           size_t bufferBlockSize,
                           size_t imageBlockSize);

  ~ResourceManager();

  Allocation AllocateBuffer(VkBuffer buffer,
                            bool transient,
                            VkMemoryPropertyFlags memoryProperty);
  Allocation AllocateImage(VkImage image,
                           bool transient,
                           VkMemoryPropertyFlags memoryProperty);

private:
  void Initialize();
  void Destroy();

  PoolManager<VkBuffer> m_BufferAllocations;
  PoolManager<VkImage> m_ImageAllocations;
};

class CommandContextPool : public DeviceChild
{
public:
  CommandContextPool(Device::Ptr pDevice);
  ~CommandContextPool() override;

  rhi::CommandContextRef RequestContext(rhi::ECommandType type);

  PtrCmdAlloc RequestCommandAllocator();

private:
  using Mutex = Os::Mutex;
  Mutex m_PoolMutex;
  Mutex m_ContextMutex;
  std::unordered_map<uint32, PtrCmdAlloc> m_AllocatorPool;
  std::unordered_map<uint32, std::list<CommandContext*>> m_ContextList;
};
/**
 * TODO: Need a Renderpass manager to cache all renderpasses
 */
class RenderTarget
  : public DeviceChild
  , public rhi::IRenderTarget
{
public:
  RenderTarget(Device::Ptr pDevice, rhi::RenderTargetLayout const& Layout);
  RenderTarget(Device::Ptr pDevice,
               Texture::TextureRef texture,
               SpFramebuffer framebuffer,
               VkRenderPass renderpass);
  ~RenderTarget() override;

  VkFramebuffer GetFramebuffer() const;
  VkRenderPass GetRenderpass() const;
  Texture::TextureRef GetTexture() const;
  VkRect2D GetRenderArea() const;
  rhi::GpuResourceRef GetBackBuffer() override;
  PtrSemaphore GetSemaphore() { return m_AcquireSemaphore; }

  void SetClearColor(kMath::Vec4f clrColor) override
  {
    m_ClearValues[0].color = {
      clrColor[0], clrColor[1], clrColor[2], clrColor[3]
    };
  }
  void SetClearDepthStencil(float depth, uint32 stencil) override
  {
    m_ClearValues[1].depthStencil = { depth, stencil };
  }

private:
  friend class CommandContext;

  VkClearValue m_ClearValues[2] = { {}, { 1.0f, 0 } };

  SpFramebuffer m_Framebuffer;
  VkRenderPass m_Renderpass;
  Texture::TextureRef m_RenderTexture;
  PtrSemaphore m_AcquireSemaphore;
};

#if 0
/**
 * Need CommandBufferManager to issue Cmds
 */
class CommandContext
  : public rhi::ICommandContext
  , public DeviceChild
{
public:
  explicit CommandContext(Device::Ptr pDevice);
  CommandContext(Device::Ptr pDevice,
                 VkCommandBuffer cmdBuf,
                 VkCommandPool pool,
                 rhi::ECommandType type = rhi::ECMD_Graphics);
  virtual ~CommandContext();

  void Detach(rhi::IDevice*) override;
  void CopyBuffer(rhi::IGpuResource& Dest,
                  rhi::IGpuResource& Src,
                  rhi::CopyBufferRegion const& Region) override;
  void CopyTexture(const rhi::TextureCopyLocation& Dest,
                   const rhi::TextureCopyLocation& Src);
  /**
   * @brief	submit command buffer to queue and wait for scheduling.
   * @param	Wait	true to wait.
   */
  void Execute(bool Wait) override;
  void Reset() override;

  void SubmitAndWait(PtrSemaphore wait, PtrSemaphore signal, PtrFence fence);

  void Begin() override;
  void End() override;

  void BeginRendering() override;
  void EndRendering() override;

  void PresentInViewport(rhi::RenderViewportRef) override;

  void BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
                          VkPipelineLayout layout,
                          uint32 firstSet,
                          uint32 descriptorSetCount,
                          const VkDescriptorSet* pDescriptorSets,
                          uint32 dynamicOffsetCount,
                          const uint32* pDynamicOffsets);
  void BindDescriptorSet(VkPipelineBindPoint pipelineBindPoint,
                         VkPipelineLayout layout,
                         const VkDescriptorSet& pDescriptorSets);
  void ClearColorImage(VkImage image,
                       VkImageLayout imageLayout,
                       const VkClearColorValue* pColor,
                       uint32 rangeCount,
                       const VkImageSubresourceRange* pRanges);

  void ClearColorImage(SpTexture colorBuffer,
                       const VkClearColorValue* pColor,
                       VkImageLayout imageLayout);

  void ClearDepthStencilImage(VkImage image,
                              VkImageLayout imageLayout,
                              const VkClearDepthStencilValue* pDepthStencil,
                              uint32 rangeCount,
                              const VkImageSubresourceRange* pRanges);
  void ClearAttachments(uint32 attachmentCount,
                        const VkClearAttachment* pAttachments,
                        uint32 rectCount,
                        const VkClearRect* pRects);
  void PipelineBarrier(VkPipelineStageFlags srcStageMask,
                       VkPipelineStageFlags dstStageMask,
                       VkDependencyFlags dependencyFlags,
                       uint32 memoryBarrierCount,
                       const VkMemoryBarrier* pMemoryBarriers,
                       uint32 bufferMemoryBarrierCount,
                       const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                       uint32 imageMemoryBarrierCount,
                       const VkImageMemoryBarrier* pImageMemoryBarriers);
  void PipelineBarrierBufferMemory(const BufferMemoryBarrierParams& params);
  void PipelineBarrierImageMemory(const ImageMemoryBarrierParams& params);
  void PushConstants(VkPipelineLayout layout,
                     VkShaderStageFlags stageFlags,
                     uint32 offset,
                     uint32 size,
                     const void* pValues);
  void BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin,
                       VkSubpassContents contents);
  void SetScissorRects(uint32 count, VkRect2D* pRects);
  /**
   *	@param contents
   *VK_SUBPASS_CONTENTS_INLINE,VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
   */
  void NextSubpass(VkSubpassContents contents);
  void EndRenderPass();

  void SetRenderTarget(rhi::RenderTargetRef rt) override;
  void ClearColorBuffer(rhi::GpuResourceRef, kMath::Vec4f const&) override;
  void SetViewport(const rhi::ViewportDesc&) override;
  void SetScissorRects(uint32, const rhi::Rect*) override;
  void SetIndexBuffer(const rhi::IndexBufferView& IBView) override;
  void SetVertexBuffer(uint32 Slot,
                       const rhi::VertexBufferView& VBView) override;
  void SetPipelineState(uint32 hashCode, rhi::PipelineStateRef const&) override;
  void SetPipelineLayout(rhi::PipelineLayoutRef pRHIPipelineLayout) override;
  void SetPrimitiveType(rhi::EPrimitiveType) override;
  void DrawInstanced(rhi::DrawInstancedParam) override;
  void DrawIndexedInstanced(rhi::DrawIndexedInstancedParam) override;
  void Dispatch(uint32 X, uint32 Y, uint32 Z) override;
  void TransitionResourceBarrier(
    rhi::GpuResourceRef resource,
    /* rhi::EPipelineStage stage,*/ rhi::EResourceState dstState) override;
  friend class CommandContextPool;

  void ExecuteBundle(rhi::ICommandContext*) override;

protected:
  VkCommandBuffer m_CommandBuffer;
  // VkCommandPool			m_CommandPool;
  VkRenderPass m_RenderPass;
  bool m_IsRenderPassActive = false;
  RenderTarget* m_CurrentRenderTarget = nullptr;
  rhi::ECommandType m_CmdType = rhi::ECMD_Graphics;

private:
  void InitCommandBufferPool();
};
#endif

class CommandQueue
  : public TVkRHIObjectBase<VkQueue, rhi::ICommandQueue>
  , public EnableSharedFromThis<CommandQueue>
{
public:
  using This = TVkRHIObjectBase<VkQueue, rhi::ICommandQueue>;
  CommandQueue(Device::Ptr pDevice,
               VkQueueFlags queueTypes,
               uint32 queueFamilyIndex,
               uint32 queueIndex);
  virtual ~CommandQueue();

  rhi::CommandBufferRef ObtainCommandBuffer(
    rhi::ECommandUsageType const&) override;

  void Submit(const std::vector<VkCommandBuffer>& cmdBufs,
              const std::vector<VkSemaphore>& waitSemaphores,
              const std::vector<VkPipelineStageFlags>& waitStageMasks,
              VkFence fence,
              const std::vector<VkSemaphore>& signalSemaphores);

  VkResult Submit(const std::vector<VkSubmitInfo>& submits, VkFence fence);

  void Present(SwapChainRef& pSwapChain);

  void WaitIdle();

protected:
  void Initialize(VkQueueFlags queueTypes,
                  uint32 queueFamilyIndex,
                  uint32 queueIndex);
  void Destroy();

private:
  VkQueueFlags m_QueueTypes = 0;
  uint32 m_QueueFamilyIndex = UINT32_MAX;
  uint32 m_QueueIndex = UINT32_MAX;

  CmdBufManagerRef m_TransientCmdBufferPool;
  CmdBufManagerRef m_ReUsableCmdBufferPool;
};

class CommandBuffer
  : public TVkRHIObjectBase<VkCommandBuffer, rhi::ICommandBuffer>
  , public EnableSharedFromThis<CommandBuffer>
{
public:
  using This = TVkRHIObjectBase<VkCommandBuffer, rhi::ICommandBuffer>;

  void Release() override;

  void Commit() override;
  void Present(rhi::SwapChainRef pSwapChain, rhi::SyncFenceRef pFence) override;

  void Reset() override;
  rhi::RenderCommandEncoderRef RenderCommandEncoder(
    rhi::RenderTargetRef const&,
    rhi::RenderPipelineStateRef const&) override;
  rhi::ComputeCommandEncoderRef ComputeCommandEncoder(
    rhi::ComputePipelineStateRef const&) override;
  rhi::ParallelRenderCommandEncoderRef ParallelRenderCommandEncoder(
    rhi::RenderTargetRef const&,
    rhi::RenderPipelineStateRef const&) override;
  void CopyTexture(const rhi::TextureCopyLocation& Dest,
                   const rhi::TextureCopyLocation& Src) override;
  void CopyBuffer(rhi::GpuResourceRef Dest,
                  rhi::GpuResourceRef Src,
                  rhi::CopyBufferRegion const& Region) override;
  void Transition(rhi::GpuResourceRef pResource,
                  rhi::EResourceState const&
                    State /*, rhi::EPipelineStage const& Stage*/) override;

  SpCmdQueue OwningQueue() { return m_OwningQueue; }

  friend class CommandQueue;
  template <typename T>
  friend class CommandEncoder;

protected:
  template<typename T>
  friend class k3d::TRefCountInstance;

  CommandBuffer(Device::Ptr, SpCmdQueue, VkCommandBuffer);

  bool m_Ended;
  SpCmdQueue m_OwningQueue;
  rhi::SwapChainRef m_PendingSwapChain;
};

template <typename CmdEncoderSubT>
class CommandEncoder : public CmdEncoderSubT
{
public:
  explicit CommandEncoder(SpCmdBuffer pCmd)
    : m_MasterCmd(pCmd)
  {
    m_pQueue = m_MasterCmd->OwningQueue();
  }

  void SetPipelineState(uint32 HashCode, rhi::PipelineStateRef const& pPipeline) override
  {
    K3D_ASSERT(pPipeline);
    if (pPipeline->GetType() == rhi::EPSO_Compute)
    {
      ComputePipelineState * computePso = static_cast<ComputePipelineState*>(pPipeline.Get());
      vkCmdBindPipeline(m_MasterCmd->NativeHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, computePso->NativeHandle());
    }
    else
    {
      RenderPipelineState * gfxPso = static_cast<RenderPipelineState*>(pPipeline.Get());
      vkCmdBindPipeline(m_MasterCmd->NativeHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, gfxPso->NativeHandle());
    }
  }

  void SetPipelineLayout(rhi::PipelineLayoutRef const& pLayout) override
  {
    K3D_ASSERT(pLayout);
    auto pipelineLayout = StaticPointerCast<PipelineLayout>(pLayout);
    VkDescriptorSet sets[] = { pipelineLayout->GetNativeDescriptorSet() };
    vkCmdBindDescriptorSets(m_MasterCmd->NativeHandle(), 
      GetBindPoint(),
      pipelineLayout->NativeHandle(), 
      0, // first set
      1, sets, // set count, 
      0, NULL); // dynamic offset
  }

  virtual void EndEncode() override
  {
    vkEndCommandBuffer(m_MasterCmd->NativeHandle());
    m_MasterCmd->m_Ended = true;
  }

  virtual VkPipelineBindPoint GetBindPoint() const = 0;

protected:
  SpCmdBuffer m_MasterCmd;
  SpCmdQueue m_pQueue;
};

enum class ECmdLevel : uint8
{
  Primary,
  Secondary
};

class ParallelCommandEncoder;
using SpParallelCmdEncoder = SharedPtr<ParallelCommandEncoder>;
class RenderCommandEncoder : public CommandEncoder<rhi::IRenderCommandEncoder>
{
public:
  using This = CommandEncoder<rhi::IRenderCommandEncoder>;
  void SetScissorRect(const rhi::Rect&) override;
  void SetViewport(const rhi::ViewportDesc&) override;
  void SetIndexBuffer(const rhi::IndexBufferView& IBView) override;
  void SetVertexBuffer(uint32 Slot, const rhi::VertexBufferView& VBView) override;
  void SetPrimitiveType(rhi::EPrimitiveType) override;
  void DrawInstanced(rhi::DrawInstancedParam) override;
  void DrawIndexedInstanced(rhi::DrawIndexedInstancedParam) override;
  void EndEncode() override;
  VkPipelineBindPoint GetBindPoint() const override 
  {
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
  }
  VkCommandBuffer OwningCmd() const { return m_MasterCmd->NativeHandle(); }
  friend class CommandBuffer;
  friend class ParallelCommandEncoder;
  template<typename T>
  friend class k3d::TRefCountInstance;
protected:
  RenderCommandEncoder(SpCmdBuffer pCmd, ECmdLevel Level);
  RenderCommandEncoder(SpParallelCmdEncoder ParentEncoder, SpCmdBuffer pCmd);
private:
  ECmdLevel m_Level;
};
using SpRenderCommandEncoder = SharedPtr<RenderCommandEncoder>;
class ComputeCommandEncoder : public CommandEncoder<rhi::IComputeCommandEncoder>
{
public:
  using This = CommandEncoder<rhi::IComputeCommandEncoder>;
  void Dispatch(uint32 GroupCountX,
    uint32 GroupCountY,
    uint32 GroupCountZ) override; 
  VkPipelineBindPoint GetBindPoint() const override
  {
    return VK_PIPELINE_BIND_POINT_COMPUTE;
  }
  friend class CommandBuffer;
  template<typename T>
  friend class k3d::TRefCountInstance;
};

class ParallelCommandEncoder 
  : public CommandEncoder<rhi::IParallelRenderCommandEncoder>
  , EnableSharedFromThis<ParallelCommandEncoder>
{
public:
  using This = CommandEncoder<rhi::IParallelRenderCommandEncoder>;
  rhi::RenderCommandEncoderRef SubRenderCommandEncoder() override;
  void EndEncode() override;
  VkPipelineBindPoint GetBindPoint() const override
  {
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
  }
  friend class CommandBuffer;
  template<typename T>
  friend class k3d::TRefCountInstance;
private:
  bool m_RenderpassBegun = false;
  DynArray<SpRenderCommandEncoder> m_RecordedCmds;
};

class SwapChain : public TVkRHIObjectBase<VkSwapchainKHR, rhi::ISwapChain>
{
public:
  using This = TVkRHIObjectBase<VkSwapchainKHR, rhi::ISwapChain>;

  ~SwapChain();

  SwapChain(Device::Ptr pDevice, void* pWindow, rhi::SwapChainDesc& Desc)
    : SwapChain::This(pDevice)
  {
    Init(pWindow, Desc);
  }

  // ISwapChain::Release
  void Release() override;
  // ISwapChain::Resize
  void Resize(uint32 Width, uint32 Height) override;
  // ISwapChain::GetCurrentTexture
  rhi::TextureRef GetCurrentTexture() override;

  void Present() override;

  void QueuePresent(SpCmdQueue pQueue, rhi::SyncFenceRef pFence);

  void AcquireNextImage();

  void Init(void* pWindow, rhi::SwapChainDesc& Desc);

  void Initialize(void* WindowHandle, rhi::RenderViewportDesc& gfxSetting);

  uint32 GetPresentQueueFamilyIndex() const
  {
    return m_SelectedPresentQueueFamilyIndex;
  }
  uint32 AcquireNextImage(PtrSemaphore presentSemaphore, PtrFence pFence);
  VkResult Present(uint32 imageIndex, PtrSemaphore renderingFinishSemaphore);
  uint32 GetBackBufferCount() const { return m_ReserveBackBufferCount; }

  VkSwapchainKHR GetSwapChain() const { return m_SwapChain; }
  VkImage GetBackImage(uint32 i) const { return m_ColorImages[i]; }

  VkExtent2D GetCurrentExtent() const { return m_SwapchainExtent; }
  VkFormat GetFormat() const { return m_ColorAttachFmt; }

private:
  void InitSurface(void* WindowHandle);
  VkPresentModeKHR ChoosePresentMode();
  std::pair<VkFormat, VkColorSpaceKHR> ChooseFormat(rhi::EPixelFormat& Format);
  int ChooseQueueIndex();
  void InitSwapChain(uint32 numBuffers,
                     std::pair<VkFormat, VkColorSpaceKHR> color,
                     VkPresentModeKHR mode,
                     VkSurfaceTransformFlagBitsKHR pretran);

private:
  friend class CommandQueue;

  VkSemaphore m_PresentSemaphore = VK_NULL_HANDLE;

  VkExtent2D m_SwapchainExtent = {};
  VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
  VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

  uint32 m_CurrentBufferID = 0;
  uint32 m_SelectedPresentQueueFamilyIndex = 0;
  uint32 m_DesiredBackBufferCount;
  uint32 m_ReserveBackBufferCount;

  VkFormat m_ColorAttachFmt = VK_FORMAT_UNDEFINED;

  DynArray<rhi::TextureRef> m_Buffers;
  std::vector<VkImage> m_ColorImages;
};
#if 0
class RenderViewport
  : public rhi::IRenderViewport
  , public DeviceChild
{
public:
  RenderViewport(Device::Ptr, void* windowHandle, rhi::RenderViewportDesc&);
  ~RenderViewport() override;
  bool InitViewport(void* windowHandle,
                    rhi::IDevice* pDevice,
                    rhi::RenderViewportDesc&) override;

  void PrepareNextFrame() override;

  bool Present(bool vSync) override;

  rhi::RenderTargetRef GetRenderTarget(uint32 index) override;
  rhi::RenderTargetRef GetCurrentBackRenderTarget() override;

  uint32 GetSwapChainIndex() override;
  uint32 GetSwapChainCount() override;

  PtrSemaphore GetPresentSemaphore() const { return m_PresentSemaphore; }
  PtrSemaphore GetRenderSemaphore() const { return m_RenderSemaphore; }

  void AllocateDefaultRenderPass(rhi::RenderViewportDesc& gfxSetting);
  void AllocateRenderTargets(rhi::RenderViewportDesc& gfxSetting);
  VkRenderPass GetRenderPass() const;

  uint32 GetWidth() const override
  {
    return m_pSwapChain->GetCurrentExtent().width;
  }
  uint32 GetHeight() const override
  {
    return m_pSwapChain->GetCurrentExtent().height;
  }

protected:
  PtrSemaphore m_PresentSemaphore;
  PtrSemaphore m_RenderSemaphore;
  SpRenderpass m_RenderPass;

private:
  std::vector<rhi::RenderTargetRef> m_RenderTargets;

  uint32 m_CurFrameId;
  SwapChainRef m_pSwapChain;
  uint32 m_NumBufferCount;
};
#endif
class RenderPass;
class FrameBuffer : public DeviceChild
{
public:
  struct Attachment
  {
    Attachment(VkFormat format, VkSampleCountFlagBits samples);
    explicit Attachment(VkImageView view) { this->ImageAttachment = view; }

    virtual ~Attachment() {}

    bool IsColorAttachment() const
    {
      return 0 != (FormatFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    }
    bool IsDepthStencilAttachment() const
    {
      return 0 !=
             (FormatFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkFormatFeatureFlags FormatFeatures = 0;
    VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageView ImageAttachment;
  };

  struct Option
  {
    uint32 Width, Height;
    std::vector<Attachment> Attachments;
  };

  /**
   * create framebuffer with SwapChain ImageViews
   */
  FrameBuffer(Device::Ptr pDevice, VkRenderPass renderPass, Option const& op);
  FrameBuffer(Device::Ptr pDevice,
              RenderPass* renderPass,
              RenderTargetLayout const&);
  ~FrameBuffer();

  uint32 GetWidth() const { return m_Width; }
  uint32 GetHeight() const { return m_Height; }
  VkFramebuffer const Get() const { return m_FrameBuffer; }

private:
  friend class RenderPass;

  VkFramebuffer m_FrameBuffer = VK_NULL_HANDLE;
  VkRenderPass m_RenderPass = VK_NULL_HANDLE;
  uint32 m_Width = 0;
  uint32 m_Height = 0;
};

using PtrFrameBuffer = std::shared_ptr<FrameBuffer>;

class RenderPass : public DeviceChild
{
public:
  typedef RenderPass* Ptr;

  RenderPass(Device::Ptr pDevice, RenderpassOptions const& options);
  RenderPass(Device::Ptr pDevice, RenderTargetLayout const& rtl);
  ~RenderPass();

  void NextSubpass();

  VkRenderPass GetPass() const { return m_RenderPass; }
  RenderpassOptions const& GetOption() const { return m_Options; }

private:
  RenderpassOptions m_Options;
  PtrContext m_GfxContext;
  PtrFrameBuffer m_FrameBuffer;
  VkRenderPass m_RenderPass = VK_NULL_HANDLE;

  uint32 mSubpass = 0;
  std::vector<VkSampleCountFlagBits> mSubpassSampleCounts;
  std::vector<uint32> mBarrieredAttachmentIndices;
  std::vector<VkAttachmentDescription> mAttachmentDescriptors;
  std::vector<VkClearValue> mAttachmentClearValues;

  void Initialize(RenderpassOptions const& options);
  void Initialize(RenderTargetLayout const& rtl);
  void Destroy();

  friend class FrameBuffer;
};

template<typename PipelineSubType>
class TPipelineState : public PipelineSubType
{
public:
  TPipelineState(Device::Ptr pDevice)
    : m_Device(pDevice)
    , m_Pipeline(VK_NULL_HANDLE)
    , m_PipelineCache(VK_NULL_HANDLE)
  {
  }

  virtual ~TPipelineState() override
  {
    vkDestroyPipelineCache(m_Device->GetRawDevice(), m_PipelineCache, nullptr);
    vkDestroyPipeline(m_Device->GetRawDevice(), m_Pipeline, nullptr);
  }

  void SavePSO(String const& Path) override {}

  void LoadPSO(String const& Path) override {}

  VkPipeline NativeHandle() const 
  {
    return m_Pipeline;
  }

protected:
  VkPipelineShaderStageCreateInfo ConvertStageInfoFromShaderBundle(
    rhi::ShaderBundle const& Bundle)
  {
    return { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
             nullptr,
             0,
             g_ShaderType[Bundle.Desc.Stage],
             m_Device->CreateShaderModule(Bundle),
             Bundle.Desc.EntryFunction.CStr() };
  }

  Device::Ptr m_Device;

  VkPipeline m_Pipeline;
  VkPipelineCache m_PipelineCache;
};

class RenderPipelineState : public TPipelineState<rhi::IRenderPipelineState>
{
public:
  RenderPipelineState(Device::Ptr pDevice,
                      rhi::RenderPipelineStateDesc const& desc,
                      PipelineLayout* ppl);
  virtual ~RenderPipelineState();

  void BindRenderPass(VkRenderPass RenderPass);

  void SetRasterizerState(const rhi::RasterizerState&) override;
  void SetBlendState(const rhi::BlendState&) override;
  void SetDepthStencilState(const rhi::DepthStencilState&) override;
  void SetSampler(rhi::SamplerRef) override;
  void SetVertexInputLayout(rhi::VertexDeclaration const*,
                            uint32 Count) override;
  void SetPrimitiveTopology(const rhi::EPrimitiveType) override;
  void SetRenderTargetFormat(const rhi::RenderTargetFormat&) override;

  VkPipeline GetPipeline() const { return m_Pipeline; }
  void Rebuild() override;

  /**
   * TOFIX
   */
  rhi::EPipelineType GetType() const override
  {
    return rhi::EPipelineType::EPSO_Graphics;
  }

protected:
  void InitWithDesc(rhi::RenderPipelineStateDesc const& desc);
  void Destroy();

  friend class CommandContext;

  VkGraphicsPipelineCreateInfo m_GfxCreateInfo;
  VkRenderPass m_RenderPass;

private:
  DynArray<VkPipelineShaderStageCreateInfo> m_ShaderStageInfos;
  VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyState;
  VkPipelineRasterizationStateCreateInfo m_RasterizationState;
  VkPipelineColorBlendStateCreateInfo m_ColorBlendState;
  VkPipelineDepthStencilStateCreateInfo m_DepthStencilState;
  VkPipelineViewportStateCreateInfo m_ViewportState;
  VkPipelineMultisampleStateCreateInfo m_MultisampleState;
  VkPipelineVertexInputStateCreateInfo m_VertexInputState;
  std::vector<VkVertexInputBindingDescription> m_BindingDescriptions;
  std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;
  PipelineLayout* m_PipelineLayout;
};

class ComputePipelineState : public TPipelineState<rhi::IComputePipelineState>
{
public:
  ComputePipelineState(Device::Ptr pDevice,
                       rhi::ComputePipelineStateDesc const& desc,
                       PipelineLayout* ppl);

  ~ComputePipelineState() override {}

  rhi::EPipelineType GetType() const override
  {
    return rhi::EPipelineType::EPSO_Compute;
  }
  void Rebuild() override;

  friend class CommandContext;

private:
  VkComputePipelineCreateInfo m_ComputeCreateInfo;
  PipelineLayout* m_PipelineLayout;
};

class ShaderResourceView
  : public TVkRHIObjectBase<VkImageView, rhi::IShaderResourceView>
{
public:
  ShaderResourceView(Device::Ptr pDevice,
                     rhi::ResourceViewDesc const& desc,
                     rhi::GpuResourceRef gpuResource);
  ~ShaderResourceView() override;
  rhi::GpuResourceRef GetResource() const override
  {
    return rhi::GpuResourceRef(m_WeakResource);
  }
  rhi::ResourceViewDesc GetDesc() const override { return m_Desc; }
  VkImageView NativeImageView() const { return m_NativeObj; }

private:
  // rhi::GpuResourceRef		m_Resource;
  WeakPtr<rhi::IGpuResource> m_WeakResource;
  rhi::ResourceViewDesc m_Desc;
  VkImageViewCreateInfo m_TextureViewInfo;
};

class PipelineLayout
  : public TVkRHIObjectBase<VkPipelineLayout, rhi::IPipelineLayout>
{
public:
  PipelineLayout(Device::Ptr pDevice, rhi::PipelineLayoutDesc const& desc);
  ~PipelineLayout() override;

  VkDescriptorSet GetNativeDescriptorSet() const
  {
    return static_cast<DescriptorSet*>(m_DescSet.Get())->GetNativeHandle();
  }

  rhi::DescriptorRef GetDescriptorSet() const override { return m_DescSet; }

protected:
  void InitWithDesc(rhi::PipelineLayoutDesc const& desc);
  void Destroy();
  friend class RenderPipelineState;

private:
  rhi::DescriptorRef m_DescSet;
  DescriptorSetLayoutRef m_DescSetLayout;
};

class CommandAllocator;
using PtrCmdAlloc = std::shared_ptr<CommandAllocator>;

class CommandAllocator : public DeviceChild
{
public:
  static PtrCmdAlloc CreateAllocator(uint32 queueFamilyIndex,
                                     bool transient,
                                     Device::Ptr device);

  ~CommandAllocator();

  VkCommandPool GetCommandPool() const { return m_Pool; }

  void Destroy();

protected:
  void Initialize();

private:
  CommandAllocator(uint32 queueFamilyIndex, bool transient, Device::Ptr device);

  VkCommandPool m_Pool;
  bool m_Transient;
  uint32 m_FamilyIndex;
};

class Semaphore : public DeviceChild
{
public:
  Semaphore(Device::Ptr pDevice,
            VkSemaphoreCreateInfo const& info = SemaphoreCreateInfo::Create())
    : DeviceChild(pDevice)
  {
    K3D_VK_VERIFY(
      vkCreateSemaphore(GetRawDevice(), &info, nullptr, &m_Semaphore));
    VKLOG(Info, "Semaphore Created. (0x%0x).", m_Semaphore);
  }
  ~Semaphore()
  {
    VKLOG(Info, "Semaphore Destroyed. (0x%0x).", m_Semaphore);
    vkDestroySemaphore(GetRawDevice(), m_Semaphore, nullptr);
  }

  VkSemaphore GetNativeHandle() const { return m_Semaphore; }

private:
  friend class SwapChain;
  friend class CommandContext;

  VkSemaphore m_Semaphore;
};

K3D_VK_END

#endif