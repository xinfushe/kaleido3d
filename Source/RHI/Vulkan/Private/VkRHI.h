#ifndef __VkRHI_h__
#define __VkRHI_h__
#pragma once
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

class CommandContextPool;

class SwapChain;
using SpSwapChain = SharedPtr<SwapChain>;

class CommandQueue;
using SpCmdQueue = SharedPtr<CommandQueue>;

class CommandBuffer;
using SpCmdBuffer = SharedPtr<CommandBuffer>;

class PipelineLayout;
using SpPipelineLayout = SharedPtr<PipelineLayout>;

class DescriptorAllocator;
class DescriptorSetLayout;

class FrameBuffer;
using SpFramebuffer = SharedPtr<FrameBuffer>;
using WpFramebuffer = WeakPtr<FrameBuffer>;

class RenderPass;
using SpRenderpass = SharedPtr<RenderPass>;
using WpRenderpass = WeakPtr<RenderPass>;

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

class Instance;
using InstanceRef = SharedPtr<Instance>;

class Gpu;
using GpuRef = SharedPtr<Gpu>;

typedef std::unordered_map<uint32, DescriptorSetLayoutRef>
  MapDescriptorSetLayout;
typedef std::unordered_map<uint32, DescriptorAllocRef> MapDescriptorAlloc;

template<class RHIObj>
struct ResTrait
{
};

template<>
struct ResTrait<NGFXBuffer>
{
  typedef VkBuffer Obj;
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
struct ResTrait<NGFXTexture>
{
  typedef VkImage Obj;
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

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugReportCallback(VkDebugReportFlagsEXT flags,
  VkDebugReportObjectTypeEXT objectType,
  uint64_t object,
  size_t location,
  int32_t messageCode,
  const char* pLayerPrefix,
  const char* pMessage,
  void* pUserData);

struct RHIRoot
{
private:
  static void EnumLayersAndExts();
  friend class Device;
};

#ifdef VK_NO_PROTOTYPES
#define __VK_DEVICE_PROC__(name) PFN_vk##name vk##name = NULL
#endif

class Gpu : public EnableSharedFromThis<Gpu>
{
public:
  ~Gpu();

  VkDevice CreateLogicDevice(bool enableValidation);
  VkBool32 GetSupportedDepthFormat(VkFormat* depthFormat);

  VkResult GetSurfaceSupportKHR(uint32_t queueFamilyIndex,
    VkSurfaceKHR surface,
    VkBool32* pSupported);
  VkResult GetSurfaceCapabilitiesKHR(
    VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* pSurfaceCapabilities);
  VkResult GetSurfaceFormatsKHR(VkSurfaceKHR surface,
    uint32_t* pSurfaceFormatCount,
    VkSurfaceFormatKHR* pSurfaceFormats);
  VkResult GetSurfacePresentModesKHR(VkSurfaceKHR surface,
    uint32_t* pPresentModeCount,
    VkPresentModeKHR* pPresentModes);

  void DestroyDevice();
  void FreeCommandBuffers(VkCommandPool, uint32, VkCommandBuffer*);
  VkResult CreateCommdPool(const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pCommandPool);
  VkResult AllocateCommandBuffers(
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers);

  InstanceRef GetInstance() const { return m_Inst; }

#ifdef VK_NO_PROTOTYPES
  __VK_DEVICE_PROC__(DestroyDevice);
  __VK_DEVICE_PROC__(GetDeviceQueue);
  __VK_DEVICE_PROC__(QueueSubmit);
  __VK_DEVICE_PROC__(QueueWaitIdle);
  __VK_DEVICE_PROC__(QueuePresentKHR);
  __VK_DEVICE_PROC__(DeviceWaitIdle);
  __VK_DEVICE_PROC__(AllocateMemory);
  __VK_DEVICE_PROC__(FreeMemory);
  __VK_DEVICE_PROC__(MapMemory);
  __VK_DEVICE_PROC__(UnmapMemory);
  __VK_DEVICE_PROC__(FlushMappedMemoryRanges);
  __VK_DEVICE_PROC__(InvalidateMappedMemoryRanges);
  __VK_DEVICE_PROC__(GetDeviceMemoryCommitment);
  __VK_DEVICE_PROC__(BindBufferMemory);
  __VK_DEVICE_PROC__(BindImageMemory);
  __VK_DEVICE_PROC__(GetBufferMemoryRequirements);
  __VK_DEVICE_PROC__(GetImageMemoryRequirements);
  __VK_DEVICE_PROC__(GetImageSparseMemoryRequirements);
  __VK_DEVICE_PROC__(QueueBindSparse);
  __VK_DEVICE_PROC__(CreateFence);
  __VK_DEVICE_PROC__(DestroyFence);
  __VK_DEVICE_PROC__(ResetFences);
  __VK_DEVICE_PROC__(GetFenceStatus);
  __VK_DEVICE_PROC__(WaitForFences);
  __VK_DEVICE_PROC__(CreateSemaphore);
  __VK_DEVICE_PROC__(DestroySemaphore);
  __VK_DEVICE_PROC__(CreateEvent);
  __VK_DEVICE_PROC__(DestroyEvent);
  __VK_DEVICE_PROC__(GetEventStatus);
  __VK_DEVICE_PROC__(SetEvent);
  __VK_DEVICE_PROC__(ResetEvent);
  __VK_DEVICE_PROC__(CreateQueryPool);
  __VK_DEVICE_PROC__(DestroyQueryPool);
  __VK_DEVICE_PROC__(GetQueryPoolResults);
  __VK_DEVICE_PROC__(CreateBuffer);
  __VK_DEVICE_PROC__(DestroyBuffer);
  __VK_DEVICE_PROC__(CreateBufferView);
  __VK_DEVICE_PROC__(DestroyBufferView);
  __VK_DEVICE_PROC__(CreateImage);
  __VK_DEVICE_PROC__(DestroyImage);
  __VK_DEVICE_PROC__(GetImageSubresourceLayout);
  __VK_DEVICE_PROC__(CreateImageView);
  __VK_DEVICE_PROC__(DestroyImageView);
  __VK_DEVICE_PROC__(CreateShaderModule);
  __VK_DEVICE_PROC__(DestroyShaderModule);
  __VK_DEVICE_PROC__(CreatePipelineCache);
  __VK_DEVICE_PROC__(DestroyPipelineCache);
  __VK_DEVICE_PROC__(GetPipelineCacheData);
  __VK_DEVICE_PROC__(MergePipelineCaches);
  __VK_DEVICE_PROC__(CreateGraphicsPipelines);
  __VK_DEVICE_PROC__(CreateComputePipelines);
  __VK_DEVICE_PROC__(DestroyPipeline);
  __VK_DEVICE_PROC__(CreatePipelineLayout);
  __VK_DEVICE_PROC__(DestroyPipelineLayout);
  __VK_DEVICE_PROC__(CreateSampler);
  __VK_DEVICE_PROC__(DestroySampler);
  __VK_DEVICE_PROC__(CreateDescriptorSetLayout);
  __VK_DEVICE_PROC__(DestroyDescriptorSetLayout);
  __VK_DEVICE_PROC__(CreateDescriptorPool);
  __VK_DEVICE_PROC__(DestroyDescriptorPool);
  __VK_DEVICE_PROC__(ResetDescriptorPool);
  __VK_DEVICE_PROC__(AllocateDescriptorSets);
  __VK_DEVICE_PROC__(FreeDescriptorSets);
  __VK_DEVICE_PROC__(UpdateDescriptorSets);
  __VK_DEVICE_PROC__(CreateFramebuffer);
  __VK_DEVICE_PROC__(DestroyFramebuffer);
  __VK_DEVICE_PROC__(CreateRenderPass);
  __VK_DEVICE_PROC__(DestroyRenderPass);
  __VK_DEVICE_PROC__(GetRenderAreaGranularity);
  __VK_DEVICE_PROC__(CreateCommandPool);
  __VK_DEVICE_PROC__(DestroyCommandPool);
  __VK_DEVICE_PROC__(ResetCommandPool);
  __VK_DEVICE_PROC__(AllocateCommandBuffers);
  __VK_DEVICE_PROC__(FreeCommandBuffers);
  __VK_DEVICE_PROC__(BeginCommandBuffer);
  __VK_DEVICE_PROC__(EndCommandBuffer);
  __VK_DEVICE_PROC__(ResetCommandBuffer);
  __VK_DEVICE_PROC__(CmdBindPipeline);
  __VK_DEVICE_PROC__(CmdSetViewport);
  __VK_DEVICE_PROC__(CmdSetScissor);
  __VK_DEVICE_PROC__(CmdSetLineWidth);
  __VK_DEVICE_PROC__(CmdSetDepthBias);
  __VK_DEVICE_PROC__(CmdSetBlendConstants);
  __VK_DEVICE_PROC__(CmdSetDepthBounds);
  __VK_DEVICE_PROC__(CmdSetStencilCompareMask);
  __VK_DEVICE_PROC__(CmdSetStencilWriteMask);
  __VK_DEVICE_PROC__(CmdSetStencilReference);
  __VK_DEVICE_PROC__(CmdBindDescriptorSets);
  __VK_DEVICE_PROC__(CmdBindIndexBuffer);
  __VK_DEVICE_PROC__(CmdBindVertexBuffers);
  __VK_DEVICE_PROC__(CmdDraw);
  __VK_DEVICE_PROC__(CmdDrawIndexed);
  __VK_DEVICE_PROC__(CmdDrawIndirect);
  __VK_DEVICE_PROC__(CmdDrawIndexedIndirect);
  __VK_DEVICE_PROC__(CmdDispatch);
  __VK_DEVICE_PROC__(CmdDispatchIndirect);
  __VK_DEVICE_PROC__(CmdCopyBuffer);
  __VK_DEVICE_PROC__(CmdCopyImage);
  __VK_DEVICE_PROC__(CmdBlitImage);
  __VK_DEVICE_PROC__(CmdCopyBufferToImage);
  __VK_DEVICE_PROC__(CmdCopyImageToBuffer);
  __VK_DEVICE_PROC__(CmdUpdateBuffer);
  __VK_DEVICE_PROC__(CmdFillBuffer);
  __VK_DEVICE_PROC__(CmdClearColorImage);
  __VK_DEVICE_PROC__(CmdClearDepthStencilImage);
  __VK_DEVICE_PROC__(CmdClearAttachments);
  __VK_DEVICE_PROC__(CmdResolveImage);
  __VK_DEVICE_PROC__(CmdSetEvent);
  __VK_DEVICE_PROC__(CmdResetEvent);
  __VK_DEVICE_PROC__(CmdWaitEvents);
  __VK_DEVICE_PROC__(CmdPipelineBarrier);
  __VK_DEVICE_PROC__(CmdBeginQuery);
  __VK_DEVICE_PROC__(CmdEndQuery);
  __VK_DEVICE_PROC__(CmdResetQueryPool);
  __VK_DEVICE_PROC__(CmdWriteTimestamp);
  __VK_DEVICE_PROC__(CmdCopyQueryPoolResults);
  __VK_DEVICE_PROC__(CmdPushConstants);
  __VK_DEVICE_PROC__(CmdBeginRenderPass);
  __VK_DEVICE_PROC__(CmdNextSubpass);
  __VK_DEVICE_PROC__(CmdEndRenderPass);
  __VK_DEVICE_PROC__(CmdExecuteCommands);
  __VK_DEVICE_PROC__(AcquireNextImageKHR);
  __VK_DEVICE_PROC__(CreateSwapchainKHR);
  __VK_DEVICE_PROC__(DestroySwapchainKHR);
  __VK_DEVICE_PROC__(GetSwapchainImagesKHR);

  PFN_vkDestroyDevice fpDestroyDevice = NULL;
  PFN_vkFreeCommandBuffers fpFreeCommandBuffers = NULL;
  PFN_vkCreateCommandPool fpCreateCommandPool = NULL;
  PFN_vkAllocateCommandBuffers fpAllocateCommandBuffers = NULL;
#endif

  VkDevice m_LogicalDevice;

private:
  friend class Instance;
  friend class Device;
  friend class DeviceAdapter;

  Gpu(VkPhysicalDevice const&, InstanceRef const& inst);

  void QuerySupportQueues();
  void LoadDeviceProcs();

  InstanceRef m_Inst;
  VkPhysicalDevice m_PhysicalGpu;
  VkPhysicalDeviceProperties m_Prop;
  VkPhysicalDeviceFeatures m_Features;
  VkPhysicalDeviceMemoryProperties m_MemProp;
  uint32 m_GraphicsQueueIndex = 0;
  uint32 m_ComputeQueueIndex = 0;
  uint32 m_CopyQueueIndex = 0;
  DynArray<VkQueueFamilyProperties> m_QueueProps;
};

class CommandBufferManager
{
public:
  CommandBufferManager(VkDevice pDevice,
    VkCommandBufferLevel bufferLevel,
    unsigned graphicsQueueIndex);
  ~CommandBufferManager();
  void Destroy();
  VkCommandBuffer RequestCommandBuffer();
  void BeginFrame();

private:

  VkDevice m_Device = VK_NULL_HANDLE;
  VkCommandPool m_Pool = VK_NULL_HANDLE;
  DynArray<VkCommandBuffer> m_Buffers;
  VkCommandBufferLevel m_CommandBufferLevel;
  uint32 m_Count = 0;
};

using CmdBufManagerRef = SharedPtr<CommandBufferManager>;

using MapFramebuffer = std::unordered_map<uint64, SpFramebuffer>;
using MapRenderpass = std::unordered_map<uint64, SpRenderpass>;
class DeviceObjectCache
{
public:
  static MapFramebuffer s_Framebuffer;
  static MapRenderpass s_RenderPass;
};

class Device
  : public NGFXDevice
  , public k3d::EnableSharedFromThis<Device>
{
public:
  typedef k3d::SharedPtr<Device> Ptr;

  Device();
  ~Device() override;
  void Release() override;

  NGFXResourceRef CreateResource(k3d::ResourceDesc const&) override;

  NGFXSRVRef CreateShaderResourceView(
    NGFXResourceRef,
    k3d::SRVDesc const&) override;

  NGFXUAVRef CreateUnorderedAccessView(const NGFXResourceRef&,
    k3d::UAVDesc const&) override;

  NGFXSamplerRef CreateSampler(const k3d::SamplerState&) override;

  NGFXPipelineLayoutRef CreatePipelineLayout(
    k3d::PipelineLayoutDesc const& table) override;

  NGFXRenderpassRef CreateRenderPass(k3d::RenderPassDesc const&) override;

  NGFXPipelineStateRef CreateRenderPipelineState(
    k3d::RenderPipelineStateDesc const&,
    NGFXPipelineLayoutRef,
    NGFXRenderpassRef) override;

  NGFXPipelineStateRef CreateComputePipelineState(
    k3d::ComputePipelineStateDesc const&,
    NGFXPipelineLayoutRef) override;

  NGFXFenceRef CreateFence() override;

  NGFXCommandQueueRef CreateCommandQueue(NGFXCommandType const&) override;

  void WaitIdle() override { vkDeviceWaitIdle(m_Device); }

  void QueryTextureSubResourceLayout(NGFXTextureRef,
                                     k3d::TextureSpec const& spec,
                                     k3d::SubResourceLayout*) override;

  VkDevice const& GetRawDevice() const { return m_Device; }

  PtrCmdAlloc NewCommandAllocator(bool transient);
  bool FindMemoryType(uint32 typeBits,
                      VkFlags requirementsMask,
                      uint32* typeIndex) const;
  PtrSemaphore NewSemaphore();

  uint32 GetQueueCount() const { return m_Gpu->m_QueueProps.Count(); }

  DescriptorAllocRef NewDescriptorAllocator(uint32 maxSets,
                                            BindingArray const& bindings);
  DescriptorSetLayoutRef NewDescriptorSetLayout(BindingArray const& bindings);

  uint64 GetMaxAllocationCount();
  VkShaderModule CreateShaderModule(NGFXShaderBundle const& Bundle);
  VkQueue GetImmQueue() const { return m_ImmediateQueue; }
  VkCommandBuffer AllocateImmediateCommand();

  SpRenderpass GetOrCreateRenderPass(const k3d::RenderPassDesc& desc);
  SpFramebuffer GetOrCreateFramebuffer(const k3d::RenderPassDesc& desc);

  friend class DeviceChild;

protected:
  SpCmdQueue InitCmdQueue(VkQueueFlags queueTypes,
                          uint32 queueFamilyIndex,
                          uint32 queueIndex);

  Result Create(GpuRef const& gpu, bool withDebug);

  friend class Instance;
  friend class SwapChain;
  template<class T1, class T2> 
  friend class TVkRHIObjectBase;

private:
  CmdBufManagerRef m_CmdBufManager;

private:
  VkPhysicalDeviceMemoryProperties m_MemoryProperties = {};
  VkDevice m_Device = VK_NULL_HANDLE;

  VkQueue m_ImmediateQueue = VK_NULL_HANDLE;
  VkCommandPool m_ImmediateCmdPool = VK_NULL_HANDLE;

  GpuRef m_Gpu;
};

#define __VK_GLOBAL_LEVEL_PROC__(name) PFN_vk##name gp##name = NULL;
#define __VK_INSTANCE_LEVEL_PROC__(name) PFN_vk##name fp##name = NULL

class Instance
  : public EnableSharedFromThis<Instance>
  , public NGFXFactory
{
public:
  Instance(const ::k3d::String& engineName,
           const ::k3d::String& appName,
           bool enableValidation = true);
  ~Instance();

  void Release() override;
  // NGFXFactory::EnumDevices
  void EnumDevices(DynArray<NGFXDeviceRef>& Devices) override;
  // NGFXFactory::CreateSwapChain
  NGFXSwapChainRef CreateSwapchain(NGFXCommandQueueRef pCommandQueue,
                                    void* nPtr,
                                    k3d::SwapChainDesc&) override;

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

  PFN_vkCreateDebugReportCallbackEXT CreateDebugReport = NULL;
  PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReport = NULL;
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
  VkCommandPool ImmCmdPool() const { return m_Device->m_ImmediateCmdPool; }

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
* If VK_FENCE_CREATE_SIGNALED_BIT is set then the fence is created already
* signaled, otherwise, the fence is created in an unsignaled state.
*/
class FenceCreateInfo : public VkFenceCreateInfo
{
public:
  static VkFenceCreateInfo Create()
  {
    return { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
  }
};
/**
 * Fences are signaled by the system when work invoked by vkQueueSubmit
 * completes.
 */
class Fence : public TVkRHIObjectBase<VkFence, NGFXFence>
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

class Sampler : public TVkRHIObjectBase<VkSampler, NGFXSampler>
{
public:
  explicit Sampler(Device::Ptr pDevice, k3d::SamplerState const& sampleDesc);
  ~Sampler() override;
  k3d::SamplerState GetSamplerDesc() const;

protected:
  VkSamplerCreateInfo m_SamplerCreateInfo = {};
  k3d::SamplerState m_SamplerState;
};

/**
 * @param binding
 */
VkDescriptorSetLayoutBinding
RHIBinding2VkBinding(NGFXShaderBinding const& binding);
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
  friend class PipelineLayout;

  Options m_Options;
  VkDescriptorPool m_Pool;
};

using SpDescriptorAllocator = SharedPtr<DescriptorAllocator>;

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
};

class DescriptorSet
  : public TVkRHIObjectBase<VkDescriptorSet, NGFXBindingGroup>
{
  using Super = TVkRHIObjectBase<VkDescriptorSet, NGFXBindingGroup>;
  template<typename T>
  friend class CommandEncoder;
public:
  static DescriptorSet* CreateDescSet(SpPipelineLayout m_RootLayout,
                                      VkDescriptorSetLayout layout,
                                      BindingArray const& bindings,
                                      Device::Ptr pDevice);
  virtual ~DescriptorSet();
  void Update(uint32 bindSet, NGFXUAVRef) override;
  void Update(uint32 bindSet, NGFXSamplerRef) override;
  void Update(uint32 bindSet, NGFXResourceRef) override;
  uint32 GetSlotNum() const override;

private:
  SpPipelineLayout m_RootLayout;
  BindingArray m_Bindings;
  std::vector<VkWriteDescriptorSet> m_BoundDescriptorSet;

private:
  DescriptorSet(SpPipelineLayout m_RootLayout,
                VkDescriptorSetLayout layout,
                BindingArray const& bindings,
                Device::Ptr pDevice);

  void Initialize(VkDescriptorSetLayout layout, BindingArray const& bindings);
  void Destroy();
};

class PipelineLayout
  : public TVkRHIObjectBase<VkPipelineLayout, NGFXPipelineLayout>
  , public EnableSharedFromThis<PipelineLayout>
{
public:
  PipelineLayout(Device::Ptr pDevice, k3d::PipelineLayoutDesc const& desc);
  ~PipelineLayout() override;

  NGFXBindingGroupRef ObtainBindingGroup() override;

  VkDescriptorPool Pool() const
  {
    return m_DescAllocator->m_Pool;
  }

protected:
  void InitWithDesc(k3d::PipelineLayoutDesc const& desc);
  void Destroy();
  friend class RenderPipelineState;

private:
  BindingArray m_BindingArray;
  SpDescriptorAllocator m_DescAllocator;
  DescriptorSetLayoutRef m_DescSetLayout;
};

template<class TRHIResObj>
class TResource : public TVkRHIObjectBase<typename ResTrait<TRHIResObj>::Obj, TRHIResObj>
{
  friend class CommandBuffer;
public:
  using Super = TResource<TRHIResObj>;
  using TVkRHIObjectBase<typename ResTrait<TRHIResObj>::Obj, TRHIResObj>::m_NativeObj;
  using TVkRHIObjectBase<typename ResTrait<TRHIResObj>::Obj, TRHIResObj>::NativeDevice;
  using TVkRHIObjectBase<typename ResTrait<TRHIResObj>::Obj, TRHIResObj>::m_Device;

  explicit TResource(VkDeviceRef const& refDevice, bool SelfOwned = true)
    : TVkRHIObjectBase<typename ResTrait<TRHIResObj>::Obj, TRHIResObj>(refDevice)
    , m_MemAllocInfo{}
    , m_HostMemAddr(nullptr)
    , m_DeviceMem{ VK_NULL_HANDLE }
    , m_ResView(VK_NULL_HANDLE)
    , m_ResDesc{}
    , m_SelfOwned(SelfOwned)
  {
  }

  TResource(VkDeviceRef const& refDevice, k3d::ResourceDesc const& desc)
    : TVkRHIObjectBase<ResTrait<TRHIResObj>::Obj, TRHIResObj>(refDevice)
    , m_MemAllocInfo{}
    , m_HostMemAddr(nullptr)
    , m_DeviceMem{}
    , m_ResView(VK_NULL_HANDLE)
    , m_ResDesc(desc)
  {
  }

  virtual ~TResource()
  {
    Release();
  }

  virtual void Release() override
  {
    if (VK_NULL_HANDLE != m_ResView) {
      ResTrait<TRHIResObj>::DestroyView(NativeDevice(), m_ResView, nullptr);
      VKLOG(Info, "TResourceView Destroying.. -- 0x%0x.", m_ResView);
      m_ResView = VK_NULL_HANDLE;
    }
    if (m_SelfOwned && VK_NULL_HANDLE != m_NativeObj) {
      VKLOG(Info, "TResource Releasing.. 0x%0x. Name: %s.", m_NativeObj, m_Name.CStr());
      ResTrait<TRHIResObj>::Destroy(NativeDevice(), m_NativeObj, nullptr);
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

  void SetName(const char* Name) override
  {
    m_Name = Name;
  }

  void* Map(uint64 offset, uint64 size) override
  {
    K3D_VK_VERIFY(vkMapMemory(
      NativeDevice(), m_DeviceMem, offset, size, 0, &m_HostMemAddr));
    return m_HostMemAddr;
  }

  void UnMap() override { vkUnmapMemory(NativeDevice(), m_DeviceMem); }

  uint64 GetLocation() const override { return (uint64)m_NativeObj; }
  virtual uint64 GetSize() const override { return m_MemAllocInfo.allocationSize; }
  k3d::ResourceDesc GetDesc() const override { return m_ResDesc; }

  typedef typename ResTrait<TRHIResObj>::CreateInfo CreateInfo;
  typedef typename ResTrait<TRHIResObj>::ViewCreateInfo ViewCreateInfo;
  typedef typename ResTrait<TRHIResObj>::View ResourceView;
  typedef typename ResTrait<TRHIResObj>::DescriptorInfo ResourceDescriptorInfo;
  typedef typename ResTrait<TRHIResObj>::UsageFlags ResourceUsageFlags;
  ResourceView NativeView() const { return m_ResView; }
  ResourceDescriptorInfo DescInfo() const { return m_ResDescInfo; }

protected:
  VkMemoryAllocateInfo m_MemAllocInfo;
  VkMemoryRequirements m_MemReq;
  VkDeviceMemory m_DeviceMem;
  VkMemoryPropertyFlags m_MemoryBits = 0;
  VkAccessFlagBits m_AccessMask = VK_ACCESS_FLAG_BITS_MAX_ENUM;
  void* m_HostMemAddr;
  ResourceView m_ResView;
  ResourceUsageFlags m_ResUsageFlags = 0;
  ResourceDescriptorInfo m_ResDescInfo{};
  k3d::ResourceDesc m_ResDesc;
  NGFXResourceState m_UsageState = NGFX_RESOURCE_STATE_UNKNOWN;
  k3d::String m_Name;
  bool m_SelfOwned = true;

protected:
  void Allocate(CreateInfo const& info)
  {
    K3D_VK_VERIFY(ResTrait<TRHIResObj>::Create(
      NativeDevice(), &info, nullptr, &m_NativeObj));
    m_MemAllocInfo = {};
    m_MemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ResTrait<TRHIResObj>::GetMemoryInfo(NativeDevice(), m_NativeObj, &m_MemReq);

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

class Buffer : public TResource<NGFXBuffer>
{
public:
  typedef Buffer* Ptr;
  explicit Buffer(Device::Ptr pDevice)
    : Super(pDevice)
  {
  }
  Buffer(Device::Ptr pDevice, k3d::ResourceDesc const& desc);
  virtual ~Buffer();
  uint64 GetSize() const override { return m_ResDesc.Size; }
  void Create(size_t size);
};

class Texture : public TResource<NGFXTexture>
{
public:
  typedef ::k3d::SharedPtr<Texture> TextureRef;

  explicit Texture(Device::Ptr pDevice)
    : Super(pDevice)
  {
  }
  Texture(Device::Ptr pDevice, k3d::ResourceDesc const&);
  // For Swapchain
  Texture(k3d::ResourceDesc const& Desc,
          VkImage image,
          Device::Ptr pDevice,
          bool selfOwnShip = true);
  
  ~Texture() override;

  void BindSampler(NGFXSamplerRef sampler) override;
  NGFXSamplerRef GetSampler() const override;
  NGFXSRVRef GetResourceView() const override { return m_SRV; }
  void SetResourceView(NGFXSRVRef srv) override { m_SRV = srv; }
  NGFXResourceState GetState() const override { return Super::m_UsageState; }
  
  VkImageLayout GetImageLayout() const { return m_ImageLayout; }
  VkImageSubresourceRange GetSubResourceRange() const { return m_SubResRange; }

  void CreateResourceView();
  void CreateViewForSwapchainImage();

  friend class SwapChain;
  friend class CommandBuffer;

private:

  void InitCreateInfos();

private:
  ::k3d::SharedPtr<Sampler> m_ImageSampler;
  VkImageViewCreateInfo m_ImageViewInfo = {};
  NGFXSRVRef m_SRV;
  VkImageCreateInfo m_ImageInfo = {};
  VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkImageMemoryBarrier m_Barrier;

  VkSubresourceLayout m_SubResourceLayout = {};
  VkImageSubresourceRange m_SubResRange = {};
};

class RenderTexture : public Texture
{

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

/**
 * TODO: Need a Renderpass manager to cache all renderpasses
 * [framebuffer, attachments, depth, stencil
 */
class RenderTarget
  : public DeviceChild
  , public NGFXRenderTarget
{
public:
  RenderTarget(Device::Ptr pDevice,
               Texture::TextureRef texture,
               SpFramebuffer framebuffer,
               VkRenderPass renderpass);
  ~RenderTarget() override;

//  static void CreateFromSwapchain();

  VkFramebuffer GetFramebuffer() const;
  VkRenderPass GetRenderpass() const;
  Texture::TextureRef GetTexture() const;
  VkRect2D GetRenderArea() const;
  NGFXResourceRef GetBackBuffer() override;
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
  
  VkFramebuffer m_Framebuffer;
  VkRenderPass m_Renderpass;

  PtrSemaphore m_AcquireSemaphore;
};

class CommandQueue
  : public TVkRHIObjectBase<VkQueue, NGFXCommandQueue>
  , public EnableSharedFromThis<CommandQueue>
{
public:
  using This = TVkRHIObjectBase<VkQueue, NGFXCommandQueue>;
  CommandQueue(Device::Ptr pDevice,
               VkQueueFlags queueTypes,
               uint32 queueFamilyIndex,
               uint32 queueIndex);
  virtual ~CommandQueue();

  NGFXCommandBufferRef ObtainCommandBuffer(
    NGFXCommandReuseType const&) override;

  void Submit(const std::vector<VkCommandBuffer>& cmdBufs,
              const std::vector<VkSemaphore>& waitSemaphores,
              const std::vector<VkPipelineStageFlags>& waitStageMasks,
              VkFence fence,
              const std::vector<VkSemaphore>& signalSemaphores);

  SpCmdBuffer ObtainSecondaryCommandBuffer();

  VkResult Submit(const std::vector<VkSubmitInfo>& submits, VkFence fence);

  void Present(SpSwapChain& pSwapChain);

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
  CmdBufManagerRef m_SecondaryCmdBufferPool;
};

class CommandBuffer
  : public TVkRHIObjectBase<VkCommandBuffer, NGFXCommandBuffer>
  , public EnableSharedFromThis<CommandBuffer>
{
public:
  using This = TVkRHIObjectBase<VkCommandBuffer, NGFXCommandBuffer>;

  void Release() override;

  void Commit(NGFXFenceRef pFence) override;
  void Present(NGFXSwapChainRef pSwapChain, NGFXFenceRef pFence) override;

  void Reset() override;
  NGFXRenderCommandEncoderRef RenderCommandEncoder(
    RenderPassDesc const&) override;
  NGFXComputeCommandEncoderRef ComputeCommandEncoder() override;
  NGFXParallelRenderCommandEncoderRef ParallelRenderCommandEncoder(
    RenderPassDesc const&) override;
  void CopyTexture(const k3d::TextureCopyLocation& Dest,
                   const k3d::TextureCopyLocation& Src) override;
  void CopyBuffer(NGFXResourceRef Dest,
                  NGFXResourceRef Src,
                  k3d::CopyBufferRegion const& Region) override;
  void Transition(NGFXResourceRef pResource,
                  NGFXResourceState const&
                    State /*, k3d::EPipelineStage const& Stage*/) override;

  SpCmdQueue OwningQueue() { return m_OwningQueue; }

  friend class CommandQueue;
  template<typename T>
  friend class CommandEncoder;

protected:
  template<typename T>
  friend class k3d::TRefCountInstance;

  CommandBuffer(Device::Ptr, SpCmdQueue, VkCommandBuffer);

  bool m_Ended;
  SpCmdQueue m_OwningQueue;
  NGFXSwapChainRef m_PendingSwapChain;
};

template<typename CmdEncoderSubT>
class CommandEncoder : public CmdEncoderSubT
{
public:
  explicit CommandEncoder(SpCmdBuffer pCmd)
    : m_MasterCmd(pCmd)
  {
    m_pQueue = m_MasterCmd->OwningQueue();
  }

  void SetPipelineState(uint32 HashCode,
                        NGFXPipelineStateRef const& pPipeline) override
  {
    K3D_ASSERT(pPipeline);
    if (pPipeline->GetType() == NGFX_PIPELINE_Compute) {
      ComputePipelineState* computePso =
        static_cast<ComputePipelineState*>(pPipeline.Get());
      vkCmdBindPipeline(m_MasterCmd->NativeHandle(),
                        VK_PIPELINE_BIND_POINT_COMPUTE,
                        computePso->NativeHandle());
    } else {
      RenderPipelineState* gfxPso =
        static_cast<RenderPipelineState*>(pPipeline.Get());
      vkCmdBindPipeline(m_MasterCmd->NativeHandle(),
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        gfxPso->NativeHandle());
    }
  }

  void SetBindingGroup(NGFXBindingGroupRef const& pBindingGroup) override
  {
    K3D_ASSERT(pBindingGroup);
    auto pDescSet = StaticPointerCast<DescriptorSet>(pBindingGroup);
    VkDescriptorSet sets[] = { pDescSet->NativeHandle() };
    vkCmdBindDescriptorSets(m_MasterCmd->NativeHandle(),
                            GetBindPoint(),
                            pDescSet->m_RootLayout->NativeHandle(),
                            0, // first set
                            1,
                            sets, // set count,
                            0,
                            NULL); // dynamic offset
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
class RenderCommandEncoder : public CommandEncoder<NGFXRenderCommandEncoder>
{
public:
  using This = CommandEncoder<NGFXRenderCommandEncoder>;
  void SetScissorRect(const k3d::Rect&) override;
  void SetViewport(const k3d::ViewportDesc&) override;
  void SetIndexBuffer(const k3d::IndexBufferView& IBView) override;
  void SetVertexBuffer(uint32 Slot,
                       const k3d::VertexBufferView& VBView) override;
  void SetPrimitiveType(NGFXPrimitiveType) override;
  void DrawInstanced(k3d::DrawInstancedParam) override;
  void DrawIndexedInstanced(k3d::DrawIndexedInstancedParam) override;
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
class ComputeCommandEncoder : public CommandEncoder<NGFXComputeCommandEncoder>
{
  using Super = CommandEncoder<NGFXComputeCommandEncoder>;
public:
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
private:
  ComputeCommandEncoder(SpCmdBuffer pCmd);
};

class ParallelCommandEncoder
  : public CommandEncoder<NGFXParallelRenderCommandEncoder>
  , public EnableSharedFromThis<ParallelCommandEncoder>
{
public:
  using Super = CommandEncoder<NGFXParallelRenderCommandEncoder>;
  NGFXRenderCommandEncoderRef SubRenderCommandEncoder() override;
  void EndEncode() override;
  VkPipelineBindPoint GetBindPoint() const override
  {
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
  }
  friend class CommandBuffer;
  template<typename T>
  friend class k3d::TRefCountInstance;
private:
  ParallelCommandEncoder(SpCmdBuffer PrimaryCmdBuffer);
  void SubAllocateSecondaryCmd();
  bool m_RenderpassBegun = false;
  DynArray<SpRenderCommandEncoder> m_RecordedCmds;
};

class SwapChain : public TVkRHIObjectBase<VkSwapchainKHR, NGFXSwapChain>
{
  friend class CommandQueue;
  friend class CommandBuffer;
public:
  using Super = TVkRHIObjectBase<VkSwapchainKHR, NGFXSwapChain>;

  SwapChain(Device::Ptr pDevice, void* pWindow, k3d::SwapChainDesc& Desc);
  ~SwapChain();

  // NGFXSwapChain::Release
  void Release() override;
  // NGFXSwapChain::Resize
  void Resize(uint32 Width, uint32 Height) override;
  // NGFXSwapChain::GetCurrentTexture
  NGFXTextureRef GetCurrentTexture() override;

  void Present() override;

  void QueuePresent(SpCmdQueue pQueue, NGFXFenceRef pFence);

  void AcquireNextImage();

  void Init(void* pWindow, k3d::SwapChainDesc& Desc);

  uint32 GetPresentQueueFamilyIndex() const
  {
    return m_SelectedPresentQueueFamilyIndex;
  }

  uint32 GetBackBufferCount() const { return m_ReserveBackBufferCount; }
  VkImage GetBackImage(uint32 i) const { return m_ColorImages[i]; }
  VkExtent2D GetCurrentExtent() const { return m_CachedCreateInfo.imageExtent; }
  VkFormat GetFormat() const { return m_CachedCreateInfo.imageFormat; }

private:
  void InitSurface(void* WindowHandle);
  VkPresentModeKHR ChoosePresentMode();
  int ChooseQueueIndex();

private:

  VkSemaphore m_smpRenderFinish = VK_NULL_HANDLE;
  VkSemaphore m_smpPresent = VK_NULL_HANDLE;
  VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

  uint32 m_CurrentBufferID = 0;
  uint32 m_SelectedPresentQueueFamilyIndex = 0;

  uint32 m_ReserveBackBufferCount = 0;

  VkSwapchainCreateInfoKHR m_CachedCreateInfo;

  DynArray<NGFXTextureRef> m_Buffers;
  DynArray<VkImage> m_ColorImages;
};

class RenderPass;
class FrameBuffer
{
public:
  /**
   * create framebuffer with SwapChain ImageViews
   */
  FrameBuffer(Device::Ptr pDevice, SpRenderpass pRenderPass, k3d::RenderPassDesc const&);
  ~FrameBuffer();

  uint32 GetWidth() const { return m_Width; }
  uint32 GetHeight() const { return m_Height; }
  VkFramebuffer NativeHandle() const { return m_FrameBuffer; }

private:
  friend class RenderPass;
  WeakPtr<RenderPass> m_OwningRenderPass;
  Device::Ptr m_Device;
  VkFramebuffer m_FrameBuffer = VK_NULL_HANDLE;
  uint32 m_Width = 0;
  uint32 m_Height = 0;
  bool m_HasDepthStencil = false;
};

using PtrFrameBuffer = std::shared_ptr<FrameBuffer>;

class RenderPass : public TVkRHIObjectBase<VkRenderPass, NGFXRenderpass>
{
public:
  using Super = TVkRHIObjectBase<VkRenderPass, NGFXRenderpass>;

  RenderPass(Device::Ptr pDevice, k3d::RenderPassDesc const&);

  void Begin(VkCommandBuffer Cmd, SpFramebuffer pFramebuffer,
    k3d::RenderPassDesc const& Desc,
    VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
  void End(VkCommandBuffer Cmd);

  k3d::RenderPassDesc GetDesc() const override 
  {
    return k3d::RenderPassDesc();
  }

  ~RenderPass();

  void Release() override;

private:
  VkClearValue m_ClearVal[2];
  VkRect2D  m_DefaultArea;
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

  VkPipeline NativeHandle() const { return m_Pipeline; }

protected:
  VkPipelineShaderStageCreateInfo ConvertStageInfoFromShaderBundle(
    NGFXShaderBundle const& Bundle)
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

class RenderPipelineState : public TPipelineState<NGFXRenderPipelineState>
{
public:
  RenderPipelineState(Device::Ptr pDevice,
                      k3d::RenderPipelineStateDesc const& desc,
                      NGFXPipelineLayoutRef ppl,
                      NGFXRenderpassRef pRenderPass);
  virtual ~RenderPipelineState();

  void BindRenderPass(VkRenderPass RenderPass);

  void SetRasterizerState(const k3d::RasterizerState&) override;
  void SetBlendState(const k3d::BlendState&) override;
  void SetDepthStencilState(const k3d::DepthStencilState&) override;
  void SetSampler(NGFXSamplerRef) override;
  void SetPrimitiveTopology(const NGFXPrimitiveType) override;

  VkPipeline GetPipeline() const { return m_Pipeline; }
  void Rebuild() override;

  /**
   * TOFIX
   */
  NGFXPipelineType GetType() const override
  {
    return NGFXPipelineType::NGFX_PIPELINE_GRAPHICS;
  }

protected:
  void InitWithDesc(k3d::RenderPipelineStateDesc const& desc);
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
  WeakPtr<NGFXRenderpass> m_WeakRenderPassRef;
  WeakPtr<NGFXPipelineLayout> m_weakPipelineLayoutRef;
};

class ComputePipelineState : public TPipelineState<NGFXComputePipelineState>
{
public:
  ComputePipelineState(Device::Ptr pDevice,
                       k3d::ComputePipelineStateDesc const& desc,
                       PipelineLayout* ppl);

  ~ComputePipelineState() override {}

  NGFXPipelineType GetType() const override
  {
    return NGFXPipelineType::NGFX_PIPELINE_Compute;
  }
  void Rebuild() override;

  friend class CommandContext;

private:
  VkComputePipelineCreateInfo m_ComputeCreateInfo;
  PipelineLayout* m_PipelineLayout;
};

class ShaderResourceView
  : public TVkRHIObjectBase<VkImageView, NGFXShaderResourceView>
{
public:
  ShaderResourceView(Device::Ptr pDevice,
                     k3d::SRVDesc const& desc,
                     NGFXResourceRef gpuResource);
  ~ShaderResourceView() override;
  NGFXResourceRef GetResource() const override
  {
    return NGFXResourceRef(m_WeakResource);
  }
  k3d::SRVDesc GetDesc() const override { return m_Desc; }
  VkImageView NativeImageView() const { return m_NativeObj; }

private:
  WeakPtr<NGFXResource> m_WeakResource;
  k3d::SRVDesc m_Desc;
  VkImageViewCreateInfo m_TextureViewInfo;
};

class UnorderedAceessView : public NGFXUnorderedAccessView
{
public:
  UnorderedAceessView(Device::Ptr pDevice, k3d::UAVDesc const& Desc, const NGFXResourceRef& pResource);
  ~UnorderedAceessView() override;
  friend class DescriptorSet;
private:
  Device::Ptr m_pDevice;
  UAVDesc m_Desc;
  VkImageView m_ImageView;
  VkBufferView m_BufferView;
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

struct SemaphoreCreateInfo : public VkSemaphoreCreateInfo
{
public:
  static VkSemaphoreCreateInfo Create()
  {
    return { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
  }
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