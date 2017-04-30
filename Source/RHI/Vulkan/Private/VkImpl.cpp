#include "Public/IVkRHI.h"
#include "VkCommon.h"
#include "VkConfig.h"
#include "VkEnums.h"
#include "VkRHI.h"
#include <Core/Module.h>
#include <Core/Os.h>
#include <Core/Utils/farmhash.h>
#include <Core/WebSocket.h>
#include <Log/Public/ILogModule.h>
#include <cstdarg>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

using namespace rhi::shc;
using namespace rhi;
using namespace std;

PFN_vklogCallBack g_logCallBack = nullptr;

static thread_local char logContent[2048] = { 0 };
#define LOG_SERVER 1

#if K3DPLATFORM_OS_ANDROID
#define OutputDebugStringA(str)                                                \
  __android_log_print(ANDROID_LOG_DEBUG, "VkRHI", "%s", str);
#endif

void
VkLog(k3d::ELogLevel const& Lv, const char* tag, const char* fmt, ...)
{
  if (g_logCallBack != nullptr) {
    va_list va;
    va_start(va, fmt);
    g_logCallBack(Lv, tag, fmt, va);
    va_end(va);
  } else {
#if LOG_SERVER
    va_list va;
    va_start(va, fmt);
    vsprintf(logContent, fmt, va);
    va_end(va);

    auto logModule = k3d::StaticPointerCast<k3d::ILogModule>(
      k3d::GlobalModuleManager.FindModule("KawaLog"));
    if (logModule) {
      k3d::ILogger* logger = logModule->GetLogger(k3d::ELoggerType::EWebsocket);
      if (logger) {
        logger->Log(Lv, tag, logContent);
      }

      if (Lv >= k3d::ELogLevel::Debug) {
        logger = logModule->GetLogger(k3d::ELoggerType::EConsole);
        if (logger) {
          logger->Log(Lv, tag, logContent);
        }
      }

      if (Lv >= k3d::ELogLevel::Warn) {
        logger = logModule->GetLogger(k3d::ELoggerType::EFile);
        if (logger) {
          logger->Log(Lv, tag, logContent);
        }
      }
    }
#else
    va_list va;
    va_start(va, fmt);
    vsprintf(logContent, fmt, va);
    va_end(va);
    OutputDebugStringA(logContent);
#endif
  }
}

void
SetVkLogCallback(PFN_vklogCallBack func)
{
  g_logCallBack = func;
}

K3D_VK_BEGIN

PtrCmdAlloc
CommandAllocator::CreateAllocator(uint32 queueFamilyIndex,
                                  bool transient,
                                  Device::Ptr device)
{
  PtrCmdAlloc result =
    PtrCmdAlloc(new CommandAllocator(queueFamilyIndex, transient, device));
  return result;
}

CommandAllocator::~CommandAllocator()
{
  Destroy();
}

void
CommandAllocator::Initialize()
{
  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.queueFamilyIndex = m_FamilyIndex;
  createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (m_Transient) {
    createInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  }

  K3D_VK_VERIFY(
    vkCreateCommandPool(GetRawDevice(), &createInfo, nullptr, &m_Pool));
}

void
CommandAllocator::Destroy()
{
  if (VK_NULL_HANDLE == m_Pool || !GetRawDevice()) {
    return;
  }
  VKLOG(Info,
        "CommandAllocator destroy.. -- %d. (device:0x%0x)",
        m_Pool,
        GetRawDevice());
  vkDestroyCommandPool(GetRawDevice(), m_Pool, nullptr);
  m_Pool = VK_NULL_HANDLE;
}

CommandAllocator::CommandAllocator(uint32 queueFamilyIndex,
                                   bool transient,
                                   Device::Ptr device)
  : m_FamilyIndex(queueFamilyIndex)
  , m_Transient(transient)
  , DeviceChild(device)
{
  Initialize();
}

CommandQueue::CommandQueue(Device::Ptr pDevice,
                           VkQueueFlags queueTypes,
                           uint32 queueFamilyIndex,
                           uint32 queueIndex)
  : CommandQueue::This(pDevice)
{
  Initialize(queueTypes, queueFamilyIndex, queueIndex);
}

CommandQueue::~CommandQueue()
{
  Destroy();
}

rhi::CommandBufferRef
CommandQueue::ObtainCommandBuffer(rhi::ECommandUsageType const& Usage)
{
  VkCommandBuffer Cmd = m_ReUsableCmdBufferPool->RequestCommandBuffer();
  return MakeShared<CommandBuffer>(m_Device, SharedFromThis(), Cmd);
}

void
CommandQueue::Submit(const std::vector<VkCommandBuffer>& cmdBufs,
                     const std::vector<VkSemaphore>& waitSemaphores,
                     const std::vector<VkPipelineStageFlags>& waitStageMasks,
                     VkFence fence,
                     const std::vector<VkSemaphore>& signalSemaphores)
{
  const VkPipelineStageFlags* pWaitDstStageMask =
    (waitSemaphores.size() == waitStageMasks.size()) ? waitStageMasks.data()
                                                     : nullptr;
  VkSubmitInfo submits = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
  submits.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  submits.pWaitSemaphores =
    waitSemaphores.empty() ? nullptr : waitSemaphores.data();
  submits.pWaitDstStageMask = pWaitDstStageMask;
  submits.commandBufferCount = static_cast<uint32_t>(cmdBufs.size());
  submits.pCommandBuffers = cmdBufs.empty() ? nullptr : cmdBufs.data();
  submits.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
  submits.pSignalSemaphores =
    signalSemaphores.empty() ? nullptr : signalSemaphores.data();
  this->Submit({ submits }, fence);
}

VkResult
CommandQueue::Submit(const std::vector<VkSubmitInfo>& submits, VkFence fence)
{
  VKRHI_METHOD_TRACE
  K3D_ASSERT(!submits.empty());
  uint32_t submitCount = static_cast<uint32_t>(submits.size());
  const VkSubmitInfo* pSubmits = submits.data();
  VkResult err = vkQueueSubmit(m_NativeObj, submitCount, pSubmits, fence);
  K3D_ASSERT((err == VK_SUCCESS) || (err == VK_ERROR_DEVICE_LOST));
  return err;
}

void
CommandQueue::Present(SwapChainRef& pSwapChain)
{
  VkResult PresentResult = VK_SUCCESS;
  VkPresentInfoKHR PresentInfo = {
    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    nullptr,
    1,                               // waitSemaphoreCount
    &pSwapChain->m_PresentSemaphore, // pWaitSemaphores
    1,                               // SwapChainCount
    &pSwapChain->m_SwapChain,
    &pSwapChain->m_CurrentBufferID, // SwapChain Image Index
    &PresentResult
  };
  VkResult Ret = vkQueuePresentKHR(NativeHandle(), &PresentInfo);
}

void
CommandQueue::WaitIdle()
{
  K3D_VK_VERIFY(vkQueueWaitIdle(m_NativeObj));
}

void
CommandQueue::Initialize(VkQueueFlags queueTypes,
                         uint32 queueFamilyIndex,
                         uint32 queueIndex)
{
  m_QueueFamilyIndex = queueFamilyIndex;
  m_QueueIndex = queueIndex;
  vkGetDeviceQueue(
    m_Device->GetRawDevice(), m_QueueFamilyIndex, m_QueueIndex, &m_NativeObj);
  m_ReUsableCmdBufferPool = MakeShared<CommandBufferManager>(
    m_Device->GetRawDevice(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_QueueIndex);
}

void
CommandQueue::Destroy()
{
  if (m_NativeObj == VK_NULL_HANDLE)
    return;
  m_NativeObj = VK_NULL_HANDLE;
}

void
CommandBuffer::Release()
{
}

void
CommandBuffer::Commit()
{
  if (!m_Ended) {
    // when in presenting, check if present image is in `Present State`
    // if not, make state transition
    if (m_PendingSwapChain) {
      auto PresentImage = m_PendingSwapChain->GetCurrentTexture();
      auto ImageState = PresentImage->GetState();
      if (ImageState != rhi::ERS_Present) {
        Transition(PresentImage, rhi::ERS_Present);
      }
    }

    vkEndCommandBuffer(m_NativeObj);
    m_Ended = true;
  }
  // Submit First
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 0;
  submitInfo.pWaitSemaphores = nullptr;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_NativeObj;
  m_OwningQueue->Submit({ submitInfo }, VK_NULL_HANDLE);

  // Do Present
  if (m_PendingSwapChain) {
    auto pSwapChain = StaticPointerCast<SwapChain>(m_PendingSwapChain);
    m_OwningQueue->Present(pSwapChain);
    // After Present, Prepare For Next Frame
    pSwapChain->AcquireNextImage();
    m_PendingSwapChain.Reset();
  }
}

void
CommandBuffer::Present(rhi::SwapChainRef pSwapChain, rhi::SyncFenceRef pFence)
{
  m_PendingSwapChain = pSwapChain;
}

void
CommandBuffer::Reset()
{
  vkResetCommandBuffer(m_NativeObj, 0);
}

rhi::RenderCommandEncoderRef
CommandBuffer::RenderCommandEncoder(rhi::RenderTargetRef const&,
                                    rhi::RenderPipelineStateRef const&)
{
  return MakeShared<vk::RenderCommandEncoder>(SharedFromThis(), ECmdLevel::Primary);
}

rhi::ComputeCommandEncoderRef
CommandBuffer::ComputeCommandEncoder(rhi::ComputePipelineStateRef const&)
{
  return rhi::ComputeCommandEncoderRef();
}

rhi::ParallelRenderCommandEncoderRef
CommandBuffer::ParallelRenderCommandEncoder(rhi::RenderTargetRef const&,
                                            rhi::RenderPipelineStateRef const&)
{
  return rhi::ParallelRenderCommandEncoderRef();
}

void
CommandBuffer::CopyTexture(const rhi::TextureCopyLocation& Dest,
                           const rhi::TextureCopyLocation& Src)
{
  K3D_ASSERT(Dest.pResource && Src.pResource);
  if (Src.pResource->GetDesc().Type == rhi::EGT_Buffer &&
      Dest.pResource->GetDesc().Type != rhi::EGT_Buffer) {
    DynArray<VkBufferImageCopy> Copies;
    for (auto footprint : Src.SubResourceFootPrints) {
      VkBufferImageCopy bImgCpy = {};
      bImgCpy.bufferOffset = footprint.BufferOffSet;
      bImgCpy.imageOffset = { footprint.TOffSetX,
                              footprint.TOffSetY,
                              footprint.TOffSetZ };
      bImgCpy.imageExtent = { footprint.Footprint.Dimension.Width,
                              footprint.Footprint.Dimension.Height,
                              footprint.Footprint.Dimension.Depth };
      bImgCpy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
      Copies.Append(bImgCpy);
    }
    auto SrcBuf = StaticPointerCast<Buffer>(Src.pResource);
    auto DestImage = StaticPointerCast<Texture>(Dest.pResource);
    vkCmdCopyBufferToImage(m_NativeObj,
                           SrcBuf->NativeHandle(),
                           DestImage->NativeHandle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           Copies.Count(),
                           Copies.Data());
  }
}

void
CommandBuffer::CopyBuffer(rhi::GpuResourceRef Dest,
                          rhi::GpuResourceRef Src,
                          rhi::CopyBufferRegion const& Region)
{
  K3D_ASSERT(Dest && Src);
  K3D_ASSERT(Dest->GetDesc().Type == rhi::EGT_Buffer &&
             Src->GetDesc().Type == rhi::EGT_Buffer);
  auto DestBuf = StaticPointerCast<Buffer>(Dest);
  auto SrcBuf = StaticPointerCast<Buffer>(Src);
  vkCmdCopyBuffer(m_NativeObj,
                  SrcBuf->NativeHandle(),
                  DestBuf->NativeHandle(),
                  1,
                  (const VkBufferCopy*)&Region);
}

// TODO:
void
CommandBuffer::Transition(rhi::GpuResourceRef pResource,
                          rhi::EResourceState const& State)
{
  auto Desc = pResource->GetDesc();
  if (Desc.Type == rhi::EGT_Buffer) // Buffer Barrier
  {
    // buffer offset, size
    VkBufferMemoryBarrier BufferBarrier = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr,
    };
  } else // ImageLayout Transition
  {
    auto pTexture = StaticPointerCast<Texture>(pResource);
    VkImageLayout SrcLayout = g_ResourceState[pTexture->GetState()];
    VkImageLayout DestLayout = g_ResourceState[State];
    VkImageMemoryBarrier ImageBarrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      nullptr,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      SrcLayout,
      DestLayout,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      pTexture->NativeHandle(),
      pTexture->GetSubResourceRange()
    };
    vkCmdPipelineBarrier(m_NativeObj,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, // DependencyFlag
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &ImageBarrier);
    pTexture->m_UsageState = State;
    pTexture->m_ImageLayout = DestLayout;
  }
}

CommandBuffer::CommandBuffer(Device::Ptr pDevice,
                             SpCmdQueue pQueue,
                             VkCommandBuffer Buffer)
  : CommandBuffer::This(pDevice)
  , m_OwningQueue(pQueue)
  , m_Ended(false)
{
  m_NativeObj = Buffer;
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = nullptr;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;
  K3D_VK_VERIFY(vkBeginCommandBuffer(m_NativeObj, &beginInfo));
}


/**
* @see #RHIRect2VkRect2D
*/
void RHIRect2VkRect(const rhi::Rect & rect, VkRect2D & r2d)
{
  r2d.extent.width = rect.Right - rect.Left;
  r2d.extent.height = rect.Bottom - rect.Top;
  r2d.offset.x = rect.Left;
  r2d.offset.y = rect.Top;
}

void RenderCommandEncoder::SetScissorRect(const rhi::Rect &pRect)
{
  VkRect2D Rect2D;
  RHIRect2VkRect(pRect, Rect2D);
  vkCmdSetScissor(m_MasterCmd->NativeHandle(), 0, 1, &Rect2D);
}

void RenderCommandEncoder::SetViewport(const rhi::ViewportDesc &viewport)
{
  vkCmdSetViewport(m_MasterCmd->NativeHandle(), 0, 1, reinterpret_cast<const VkViewport*>(&viewport));
}

void RenderCommandEncoder::SetIndexBuffer(const rhi::IndexBufferView & IBView)
{
  VkBuffer buf = (VkBuffer)(IBView.BufferLocation);
  vkCmdBindIndexBuffer(m_MasterCmd->NativeHandle(), buf, 0, VK_INDEX_TYPE_UINT32);
}

void RenderCommandEncoder::SetVertexBuffer(uint32 Slot, const rhi::VertexBufferView & VBView)
{
  VkBuffer buf = (VkBuffer)(VBView.BufferLocation);
  VkDeviceSize offsets[1] = { 0 };
  vkCmdBindVertexBuffers(m_MasterCmd->NativeHandle(), Slot, 1, &buf, offsets);
}

void RenderCommandEncoder::SetPrimitiveType(rhi::EPrimitiveType)
{
}

void RenderCommandEncoder::DrawInstanced(rhi::DrawInstancedParam drawParam)
{
  vkCmdDraw(m_MasterCmd->NativeHandle(), drawParam.VertexCountPerInstance, drawParam.InstanceCount,
    drawParam.StartVertexLocation, drawParam.StartInstanceLocation);
}

void RenderCommandEncoder::DrawIndexedInstanced(rhi::DrawIndexedInstancedParam drawParam)
{
  vkCmdDrawIndexed(m_MasterCmd->NativeHandle(), drawParam.IndexCountPerInstance, drawParam.InstanceCount,
    drawParam.StartIndexLocation, drawParam.BaseVertexLocation, drawParam.StartInstanceLocation);
}

void RenderCommandEncoder::EndEncode()
{
  if (m_Level != ECmdLevel::Secondary)
  {
    vkCmdEndRenderPass(m_MasterCmd->NativeHandle());
  }
  This::EndEncode();
}

// from primary render command buffer
RenderCommandEncoder::RenderCommandEncoder(SpCmdBuffer pCmd, ECmdLevel Level)
  : RenderCommandEncoder::This(pCmd)
  , m_Level(Level)
{
  VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
  /*renderPassBeginInfo.renderPass = pRT->GetRenderpass();
  renderPassBeginInfo.renderArea = pRT->GetRenderArea();*/
  //renderPassBeginInfo.pClearValues = pRT->m_ClearValues;
  renderPassBeginInfo.clearValueCount = 2;
  //renderPassBeginInfo.framebuffer = pRT->GetFramebuffer();
  vkCmdBeginRenderPass(OwningCmd(), &renderPassBeginInfo, 
    VK_SUBPASS_CONTENTS_INLINE);
}

RenderCommandEncoder::RenderCommandEncoder(SpParallelCmdEncoder ParentEncoder, SpCmdBuffer pCmd)
  : RenderCommandEncoder::This(pCmd)
  , m_Level(ECmdLevel::Secondary)
{
  // render pass begun before allocate
}

void ComputeCommandEncoder::Dispatch(uint32 GroupCountX,
  uint32 GroupCountY,
  uint32 GroupCountZ)
{
  vkCmdDispatch(m_MasterCmd->NativeHandle(), GroupCountX, GroupCountY, GroupCountZ);
}

rhi::RenderCommandEncoderRef ParallelCommandEncoder::SubRenderCommandEncoder()
{
  if (!m_RenderpassBegun)
  {
    VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    /*renderPassBeginInfo.renderPass = pRT->GetRenderpass();
    renderPassBeginInfo.renderArea = pRT->GetRenderArea();*/
    //renderPassBeginInfo.pClearValues = pRT->m_ClearValues;
    renderPassBeginInfo.clearValueCount = 2;
    //renderPassBeginInfo.framebuffer = pRT->GetFramebuffer();
    vkCmdBeginRenderPass(m_MasterCmd->NativeHandle(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    m_RenderpassBegun = true;
  }
  // allocate secondary cmd

  return nullptr;
}

void ParallelCommandEncoder::EndEncode()
{
  DynArray<VkCommandBuffer> Cmds;
  for (auto pCmd : m_RecordedCmds)
  {
    Cmds.Append(pCmd->OwningCmd());
  }
  vkCmdExecuteCommands(m_MasterCmd->NativeHandle(), Cmds.Count(), Cmds.Data());

  // end renderpass
  if (m_RenderpassBegun)
  {
    vkCmdEndRenderPass(m_MasterCmd->NativeHandle());
  }

  This::EndEncode();
}

RenderTarget::RenderTarget(Device::Ptr pDevice,
                           rhi::RenderTargetLayout const& Layout)
  : DeviceChild(pDevice)
{
}

RenderTarget::RenderTarget(Device::Ptr pDevice,
                           Texture::TextureRef texture,
                           SpFramebuffer framebuffer,
                           VkRenderPass renderpass)
  : DeviceChild(pDevice)
  , m_RenderTexture(texture)
  , m_Framebuffer(framebuffer)
  , m_Renderpass(renderpass)
{
  m_AcquireSemaphore = PtrSemaphore(new Semaphore(pDevice));
}

RenderTarget::~RenderTarget()
{
}

VkFramebuffer
RenderTarget::GetFramebuffer() const
{
  return m_Framebuffer->Get();
}

VkRenderPass
RenderTarget::GetRenderpass() const
{
  return m_Renderpass;
}

Texture::TextureRef
RenderTarget::GetTexture() const
{
  return m_RenderTexture;
}

VkRect2D
RenderTarget::GetRenderArea() const
{
  VkRect2D renderArea = {};
  renderArea.offset = { 0, 0 };
  renderArea.extent = { m_Framebuffer->GetWidth(), m_Framebuffer->GetHeight() };
  return renderArea;
}

rhi::GpuResourceRef
RenderTarget::GetBackBuffer()
{
  return m_RenderTexture;
}

RenderPipelineState::RenderPipelineState(
  Device::Ptr pDevice,
  rhi::RenderPipelineStateDesc const& desc,
  PipelineLayout* ppl)
  : TPipelineState<rhi::IRenderPipelineState>(pDevice)
  , m_RenderPass(VK_NULL_HANDLE)
  , m_GfxCreateInfo{}
  , m_PipelineLayout(ppl)
{
  memset(&m_GfxCreateInfo, 0, sizeof(m_GfxCreateInfo));
  InitWithDesc(desc);
}

RenderPipelineState::~RenderPipelineState()
{
  Destroy();
}

void
RenderPipelineState::BindRenderPass(VkRenderPass RenderPass)
{
  this->m_RenderPass = RenderPass;
}

void
RenderPipelineState::Rebuild()
{
  if (VK_NULL_HANDLE != m_Pipeline)
    return;
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  K3D_VK_VERIFY(vkCreatePipelineCache(m_Device->GetRawDevice(),
                                      &pipelineCacheCreateInfo,
                                      nullptr,
                                      &m_PipelineCache));
  K3D_VK_VERIFY(vkCreateGraphicsPipelines(m_Device->GetRawDevice(),
                                          m_PipelineCache,
                                          1,
                                          &m_GfxCreateInfo,
                                          nullptr,
                                          &m_Pipeline));
}

#if 0
void
RenderPipelineState::SavePSO(const char* path)
{
  size_t szPSO = 0;
  K3D_VK_VERIFY(
    vkGetPipelineCacheData(GetRawDevice(), m_PipelineCache, &szPSO, nullptr));
  if (!szPSO || !path)
    return;
  DynArray<char> dataBlob;
  dataBlob.Resize(szPSO);
  vkGetPipelineCacheData(
    GetRawDevice(), m_PipelineCache, &szPSO, dataBlob.Data());
  Os::File psoCacheFile(path);
  psoCacheFile.Open(IOWrite);
  psoCacheFile.Write(dataBlob.Data(), szPSO);
  psoCacheFile.Close();
}

void
RenderPipelineState::LoadPSO(const char* path)
{
  Os::MemMapFile psoFile;
  if (!path || !psoFile.Open(path, IORead))
    return;
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  pipelineCacheCreateInfo.pInitialData = psoFile.FileData();
  pipelineCacheCreateInfo.initialDataSize = psoFile.GetSize();
  K3D_VK_VERIFY(vkCreatePipelineCache(
    GetRawDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
}
#endif
void
RenderPipelineState::SetRasterizerState(const rhi::RasterizerState& rasterState)
{
  m_RasterizationState.sType =
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // Solid polygon mode
  m_RasterizationState.polygonMode = g_FillMode[rasterState.FillMode];
  // No culling
  m_RasterizationState.cullMode = g_CullMode[rasterState.CullMode];
  m_RasterizationState.frontFace = rasterState.FrontCCW
                                     ? VK_FRONT_FACE_COUNTER_CLOCKWISE
                                     : VK_FRONT_FACE_CLOCKWISE;
  m_RasterizationState.depthClampEnable =
    rasterState.DepthClipEnable ? VK_TRUE : VK_FALSE;
  m_RasterizationState.rasterizerDiscardEnable = VK_FALSE;
  m_RasterizationState.depthBiasEnable = VK_FALSE;
  this->m_GfxCreateInfo.pRasterizationState = &m_RasterizationState;
}

// One blend attachment state
// Blending is not used in this example
// TODO
void
RenderPipelineState::SetBlendState(const rhi::BlendState& blendState)
{
  m_ColorBlendState.sType =
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
  blendAttachmentState[0].colorWriteMask = 0xf;
  blendAttachmentState[0].blendEnable = blendState.Enable ? VK_TRUE : VK_FALSE;
  m_ColorBlendState.attachmentCount = 1;
  m_ColorBlendState.pAttachments = blendAttachmentState;
  this->m_GfxCreateInfo.pColorBlendState = &m_ColorBlendState;
}

void
RenderPipelineState::SetDepthStencilState(
  const rhi::DepthStencilState& depthStencilState)
{
  m_DepthStencilState.sType =
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  m_DepthStencilState.depthTestEnable =
    depthStencilState.DepthEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.depthWriteEnable = VK_TRUE;
  m_DepthStencilState.depthCompareOp =
    g_ComparisonFunc[depthStencilState.DepthFunc];
  m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
  m_DepthStencilState.back.failOp =
    g_StencilOp[depthStencilState.BackFace.StencilFailOp];
  m_DepthStencilState.back.passOp =
    g_StencilOp[depthStencilState.BackFace.StencilPassOp];
  m_DepthStencilState.back.compareOp =
    g_ComparisonFunc[depthStencilState.BackFace.StencilFunc];
  m_DepthStencilState.stencilTestEnable =
    depthStencilState.StencilEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.front = m_DepthStencilState.back;
  this->m_GfxCreateInfo.pDepthStencilState = &m_DepthStencilState;
}

void RenderPipelineState::SetSampler(rhi::SamplerRef)
{
}

void
RenderPipelineState::SetVertexInputLayout(
  rhi::VertexDeclaration const* vertDecs,
  uint32 Count)
{
  K3D_ASSERT(vertDecs && Count > 0 && m_BindingDescriptions.empty());
  for (uint32 i = 0; i < Count; i++) {
    rhi::VertexDeclaration const& vertDec = vertDecs[i];
    VkVertexInputBindingDescription bindingDesc = {
      vertDec.BindID, vertDec.Stride, VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription attribDesc = {
      vertDec.AttributeIndex,
      vertDec.BindID,
      g_VertexFormatTable[vertDec.Format],
      vertDec.OffSet
    };
    m_BindingDescriptions.push_back(bindingDesc);
    m_AttributeDescriptions.push_back(attribDesc);
  }
  m_VertexInputState.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  m_VertexInputState.pNext = NULL;
  m_VertexInputState.vertexBindingDescriptionCount =
    (uint32)m_BindingDescriptions.size();
  m_VertexInputState.pVertexBindingDescriptions = m_BindingDescriptions.data();
  m_VertexInputState.vertexAttributeDescriptionCount =
    (uint32)m_AttributeDescriptions.size();
  m_VertexInputState.pVertexAttributeDescriptions =
    m_AttributeDescriptions.data();
  this->m_GfxCreateInfo.pVertexInputState = &m_VertexInputState;
}

void
RenderPipelineState::SetPrimitiveTopology(const rhi::EPrimitiveType Type)
{
  m_InputAssemblyState.sType =
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  m_InputAssemblyState.topology = g_PrimitiveTopology[Type];
  this->m_GfxCreateInfo.pInputAssemblyState = &m_InputAssemblyState;
}

void
RenderPipelineState::SetRenderTargetFormat(const rhi::RenderTargetFormat&)
{
}

DynArray<VkVertexInputAttributeDescription>
RHIInputAttribs(rhi::VertexInputState ia)
{
  DynArray<VkVertexInputAttributeDescription> iad;
  for (uint32 i = 0; i < rhi::VertexInputState::kMaxVertexBindings; i++) {
    auto attrib = ia.Attribs[i];
    if (attrib.Slot == rhi::VertexInputState::kInvalidValue)
      break;
    iad.Append(
      { i, attrib.Slot, g_VertexFormatTable[attrib.Format], attrib.OffSet });
  }
  return iad;
}

DynArray<VkVertexInputBindingDescription>
RHIInputLayouts(rhi::VertexInputState const& ia)
{
  DynArray<VkVertexInputBindingDescription> ibd;
  for (uint32 i = 0; i < rhi::VertexInputState::kMaxVertexLayouts; i++) {
    auto layout = ia.Layouts[i];
    if (layout.Stride == rhi::VertexInputState::kInvalidValue)
      break;
    ibd.Append({ i, layout.Stride, g_InputRates[layout.Rate] });
  }
  return ibd;
}

void
RenderPipelineState::InitWithDesc(rhi::RenderPipelineStateDesc const& desc)
{
  // setup shaders
  if (desc.VertexShader.RawData.Length() != 0) {
    auto StageInfo = ConvertStageInfoFromShaderBundle(desc.VertexShader);
    m_ShaderStageInfos.Append(StageInfo);
  }
  if (desc.PixelShader.RawData.Length() != 0) {
    auto StageInfo = ConvertStageInfoFromShaderBundle(desc.PixelShader);
    m_ShaderStageInfos.Append(StageInfo);
  }
  if (desc.GeometryShader.RawData.Length() != 0) {
    auto StageInfo = ConvertStageInfoFromShaderBundle(desc.GeometryShader);
    m_ShaderStageInfos.Append(StageInfo);
  }
  if (desc.DomainShader.RawData.Length() != 0) {
    auto StageInfo = ConvertStageInfoFromShaderBundle(desc.DomainShader);
    m_ShaderStageInfos.Append(StageInfo);
  }
  if (desc.HullShader.RawData.Length() != 0) {
    auto StageInfo = ConvertStageInfoFromShaderBundle(desc.HullShader);
    m_ShaderStageInfos.Append(StageInfo);
  }

  // Init PrimType
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    nullptr,
    0,
    g_PrimitiveTopology[desc.PrimitiveTopology],
    VK_FALSE
  };

  // Init RasterState
  VkPipelineRasterizationStateCreateInfo rasterizationState = {
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, NULL
  };
  rasterizationState.polygonMode = g_FillMode[desc.Rasterizer.FillMode];
  rasterizationState.cullMode = g_CullMode[desc.Rasterizer.CullMode];
  rasterizationState.frontFace = desc.Rasterizer.FrontCCW
                                   ? VK_FRONT_FACE_COUNTER_CLOCKWISE
                                   : VK_FRONT_FACE_CLOCKWISE;
  rasterizationState.depthClampEnable =
    desc.Rasterizer.DepthClipEnable ? VK_TRUE : VK_FALSE;
  rasterizationState.rasterizerDiscardEnable = VK_FALSE;
  rasterizationState.depthBiasEnable = VK_FALSE;
  rasterizationState.lineWidth = 1.0f;

  VkPipelineTessellationStateCreateInfo tessellation;

  // Init DepthStencilState
  VkPipelineDepthStencilStateCreateInfo depthStencilState = {
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, NULL
  };
  depthStencilState.depthTestEnable =
    desc.DepthStencil.DepthEnable ? VK_TRUE : VK_FALSE;
  depthStencilState.depthWriteEnable = VK_TRUE;
  depthStencilState.depthCompareOp =
    g_ComparisonFunc[desc.DepthStencil.DepthFunc];
  depthStencilState.depthBoundsTestEnable = VK_FALSE;
  depthStencilState.back.failOp =
    g_StencilOp[desc.DepthStencil.BackFace.StencilFailOp];
  depthStencilState.back.passOp =
    g_StencilOp[desc.DepthStencil.BackFace.StencilPassOp];
  depthStencilState.back.compareOp =
    g_ComparisonFunc[desc.DepthStencil.BackFace.StencilFunc];
  depthStencilState.stencilTestEnable =
    desc.DepthStencil.StencilEnable ? VK_TRUE : VK_FALSE;
  depthStencilState.front = depthStencilState.back;

  // Init BlendState
  VkPipelineColorBlendStateCreateInfo colorBlendState = {
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, NULL
  };
  VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
  blendAttachmentState[0].colorWriteMask = 0xf;
  blendAttachmentState[0].blendEnable = desc.Blend.Enable ? VK_TRUE : VK_FALSE;
  colorBlendState.attachmentCount = 1;
  colorBlendState.pAttachments = blendAttachmentState;

  auto IAs = RHIInputAttribs(desc.InputState);
  auto IBs = RHIInputLayouts(desc.InputState);
  VkPipelineVertexInputStateCreateInfo vertexInputState = {
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, NULL
  };
  vertexInputState.vertexBindingDescriptionCount = IBs.Count();
  vertexInputState.pVertexBindingDescriptions = IBs.Data();
  vertexInputState.vertexAttributeDescriptionCount = IAs.Count();
  vertexInputState.pVertexAttributeDescriptions = IAs.Data();

  VkPipelineViewportStateCreateInfo vpInfo = {
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
  };
  vpInfo.viewportCount = 1;
  vpInfo.scissorCount = 1;

  VkPipelineDynamicStateCreateInfo dynamicState = {
    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
  };
  std::vector<VkDynamicState> dynamicStateEnables;
  dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
  dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
  dynamicStateEnables.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
  dynamicState.pDynamicStates = dynamicStateEnables.data();
  dynamicState.dynamicStateCount = (uint32)dynamicStateEnables.size();

  VkPipelineMultisampleStateCreateInfo msInfo = {
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
  };
  msInfo.pSampleMask = NULL;
  msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  this->m_GfxCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
  this->m_GfxCreateInfo.stageCount = m_ShaderStageInfos.Count();
  this->m_GfxCreateInfo.pStages = m_ShaderStageInfos.Data();

  this->m_GfxCreateInfo.pInputAssemblyState = &inputAssemblyState;
  this->m_GfxCreateInfo.pRasterizationState = &rasterizationState;
  this->m_GfxCreateInfo.pDepthStencilState = &depthStencilState;
  this->m_GfxCreateInfo.pColorBlendState = &colorBlendState;
  this->m_GfxCreateInfo.pVertexInputState = &vertexInputState;
  this->m_GfxCreateInfo.pDynamicState = &dynamicState;
  this->m_GfxCreateInfo.pMultisampleState = &msInfo;
  this->m_GfxCreateInfo.pViewportState = &vpInfo;

#if 0
  m_RenderPass = RHIRoot::GetViewport(0)->GetRenderPass();
#endif
  this->m_GfxCreateInfo.renderPass = m_RenderPass;
  this->m_GfxCreateInfo.layout = m_PipelineLayout->NativeHandle();

  // Finalize
  Rebuild();
}

void
RenderPipelineState::Destroy()
{ /*
   for (auto iter : m_ShaderStageInfos) {
     if (iter.module) {
       vkDestroyShaderModule(, iter.module, nullptr);
     }
   }*/
}

ComputePipelineState::ComputePipelineState(
  Device::Ptr pDevice,
  rhi::ComputePipelineStateDesc const& desc,
  PipelineLayout* ppl)
  : TPipelineState<rhi::IComputePipelineState>(pDevice)
  , m_ComputeCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO }
  , m_PipelineLayout(ppl)
{
  auto stageInfo = ConvertStageInfoFromShaderBundle(desc.ComputeShader);
  m_ComputeCreateInfo.layout = ppl->NativeHandle();
  m_ComputeCreateInfo.stage = stageInfo;
  Rebuild();
}

void
ComputePipelineState::Rebuild()
{
  if (VK_NULL_HANDLE != m_Pipeline)
    return;
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  K3D_VK_VERIFY(vkCreatePipelineCache(m_Device->GetRawDevice(),
                                      &pipelineCacheCreateInfo,
                                      nullptr,
                                      &m_PipelineCache));
  K3D_VK_VERIFY(vkCreateComputePipelines(m_Device->GetRawDevice(),
                                         m_PipelineCache,
                                         1,
                                         &m_ComputeCreateInfo,
                                         nullptr,
                                         &m_Pipeline));
}

PipelineLayout::PipelineLayout(Device::Ptr pDevice,
                               rhi::PipelineLayoutDesc const& desc)
  : PipelineLayout::ThisObj(pDevice)
  , m_DescSetLayout(nullptr)
  , m_DescSet()
{
  InitWithDesc(desc);
}

PipelineLayout::~PipelineLayout()
{
  Destroy();
}

void
PipelineLayout::Destroy()
{
  if (m_NativeObj == VK_NULL_HANDLE)
    return;
  vkDestroyPipelineLayout(NativeDevice(), m_NativeObj, nullptr);
  VKLOG(Info, "PipelineLayout Destroyed . -- %0x.", m_NativeObj);
  m_NativeObj = VK_NULL_HANDLE;
}

uint64
BindingHash(Binding const& binding)
{
  return (uint64)(1 << (3 + binding.VarNumber)) | binding.VarStage;
}

bool
operator<(Binding const& lhs, Binding const& rhs)
{
  return rhs.VarStage < lhs.VarStage && rhs.VarNumber < lhs.VarNumber;
}

BindingArray
ExtractBindingsFromTable(::k3d::DynArray<Binding> const& bindings)
{
  //	merge image sampler
  std::map<uint64, Binding> bindingMap;
  for (auto const& binding : bindings) {
    uint64 hash = BindingHash(binding);
    if (bindingMap.find(hash) == bindingMap.end()) {
      bindingMap.insert({ hash, binding });
    } else // binding slot override
    {
      auto& overrideBinding = bindingMap[hash];
      if (EBindType((uint32)overrideBinding.VarType |
                    (uint32)binding.VarType) ==
          EBindType::ESamplerImageCombine) {
        overrideBinding.VarType = EBindType::ESamplerImageCombine;
      }
    }
  }

  BindingArray array;
  for (auto& p : bindingMap) {
    array.Append(RHIBinding2VkBinding(p.second));
  }
  return array;
}

void
PipelineLayout::InitWithDesc(rhi::PipelineLayoutDesc const& desc)
{
  DescriptorAllocator::Options options;
  BindingArray array = ExtractBindingsFromTable(desc.Bindings);
  auto alloc = m_Device->NewDescriptorAllocator(16, array);
  m_DescSetLayout = m_Device->NewDescriptorSetLayout(array);
  m_DescSet = rhi::DescriptorRef(DescriptorSet::CreateDescSet(
    alloc, m_DescSetLayout->GetNativeHandle(), array, m_Device));

  VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
  pPipelineLayoutCreateInfo.sType =
    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pPipelineLayoutCreateInfo.pNext = NULL;
  pPipelineLayoutCreateInfo.setLayoutCount = 1;
  pPipelineLayoutCreateInfo.pSetLayouts =
    &m_DescSetLayout->m_DescriptorSetLayout;
  K3D_VK_VERIFY(vkCreatePipelineLayout(
    NativeDevice(), &pPipelineLayoutCreateInfo, nullptr, &m_NativeObj));
}

/*
enum class EBindType
{
EUndefined		= 0,
EBlock			= 0x1,
ESampler		= 0x1 << 1,
EStorageImage	= 0x1 << 2,
EStorageBuffer	= 0x1 << 3,
EConstants		= 0x000000010
};
*/
VkDescriptorType
RHIDataType2VkType(rhi::shc::EBindType const& type)
{
  switch (type) {
    case rhi::shc::EBindType::EBlock:
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case rhi::shc::EBindType::ESampler:
      return VK_DESCRIPTOR_TYPE_SAMPLER;
    case rhi::shc::EBindType::ESampledImage:
      return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case rhi::shc::EBindType::ESamplerImageCombine:
      return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case rhi::shc::EBindType::EStorageImage:
      return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case rhi::shc::EBindType::EStorageBuffer:
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  }
  return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

VkDescriptorSetLayoutBinding
RHIBinding2VkBinding(rhi::shc::Binding const& binding)
{
  VkDescriptorSetLayoutBinding vkbinding = {};
  vkbinding.descriptorType = RHIDataType2VkType(binding.VarType);
  vkbinding.binding = binding.VarNumber;
  vkbinding.stageFlags = g_ShaderType[binding.VarStage];
  vkbinding.descriptorCount = 1;
  return vkbinding;
}

DescriptorAllocator::DescriptorAllocator(
  Device::Ptr pDevice,
  DescriptorAllocator::Options const& option,
  uint32 maxSets,
  BindingArray const& bindings)
  : DeviceChild(pDevice)
  , m_Options(option)
  , m_Pool(VK_NULL_HANDLE)
{
  Initialize(maxSets, bindings);
}

DescriptorAllocator::~DescriptorAllocator()
{
  Destroy();
}

void
DescriptorAllocator::Initialize(uint32 maxSets, BindingArray const& bindings)
{
  std::map<VkDescriptorType, uint32_t> mappedCounts;
  for (const auto& binding : bindings) {
    VkDescriptorType descType = binding.descriptorType;
    auto it = mappedCounts.find(descType);
    if (it != mappedCounts.end()) {
      ++mappedCounts[descType];
    } else {
      mappedCounts[descType] = 1;
    }
  }

  std::vector<VkDescriptorPoolSize> typeCounts;
  for (const auto& mc : mappedCounts) {
    typeCounts.push_back({ mc.first, mc.second });
  }

  VkDescriptorPoolCreateInfo createInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
  };
  createInfo.flags = m_Options.CreateFlags;
  createInfo.maxSets = 1;
  createInfo.poolSizeCount = typeCounts.size();
  createInfo.pPoolSizes = typeCounts.size() == 0 ? nullptr : typeCounts.data();
  K3D_VK_VERIFY(
    vkCreateDescriptorPool(GetRawDevice(), &createInfo, NULL, &m_Pool));
}

void
DescriptorAllocator::Destroy()
{
  if (VK_NULL_HANDLE == m_Pool || !GetRawDevice())
    return;
  vkDestroyDescriptorPool(GetRawDevice(), m_Pool, nullptr);
  m_Pool = VK_NULL_HANDLE;
  VKLOG(Info, "DescriptorAllocator-destroying vkDestroyDescriptorPool...");
}

DescriptorSetLayout::DescriptorSetLayout(Device::Ptr pDevice,
                                         BindingArray const& bindings)
  : DeviceChild(pDevice)
  , m_DescriptorSetLayout(VK_NULL_HANDLE)
{
  Initialize(bindings);
}

DescriptorSetLayout::~DescriptorSetLayout()
{
  // Destroy();
}

void
DescriptorSetLayout::Initialize(BindingArray const& bindings)
{
  VkDescriptorSetLayoutCreateInfo createInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
  };
  createInfo.bindingCount = bindings.Count();
  createInfo.pBindings = bindings.Count() == 0 ? nullptr : bindings.Data();
  K3D_VK_VERIFY(vkCreateDescriptorSetLayout(
    GetRawDevice(), &createInfo, nullptr, &m_DescriptorSetLayout));
}

void
DescriptorSetLayout::Destroy()
{
  if (VK_NULL_HANDLE == m_DescriptorSetLayout || !GetRawDevice())
    return;
  vkDestroyDescriptorSetLayout(GetRawDevice(), m_DescriptorSetLayout, nullptr);
  VKLOG(
    Info, "DescriptorSetLayout  destroying... -- %0x.", m_DescriptorSetLayout);
  m_DescriptorSetLayout = VK_NULL_HANDLE;
}

DescriptorSet::DescriptorSet(DescriptorAllocRef descriptorAllocator,
                             VkDescriptorSetLayout layout,
                             BindingArray const& bindings,
                             Device::Ptr pDevice)
  : DeviceChild(pDevice)
  , m_DescriptorAllocator(descriptorAllocator)
  , m_Bindings(bindings)
{
  Initialize(layout, bindings);
}

DescriptorSet*
DescriptorSet::CreateDescSet(DescriptorAllocRef descriptorPool,
                             VkDescriptorSetLayout layout,
                             BindingArray const& bindings,
                             Device::Ptr pDevice)
{
  return new DescriptorSet(descriptorPool, layout, bindings, pDevice);
}

DescriptorSet::~DescriptorSet()
{
  Destroy();
}

void
DescriptorSet::Update(uint32 bindSet, rhi::SamplerRef pRHISampler)
{
  auto pSampler = StaticPointerCast<Sampler>(pRHISampler);
  VkDescriptorImageInfo imageInfo = {
    pSampler->NativeHandle(),
    VK_NULL_HANDLE,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  };
  m_BoundDescriptorSet[bindSet].pImageInfo = &imageInfo;
  vkUpdateDescriptorSets(
    GetRawDevice(), 1, &m_BoundDescriptorSet[bindSet], 0, NULL);
  VKLOG(Info,
        "%s , Set (0x%0x) updated with sampler(location:0x%x).",
        __K3D_FUNC__,
        m_DescriptorSet,
        pSampler->NativeHandle());
}

void
DescriptorSet::Update(uint32 bindSet, rhi::GpuResourceRef gpuResource)
{
  auto desc = gpuResource->GetDesc();
  switch (desc.Type) {
    case rhi::EGT_Buffer: {
      VkDescriptorBufferInfo bufferInfo = {
        (VkBuffer)gpuResource->GetLocation(), 0, gpuResource->GetSize()
      };
      m_BoundDescriptorSet[bindSet].pBufferInfo = &bufferInfo;
      vkUpdateDescriptorSets(
        GetRawDevice(), 1, &m_BoundDescriptorSet[bindSet], 0, NULL);
      VKLOG(Info,
            "%s , Set (0x%0x) updated with buffer(location:0x%x, size:%d).",
            __K3D_FUNC__,
            m_DescriptorSet,
            gpuResource->GetLocation(),
            gpuResource->GetSize());
      break;
    }
    case rhi::EGT_Texture1D:
    case rhi::EGT_Texture2D:
    case rhi::EGT_Texture3D:
    case rhi::EGT_Texture2DArray: // combined/seperated image sampler should be
                                  // considered
    {
      auto pTex = k3d::StaticPointerCast<Texture>(gpuResource);
      auto rSampler = k3d::StaticPointerCast<Sampler>(pTex->GetSampler());
      assert(rSampler);
      auto srv =
        k3d::StaticPointerCast<ShaderResourceView>(pTex->GetResourceView());
      VkDescriptorImageInfo imageInfo = {
        rSampler->NativeHandle(), srv->NativeImageView(), pTex->GetImageLayout()
      }; //TODO : sampler shouldn't be null
      m_BoundDescriptorSet[bindSet].pImageInfo = &imageInfo;
      vkUpdateDescriptorSets(
        GetRawDevice(), 1, &m_BoundDescriptorSet[bindSet], 0, NULL);
      VKLOG(Info,
            "%s , Set (0x%0x) updated with image(location:0x%x, size:%d).",
            __K3D_FUNC__,
            m_DescriptorSet,
            gpuResource->GetLocation(),
            gpuResource->GetSize());
      break;
    }
  }
}

uint32
DescriptorSet::GetSlotNum() const
{
  return (uint32)m_BoundDescriptorSet.size();
}

void
DescriptorSet::Initialize(VkDescriptorSetLayout layout,
                          BindingArray const& bindings)
{
  std::vector<VkDescriptorSetLayout> layouts = { layout };
  VkDescriptorSetAllocateInfo allocInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
  };
  allocInfo.descriptorPool = m_DescriptorAllocator->m_Pool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
  allocInfo.pSetLayouts = layouts.empty() ? nullptr : layouts.data();
  K3D_VK_VERIFY(
    vkAllocateDescriptorSets(GetRawDevice(), &allocInfo, &m_DescriptorSet));
  VKLOG(Info, "%s , Set (0x%0x) created.", __K3D_FUNC__, m_DescriptorSet);

  for (auto& binding : m_Bindings) {
    VkWriteDescriptorSet entry = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    entry.dstSet = m_DescriptorSet;
    entry.descriptorCount = 1;
    entry.dstArrayElement = 0;
    entry.dstBinding = binding.binding;
    entry.descriptorType = binding.descriptorType;
    m_BoundDescriptorSet.push_back(entry);
  }
}

void
DescriptorSet::Destroy()
{
  if (VK_NULL_HANDLE == m_DescriptorSet || !GetRawDevice())
    return;

  if (m_DescriptorAllocator) {
    // const auto& options = m_DescriptorAllocator->m_Options;
    // if( options.hasFreeDescriptorSetFlag() ) {
    VkDescriptorSet descSets[1] = { m_DescriptorSet };
    vkFreeDescriptorSets(
      GetRawDevice(), m_DescriptorAllocator->m_Pool, 1, descSets);
    m_DescriptorSet = VK_NULL_HANDLE;
    //}
  }
  VKRHI_METHOD_TRACE
}

RenderPass::RenderPass(Device::Ptr pDevice, RenderpassOptions const& options)
  : DeviceChild(pDevice)
  , m_Options(options)
{
  Initialize(options);
}

RenderPass::RenderPass(Device::Ptr pDevice, RenderTargetLayout const& rtl)
  : DeviceChild(pDevice)
{
  Initialize(rtl);
}

RenderPass::~RenderPass()
{
  Destroy();
}

void
RenderPass::NextSubpass()
{
  //  m_GfxContext->NextSubpass(VK_SUBPASS_CONTENTS_INLINE);
  //  ++mSubpass;
}

void
RenderPass::Initialize(RenderpassOptions const& options)
{
  if (VK_NULL_HANDLE != m_RenderPass) {
    return;
  }

  m_Options = options;

  K3D_ASSERT(options.m_Attachments.size() > 0);
  K3D_ASSERT(options.m_Subpasses.size() > 0);

  // Populate attachment descriptors
  const size_t numAttachmentDesc = options.m_Attachments.size();
  mAttachmentDescriptors.resize(numAttachmentDesc);
  mAttachmentClearValues.resize(numAttachmentDesc);
  for (size_t i = 0; i < numAttachmentDesc; ++i) {
    mAttachmentDescriptors[i] = options.m_Attachments[i].GetDescription();
    mAttachmentClearValues[i] = options.m_Attachments[i].GetClearValue();
  }

  // Populate attachment references
  const size_t numSubPasses = options.m_Subpasses.size();
  std::vector<Subpass::AttachReferences> subPassAttachmentRefs(numSubPasses);
  for (size_t i = 0; i < numSubPasses; ++i) {
    const auto& subPass = options.m_Subpasses[i];

    // Color attachments
    {
      // Allocate elements for color attachments
      const size_t numColorAttachments = subPass.m_ColorAttachments.size();
      subPassAttachmentRefs[i].Color.resize(numColorAttachments);
      subPassAttachmentRefs[i].Resolve.resize(numColorAttachments);

      // Populate color and resolve attachments
      for (size_t j = 0; j < numColorAttachments; ++j) {
        // color
        uint32_t colorAttachmentIndex = subPass.m_ColorAttachments[j];
        VkImageLayout colorImageLayout =
          mAttachmentDescriptors[colorAttachmentIndex].initialLayout;
        ;
        subPassAttachmentRefs[i].Color[j] = {};
        subPassAttachmentRefs[i].Color[j].attachment = colorAttachmentIndex;
        subPassAttachmentRefs[i].Color[j].layout = colorImageLayout;
        // resolve
        uint32_t resolveAttachmentIndex = subPass.m_ResolveAttachments[j];
        subPassAttachmentRefs[i].Resolve[j] = {};
        subPassAttachmentRefs[i].Resolve[j].attachment = resolveAttachmentIndex;
        subPassAttachmentRefs[i].Resolve[j].layout =
          colorImageLayout; // Not a mistake, this is on purpose
      }
    }

    // Depth/stencil attachment
    std::vector<VkAttachmentReference> depthStencilAttachmentRef;
    if (!subPass.m_DepthStencilAttachment.empty()) {
      // Allocate elements for depth/stencil attachments
      subPassAttachmentRefs[i].Depth.resize(1);

      // Populate depth/stencil attachments
      uint32_t attachmentIndex = subPass.m_DepthStencilAttachment[0];
      subPassAttachmentRefs[i].Depth[0] = {};
      subPassAttachmentRefs[i].Depth[0].attachment = attachmentIndex;
      subPassAttachmentRefs[i].Depth[0].layout =
        mAttachmentDescriptors[attachmentIndex].initialLayout;
    }

    // Preserve attachments
    if (!subPass.m_PreserveAttachments.empty()) {
      subPassAttachmentRefs[i].Preserve = subPass.m_PreserveAttachments;
    }
  }

  // Populate sub passes
  std::vector<VkSubpassDescription> subPassDescs(numSubPasses);
  for (size_t i = 0; i < numSubPasses; ++i) {
    const auto& colorAttachmentRefs = subPassAttachmentRefs[i].Color;
    const auto& resolveAttachmentRefs = subPassAttachmentRefs[i].Resolve;
    const auto& depthStencilAttachmentRef = subPassAttachmentRefs[i].Depth;
    const auto& preserveAttachmentRefs = subPassAttachmentRefs[i].Preserve;

    bool noResolves = true;
    for (const auto& attachRef : resolveAttachmentRefs) {
      if (VK_ATTACHMENT_UNUSED != attachRef.attachment) {
        noResolves = false;
        break;
      }
    }

    subPassDescs[i] = {};
    auto& desc = subPassDescs[i];
    desc.pipelineBindPoint = options.m_Subpasses[i].m_PipelineBindPoint;
    desc.flags = 0;
    desc.inputAttachmentCount = 0;
    desc.pInputAttachments = nullptr;
    desc.colorAttachmentCount =
      static_cast<uint32_t>(colorAttachmentRefs.size());
    desc.pColorAttachments =
      colorAttachmentRefs.empty() ? nullptr : colorAttachmentRefs.data();
    desc.pResolveAttachments = (resolveAttachmentRefs.empty() || noResolves)
                                 ? nullptr
                                 : resolveAttachmentRefs.data();
    desc.pDepthStencilAttachment = depthStencilAttachmentRef.empty()
                                     ? nullptr
                                     : depthStencilAttachmentRef.data();
    desc.preserveAttachmentCount =
      static_cast<uint32_t>(preserveAttachmentRefs.size());
    desc.pPreserveAttachments =
      preserveAttachmentRefs.empty() ? nullptr : preserveAttachmentRefs.data();
  }

  // Cache the subpass sample counts
  for (auto& subpass : m_Options.m_Subpasses) {
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    // Look at color attachments first..
    if (!subpass.m_ColorAttachments.empty()) {
      uint32_t attachmentIndex = subpass.m_ColorAttachments[0];
      sampleCount =
        m_Options.m_Attachments[attachmentIndex].m_Description.samples;
    }
    // ..and then look at depth attachments
    if ((VK_SAMPLE_COUNT_1_BIT == sampleCount) &&
        (!subpass.m_DepthStencilAttachment.empty())) {
      uint32_t attachmentIndex = subpass.m_DepthStencilAttachment[0];
      sampleCount =
        m_Options.m_Attachments[attachmentIndex].m_Description.samples;
    }
    // Cache it
    mSubpassSampleCounts.push_back(sampleCount);
  }

  std::vector<VkSubpassDependency> dependencies;
  for (auto& subpassDep : m_Options.m_SubpassDependencies) {
    dependencies.push_back(subpassDep.m_Dependency);
  }

  // Create render pass
  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.pNext = nullptr;
  renderPassCreateInfo.attachmentCount =
    static_cast<uint32_t>(mAttachmentDescriptors.size());
  renderPassCreateInfo.pAttachments = mAttachmentDescriptors.data();
  renderPassCreateInfo.subpassCount =
    static_cast<uint32_t>(subPassDescs.size());
  renderPassCreateInfo.pSubpasses =
    subPassDescs.empty() ? nullptr : subPassDescs.data();
  renderPassCreateInfo.dependencyCount =
    static_cast<uint32_t>(dependencies.size());
  renderPassCreateInfo.pDependencies =
    dependencies.empty() ? nullptr : dependencies.data();
  K3D_VK_VERIFY(vkCreateRenderPass(
    GetRawDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass));
}

void
RenderPass::Initialize(RenderTargetLayout const& rtl)
{
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.flags = 0;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.colorAttachmentCount = rtl.GetNumColorAttachments();
  subpass.pColorAttachments = rtl.GetColorAttachmentReferences();
  subpass.pResolveAttachments = rtl.GetResolveAttachmentReferences();
  subpass.pDepthStencilAttachment = rtl.GetDepthStencilAttachmentReference();
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = nullptr;

  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.pNext = nullptr;

  renderPassCreateInfo.attachmentCount = rtl.GetNumAttachments();
  renderPassCreateInfo.pAttachments = rtl.GetAttachmentDescriptions();

  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  renderPassCreateInfo.dependencyCount = 0;
  renderPassCreateInfo.pDependencies = nullptr;
  K3D_VK_VERIFY(vkCreateRenderPass(
    GetRawDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass));
}

void
RenderPass::Destroy()
{
  if (VK_NULL_HANDLE == m_RenderPass) {
    return;
  }
  VKLOG(Info, "RenderPass destroy... -- %0x.", m_RenderPass);
  vkDestroyRenderPass(GetRawDevice(), m_RenderPass, nullptr);
  m_RenderPass = VK_NULL_HANDLE;
}

VkImageAspectFlags
DetermineAspectMask(VkFormat format)
{
  VkImageAspectFlags result = 0;
  switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT: {
      result = VK_IMAGE_ASPECT_DEPTH_BIT;
    } break;
    case VK_FORMAT_S8_UINT: {
      result = VK_IMAGE_ASPECT_STENCIL_BIT;
    } break;
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT: {
      result = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    } break;
    default: {
      result = VK_IMAGE_ASPECT_COLOR_BIT;
    } break;
  }
  return result;
}

FrameBuffer::FrameBuffer(Device::Ptr pDevice,
                         VkRenderPass renderPass,
                         FrameBuffer::Option const& op)
  : DeviceChild(pDevice)
  , m_RenderPass(renderPass)
  , m_Width(op.Width)
  , m_Height(op.Height)
{
  VKRHI_METHOD_TRACE
  for (auto& elem : op.Attachments) {
    if (elem.ImageAttachment) {
      continue;
    }
  }

  std::vector<VkImageView> attachments;
  for (const auto& elem : op.Attachments) {
    attachments.push_back(elem.ImageAttachment);
  }

  VkFramebufferCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.renderPass = m_RenderPass;
  createInfo.attachmentCount = static_cast<uint32_t>(op.Attachments.size());
  createInfo.pAttachments = attachments.data();
  createInfo.width = m_Width;
  createInfo.height = m_Height;
  createInfo.layers = 1;
  createInfo.flags = 0;
  K3D_VK_VERIFY(
    vkCreateFramebuffer(GetRawDevice(), &createInfo, nullptr, &m_FrameBuffer));
}

FrameBuffer::FrameBuffer(Device::Ptr pDevice,
                         RenderPass* renderPass,
                         RenderTargetLayout const&)
  : DeviceChild(pDevice)
{
}

FrameBuffer::~FrameBuffer()
{
  if (VK_NULL_HANDLE == m_FrameBuffer) {
    return;
  }
  VKLOG(Info, "FrameBuffer destroy... -- %0x.", m_FrameBuffer);
  vkDestroyFramebuffer(GetRawDevice(), m_FrameBuffer, nullptr);
  m_FrameBuffer = VK_NULL_HANDLE;
}

FrameBuffer::Attachment::Attachment(VkFormat format,
                                    VkSampleCountFlagBits samples)
{
  VkImageAspectFlags aspectMask = DetermineAspectMask(Format);
  if (VK_IMAGE_ASPECT_COLOR_BIT == aspectMask) {
    FormatFeatures = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
  } else {
    FormatFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
}

SwapChain::~SwapChain()
{
  Release();
}

void
SwapChain::Release()
{
  VKLOG(Info, "SwapChain Destroying..");
  if (m_SwapChain) {
    vkDestroySwapchainKHR(NativeDevice(), m_SwapChain, nullptr);
    m_SwapChain = VK_NULL_HANDLE;
  }
  if (m_Surface) {
    vkDestroySurfaceKHR(
      m_Device->m_Gpu->GetInstance()->m_Instance, m_Surface, nullptr);
    m_Surface = VK_NULL_HANDLE;
  }
  if (m_PresentSemaphore) {
    vkDestroySemaphore(NativeDevice(), m_PresentSemaphore, nullptr);
    m_PresentSemaphore = VK_NULL_HANDLE;
  }
}

void
SwapChain::Resize(uint32 Width, uint32 Height)
{
}

rhi::TextureRef
SwapChain::GetCurrentTexture()
{
  return m_Buffers[m_CurrentBufferID];
}

void
SwapChain::Present()
{
}

void
SwapChain::QueuePresent(SpCmdQueue pQueue, rhi::SyncFenceRef pFence)
{
  VkPresentInfoKHR PresentInfo = {};
}

void
SwapChain::AcquireNextImage()
{
  VkResult ret = vkAcquireNextImageKHR(NativeDevice(),
                                       m_SwapChain,
                                       UINT64_MAX,
                                       m_PresentSemaphore,
                                       VK_NULL_HANDLE,
                                       &m_CurrentBufferID);
}

static VkImageViewCreateInfo
CreateViewForSwapChain(VkImage Image, VkFormat Format)
{
  VkImageViewCreateInfo ViewCreateInfo = {
    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    nullptr,
    0,
    Image,
    VK_IMAGE_VIEW_TYPE_2D,
    Format,
    { VK_COMPONENT_SWIZZLE_R,
      VK_COMPONENT_SWIZZLE_G,
      VK_COMPONENT_SWIZZLE_B,
      VK_COMPONENT_SWIZZLE_A },
    { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
  };
  return ViewCreateInfo;
}

void
SwapChain::Init(void* pWindow, rhi::SwapChainDesc& Desc)
{
  InitSurface(pWindow);
  VkPresentModeKHR swapchainPresentMode = ChoosePresentMode();
  std::pair<VkFormat, VkColorSpaceKHR> chosenFormat = ChooseFormat(Desc.Format);
  m_SelectedPresentQueueFamilyIndex = ChooseQueueIndex();
  m_SwapchainExtent = { Desc.Width, Desc.Height };
  VkSurfaceCapabilitiesKHR surfProperties;
  K3D_VK_VERIFY(
    m_Device->m_Gpu->GetSurfaceCapabilitiesKHR(m_Surface, &surfProperties));
  m_SwapchainExtent = surfProperties.currentExtent;
  uint32 desiredNumBuffers = kMath::Clamp(Desc.NumBuffers,
                                          surfProperties.minImageCount,
                                          surfProperties.maxImageCount);
  m_DesiredBackBufferCount = desiredNumBuffers;
  InitSwapChain(m_DesiredBackBufferCount,
                chosenFormat,
                swapchainPresentMode,
                surfProperties.currentTransform);
  K3D_VK_VERIFY(vkGetSwapchainImagesKHR(
    NativeDevice(), m_SwapChain, &m_ReserveBackBufferCount, nullptr));
  m_ColorImages.resize(m_ReserveBackBufferCount);
  K3D_VK_VERIFY(vkGetSwapchainImagesKHR(NativeDevice(),
                                        m_SwapChain,
                                        &m_ReserveBackBufferCount,
                                        m_ColorImages.data()));
  for (auto ColorImage : m_ColorImages) {
    VkImageViewCreateInfo ViewCreateInfo =
      CreateViewForSwapChain(ColorImage, m_ColorAttachFmt);
    VkImageView View = VK_NULL_HANDLE;
    vkCreateImageView(NativeDevice(), &ViewCreateInfo, nullptr, &View);
    m_Buffers.Append(Texture::CreateFromSwapChain(
      ColorImage, View, ViewCreateInfo, SharedDevice()));
  }
  Desc.NumBuffers = m_ReserveBackBufferCount;
  VKLOG(
    Info,
    "[SwapChain::Initialize] desired imageCount=%d, reserved imageCount = %d.",
    m_DesiredBackBufferCount,
    m_ReserveBackBufferCount);

  VkSemaphoreCreateInfo SemaphoreInfo = {
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
  };
  vkCreateSemaphore(
    NativeDevice(), &SemaphoreInfo, nullptr, &m_PresentSemaphore);

  AcquireNextImage();
}

void
SwapChain::Initialize(void* WindowHandle, rhi::RenderViewportDesc& gfxSetting)
{
  InitSurface(WindowHandle);
  VkPresentModeKHR swapchainPresentMode = ChoosePresentMode();
  std::pair<VkFormat, VkColorSpaceKHR> chosenFormat =
    ChooseFormat(gfxSetting.ColorFormat);
  m_SelectedPresentQueueFamilyIndex = ChooseQueueIndex();
  m_SwapchainExtent = { gfxSetting.Width, gfxSetting.Height };
  VkSurfaceCapabilitiesKHR surfProperties;
  K3D_VK_VERIFY(
    m_Device->m_Gpu->GetSurfaceCapabilitiesKHR(m_Surface, &surfProperties));
  m_SwapchainExtent = surfProperties.currentExtent;
  uint32 desiredNumBuffers = kMath::Clamp(gfxSetting.BackBufferCount,
                                          surfProperties.minImageCount,
                                          surfProperties.maxImageCount);
  m_DesiredBackBufferCount = desiredNumBuffers;
  InitSwapChain(m_DesiredBackBufferCount,
                chosenFormat,
                swapchainPresentMode,
                surfProperties.currentTransform);
  K3D_VK_VERIFY(vkGetSwapchainImagesKHR(
    NativeDevice(), m_SwapChain, &m_ReserveBackBufferCount, nullptr));
  m_ColorImages.resize(m_ReserveBackBufferCount);
  K3D_VK_VERIFY(vkGetSwapchainImagesKHR(NativeDevice(),
                                        m_SwapChain,
                                        &m_ReserveBackBufferCount,
                                        m_ColorImages.data()));
  gfxSetting.BackBufferCount = m_ReserveBackBufferCount;
  VKLOG(
    Info,
    "[SwapChain::Initialize] desired imageCount=%d, reserved imageCount = %d.",
    m_DesiredBackBufferCount,
    m_ReserveBackBufferCount);
}

uint32
SwapChain::AcquireNextImage(PtrSemaphore presentSemaphore, PtrFence pFence)
{
  VkResult result = vkAcquireNextImageKHR(
    NativeDevice(),
    m_SwapChain,
    UINT64_MAX,
    presentSemaphore ? presentSemaphore->m_Semaphore : VK_NULL_HANDLE,
    pFence ? pFence->NativeHandle() : VK_NULL_HANDLE,
    &m_CurrentBufferID);
  switch (result) {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
      // OnWindowSizeChanged();
      VKLOG(Info, "Swapchain need update");
    default:
      break;
  }
  return m_CurrentBufferID;
}

VkResult
SwapChain::Present(uint32 imageIndex, PtrSemaphore renderingFinishSemaphore)
{
#if 0
  VkSemaphore renderSem = renderingFinishSemaphore
                            ? renderingFinishSemaphore->GetNativeHandle()
                            : VK_NULL_HANDLE;
  VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                   nullptr };
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.swapchainCount = m_SwapChain ? 1 : 0;
  presentInfo.pSwapchains = &m_SwapChain;
  presentInfo.waitSemaphoreCount = renderSem ? 1 : 0;
  presentInfo.pWaitSemaphores = &renderSem;
  return vkQueuePresentKHR(GetImmCmdQueue()->GetNativeHandle(), &presentInfo);
#endif
  return VK_SUCCESS;
}

void
SwapChain::InitSurface(void* WindowHandle)
{
#if K3DPLATFORM_OS_WIN
  VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {};
  SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
  SurfaceCreateInfo.hwnd = (HWND)WindowHandle;
  K3D_VK_VERIFY(
    vkCreateWin32SurfaceKHR(m_Device->m_Gpu->GetInstance()->m_Instance,
                            &SurfaceCreateInfo,
                            nullptr,
                            &m_Surface));
#elif K3DPLATFORM_OS_ANDROID
  VkAndroidSurfaceCreateInfoKHR SurfaceCreateInfo = {};
  SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  SurfaceCreateInfo.window = (ANativeWindow*)WindowHandle;
  K3D_VK_VERIFY(
    vkCreateAndroidSurfaceKHR(GetGpuRef()->GetInstance()->m_Instance,
                              &SurfaceCreateInfo,
                              nullptr,
                              &m_Surface));
#endif
}

VkPresentModeKHR
SwapChain::ChoosePresentMode()
{
  uint32_t presentModeCount;
  K3D_VK_VERIFY(m_Device->m_Gpu->GetSurfacePresentModesKHR(
    m_Surface, &presentModeCount, NULL));
  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  K3D_VK_VERIFY(m_Device->m_Gpu->GetSurfacePresentModesKHR(
    m_Surface, &presentModeCount, presentModes.data()));
  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (size_t i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
    if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
        (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
      swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
  }
  return swapchainPresentMode;
}

std::pair<VkFormat, VkColorSpaceKHR>
SwapChain::ChooseFormat(rhi::EPixelFormat& Format)
{
  uint32_t formatCount;
  K3D_VK_VERIFY(
    m_Device->m_Gpu->GetSurfaceFormatsKHR(m_Surface, &formatCount, NULL));
  std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
  K3D_VK_VERIFY(m_Device->m_Gpu->GetSurfaceFormatsKHR(
    m_Surface, &formatCount, surfFormats.data()));
  VkFormat colorFormat;
  VkColorSpaceKHR colorSpace;
  if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
    colorFormat = g_FormatTable[Format];
  } else {
    K3D_ASSERT(formatCount >= 1);
    colorFormat = surfFormats[0].format;
  }
  colorSpace = surfFormats[0].colorSpace;
  return std::make_pair(colorFormat, colorSpace);
}

int
SwapChain::ChooseQueueIndex()
{
  uint32 chosenIndex = 0;
  std::vector<VkBool32> queuePresentSupport(m_Device->GetQueueCount());

  for (uint32_t i = 0; i < m_Device->GetQueueCount(); ++i) {
    m_Device->m_Gpu->GetSurfaceSupportKHR(
      i, m_Surface, &queuePresentSupport[i]);
    if (queuePresentSupport[i]) {
      chosenIndex = i;
      break;
    }
  }

  return chosenIndex;
}

void
SwapChain::InitSwapChain(uint32 numBuffers,
                         std::pair<VkFormat, VkColorSpaceKHR> color,
                         VkPresentModeKHR mode,
                         VkSurfaceTransformFlagBitsKHR pretran)
{
  VkSwapchainCreateInfoKHR swapchainCI = {
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
  };
  swapchainCI.surface = m_Surface;
  swapchainCI.minImageCount = numBuffers;
  swapchainCI.imageFormat = color.first;
  m_ColorAttachFmt = color.first;
  swapchainCI.imageColorSpace = color.second;
  swapchainCI.imageExtent = m_SwapchainExtent;
  swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCI.preTransform = pretran;
  swapchainCI.imageArrayLayers = 1;
  swapchainCI.queueFamilyIndexCount = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCI.queueFamilyIndexCount = 0;
  swapchainCI.pQueueFamilyIndices = NULL;
  swapchainCI.presentMode = mode;
  swapchainCI.oldSwapchain = VK_NULL_HANDLE;
  swapchainCI.clipped = VK_TRUE;
  swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  K3D_VK_VERIFY(
    vkCreateSwapchainKHR(NativeDevice(), &swapchainCI, nullptr, &m_SwapChain));
  VKLOG(Info, "Init Swapchain with ColorFmt(%d)", m_ColorAttachFmt);
}
#if 0
RenderViewport::RenderViewport(Device::Ptr pDevice,
                               void* windowHandle,
                               rhi::RenderViewportDesc& setting)
  : m_NumBufferCount(-1)
  , DeviceChild(pDevice)
  , m_CurFrameId(0)
{
  if (pDevice) {
    //m_pSwapChain = pDevice->NewSwapChain(setting);
    m_pSwapChain->Initialize(windowHandle, setting);
    m_NumBufferCount = m_pSwapChain->GetBackBufferCount();
    VKLOG(
      Info,
      "RenderViewport-Initialized: width(%d), height(%d), swapImage num(%d).",
      setting.Width,
      setting.Height,
      m_NumBufferCount);
    m_PresentSemaphore = GetDevice()->NewSemaphore();
    m_RenderSemaphore = GetDevice()->NewSemaphore();
  }
}

RenderViewport::~RenderViewport()
{
  VKLOG(Info, "RenderViewport-Destroyed..");
}

bool
RenderViewport::InitViewport(void* windowHandle,
                             rhi::IDevice* pDevice,
                             rhi::RenderViewportDesc& gfxSetting)
{
  AllocateDefaultRenderPass(gfxSetting);
  AllocateRenderTargets(gfxSetting);
  return true;
}

void
RenderViewport::PrepareNextFrame()
{
  VKRHI_METHOD_TRACE
  m_CurFrameId = m_pSwapChain->AcquireNextImage(m_PresentSemaphore, nullptr);
  std::stringstream stream;
  stream << "Current Frame Id = " << m_CurFrameId << " acquiring " << std::hex
         << std::setfill('0') << m_PresentSemaphore->GetNativeHandle();
  VKLOG(Info, stream.str().c_str());
}

bool
RenderViewport::Present(bool vSync)
{
  VKRHI_METHOD_TRACE
  std::stringstream stream;
  stream << "present ----- renderSemaphore " << std::hex << std::setfill('0')
         << m_RenderSemaphore->GetNativeHandle();
  VKLOG(Info, stream.str().c_str());
  VkResult result = m_pSwapChain->Present(m_CurFrameId, m_RenderSemaphore);
  return result == VK_SUCCESS;
}

rhi::RenderTargetRef
RenderViewport::GetRenderTarget(uint32 index)
{
  return m_RenderTargets[index];
}

rhi::RenderTargetRef
RenderViewport::GetCurrentBackRenderTarget()
{
  return m_RenderTargets[m_CurFrameId];
}

uint32
RenderViewport::GetSwapChainIndex()
{
  return m_CurFrameId;
}

uint32
RenderViewport::GetSwapChainCount()
{
  return m_pSwapChain->GetBackBufferCount();
}

void
RenderViewport::AllocateDefaultRenderPass(rhi::RenderViewportDesc& gfxSetting)
{
  VKRHI_METHOD_TRACE
  VkFormat colorformat = m_pSwapChain ? m_pSwapChain->GetFormat()
                                      : g_FormatTable[gfxSetting.ColorFormat];
  RenderpassAttachment colorAttach =
    RenderpassAttachment::CreateColor(colorformat);
  colorAttach.GetDescription()
    .InitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    .FinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  RenderpassOptions option;
  option.AddAttachment(colorAttach);
  Subpass subpass_;
  subpass_.AddColorAttachment(0);
  if (gfxSetting.HasDepth) {
    VkFormat depthFormat = g_FormatTable[gfxSetting.DepthStencilFormat];
    GetGpuRef()->GetSupportedDepthFormat(&depthFormat);
    RenderpassAttachment depthAttach =
      RenderpassAttachment::CreateDepthStencil(depthFormat);
    depthAttach.GetDescription()
      .InitialLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
      .FinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    subpass_.AddDepthStencilAttachment(1);
    option.AddAttachment(depthAttach);
  }
  option.AddSubPass(subpass_);
  m_RenderPass = MakeShared<RenderPass>(GetDevice(), option);
}

void
RenderViewport::AllocateRenderTargets(rhi::RenderViewportDesc& gfxSetting)
{
  if (m_RenderPass) {
    VkFormat depthFormat = g_FormatTable[gfxSetting.DepthStencilFormat];
    GetGpuRef()->GetSupportedDepthFormat(&depthFormat);
    VkImage depthImage;
    VkImageCreateInfo image = {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = depthFormat;
    // Use example's height and width
    image.extent = { GetWidth(), GetHeight(), 1 };
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(GetRawDevice(), &image, nullptr, &depthImage);

    VkDeviceMemory depthMem;
    // Allocate memory for the image (device local) and bind it to our image
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(GetRawDevice(), depthImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetDevice()->FindMemoryType(memReqs.memoryTypeBits,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &memAlloc.memoryTypeIndex);
    vkAllocateMemory(GetRawDevice(), &memAlloc, nullptr, &depthMem);
    vkBindImageMemory(GetRawDevice(), depthImage, depthMem, 0);

    auto layoutCmd = k3d::StaticPointerCast<CommandContext>(
      GetDevice()->NewCommandContext(rhi::ECMD_Graphics));
    ImageMemoryBarrierParams params(
      depthImage,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    layoutCmd->Begin();
    params.MipLevelCount(1)
      .AspectMask(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)
      .LayerCount(1);
    layoutCmd->PipelineBarrierImageMemory(params);
    layoutCmd->End();
    layoutCmd->Execute(false);

    VkImageView depthView;
    VkImageViewCreateInfo depthStencilView = {};
    depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format = depthFormat;
    depthStencilView.subresourceRange = {};
    depthStencilView.subresourceRange.aspectMask =
      DetermineAspectMask(depthFormat);
    depthStencilView.subresourceRange.baseMipLevel = 0;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.baseArrayLayer = 0;
    depthStencilView.subresourceRange.layerCount = 1;
    depthStencilView.image = depthImage;
    vkCreateImageView(GetRawDevice(), &depthStencilView, nullptr, &depthView);

    m_RenderTargets.resize(m_NumBufferCount);
    RenderpassOptions option = m_RenderPass->GetOption();
    VkFormat colorFmt = option.GetAttachments()[0].GetFormat();
    for (uint32_t i = 0; i < m_NumBufferCount; ++i) {
      VkImage colorImage = m_pSwapChain->GetBackImage(i);
      auto colorImageInfo =
        ImageViewInfo::CreateColorImageView(GetGpuRef(), colorFmt, colorImage);
      VKLOG(
        Info, "swapchain imageView created . (0x%0x).", colorImageInfo.first);
      colorImageInfo.second.components = { VK_COMPONENT_SWIZZLE_R,
                                           VK_COMPONENT_SWIZZLE_G,
                                           VK_COMPONENT_SWIZZLE_B,
                                           VK_COMPONENT_SWIZZLE_A };
      auto colorTex = Texture::CreateFromSwapChain(
        colorImage, colorImageInfo.first, colorImageInfo.second, GetDevice());

      FrameBuffer::Attachment colorAttach(colorImageInfo.first);
      FrameBuffer::Attachment depthAttach(depthView);
      FrameBuffer::Option op;
      op.Width = GetWidth();
      op.Height = GetHeight();
      op.Attachments.push_back(colorAttach);
      op.Attachments.push_back(depthAttach);
      auto framebuffer = SpFramebuffer(
        new FrameBuffer(GetDevice(), m_RenderPass->GetPass(), op));
      m_RenderTargets[i] = k3d::MakeShared<RenderTarget>(
        GetDevice(), colorTex, framebuffer, m_RenderPass->GetPass());
    }
  }
}

VkRenderPass
RenderViewport::GetRenderPass() const
{
  return m_RenderPass->GetPass();
}
#endif
::k3d::DynArray<GpuRef>
Instance::EnumGpus()
{
  DynArray<GpuRef> gpus;
  DynArray<VkPhysicalDevice> gpuDevices;
  uint32_t gpuCount = 0;
  K3D_VK_VERIFY(vkEnumeratePhysicalDevices(m_Instance, &gpuCount, nullptr));
  // gpus.Resize(gpuCount);
  gpuDevices.Resize(gpuCount);
  K3D_VK_VERIFY(
    vkEnumeratePhysicalDevices(m_Instance, &gpuCount, gpuDevices.Data()));
  for (auto gpu : gpuDevices) {
    auto gpuRef = GpuRef(new Gpu(gpu, SharedFromThis()));
    gpus.Append(gpuRef);
  }
  return gpus;
}

// DeviceAdapter::DeviceAdapter(GpuRef const & gpu)
//	: m_Gpu(gpu)
//{
//	m_pDevice = MakeShared<Device>();
//	m_pDevice->Create(this, gpu->m_Inst->WithValidation());
//	//m_Gpu->m_Inst->AppendLogicalDevice(m_pDevice);
//}
//
// DeviceAdapter::~DeviceAdapter()
//{
//}
//
// DeviceRef DeviceAdapter::GetDevice()
//{
//	return m_pDevice;
//}

Device::Device()
  : m_Gpu(nullptr)
  , m_Device(VK_NULL_HANDLE)
{
}

Device::~Device()
{
  Release();
}

void
Device::Release()
{
  if (VK_NULL_HANDLE == m_Device)
    return;
  vkDeviceWaitIdle(m_Device);
  VKLOG(Info, "Device Destroying .  -- %0x.", m_Device);
  ///*if (!m_CachedDescriptorSetLayout.empty())
  //{
  //	m_CachedDescriptorSetLayout.erase(m_CachedDescriptorSetLayout.begin(),
  //		m_CachedDescriptorSetLayout.end());
  //}
  // if (!m_CachedDescriptorPool.empty())
  //{
  //	m_CachedDescriptorPool.erase(m_CachedDescriptorPool.begin(),
  //		m_CachedDescriptorPool.end());
  //}
  // if (!m_CachedPipelineLayout.empty())
  //{
  //	m_CachedPipelineLayout.erase(m_CachedPipelineLayout.begin(),
  //		m_CachedPipelineLayout.end());
  //}*/
  // m_PendingPass.~vector();
  // m_ResourceManager->~ResourceManager();
  // m_ContextPool->~CommandContextPool();
  if (m_CmdBufManager) {
    m_CmdBufManager->~CommandBufferManager();
  }
  vkDestroyDevice(m_Device, nullptr);
  VKLOG(Info, "Device Destroyed .  -- %0x.", m_Device);
  m_Device = VK_NULL_HANDLE;
}

IDevice::Result
Device::Create(GpuRef const& gpu, bool withDebug)
{
  m_Gpu = gpu;
  m_Device = m_Gpu->CreateLogicDevice(withDebug);
  if (m_Device) {
    // LoadVulkan(m_Gpu->m_Inst->m_Instance, m_Device);
    if (withDebug) {
      /*RHIRoot::SetupDebug(VK_DEBUG_REPORT_ERROR_BIT_EXT |
                            VK_DEBUG_REPORT_WARNING_BIT_EXT |
                            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                          DebugReportCallback);*/
    }
    // m_ResourceManager = std::make_unique<ResourceManager>(SharedFromThis(),
    // 1024, 1024);  m_ContextPool =
    // std::make_unique<CommandContextPool>(SharedFromThis());
    return rhi::IDevice::DeviceFound;
  }
  return rhi::IDevice::DeviceNotFound;
}

RenderTargetRef
Device::NewRenderTarget(rhi::RenderTargetLayout const& layout)
{
  return RenderTargetRef(new RenderTarget(SharedFromThis(), layout));
}

uint64
Device::GetMaxAllocationCount()
{
  return m_Gpu->m_Prop.limits.maxMemoryAllocationCount;
}

SpCmdQueue
Device::InitCmdQueue(VkQueueFlags queueTypes,
                     uint32 queueFamilyIndex,
                     uint32 queueIndex)
{
  return MakeShared<CommandQueue>(
    SharedFromThis(), queueTypes, queueFamilyIndex, queueIndex);
}

VkShaderModule
Device::CreateShaderModule(rhi::ShaderBundle const& Bundle)
{
  VkShaderModule shaderModule;
  VkShaderModuleCreateInfo moduleCreateInfo;

  moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleCreateInfo.pNext = NULL;

  moduleCreateInfo.codeSize = Bundle.RawData.Length();
  moduleCreateInfo.pCode = (const uint32_t*)Bundle.RawData.Data();
  moduleCreateInfo.flags = 0;
  K3D_VK_VERIFY(
    vkCreateShaderModule(m_Device, &moduleCreateInfo, NULL, &shaderModule));
  return shaderModule;
}

bool
Device::FindMemoryType(uint32 typeBits,
                       VkFlags requirementsMask,
                       uint32* typeIndex) const
{
#ifdef max
#undef max
  *typeIndex =
    std::numeric_limits<std::remove_pointer<decltype(typeIndex)>::type>::max();
#endif
  auto memProp = m_Gpu->m_MemProp;
  for (uint32_t i = 0; i < memProp.memoryTypeCount; ++i) {
    if (typeBits & 0x00000001) {
      if (requirementsMask ==
          (memProp.memoryTypes[i].propertyFlags & requirementsMask)) {
        *typeIndex = i;
        return true;
      }
    }
    typeBits >>= 1;
  }

  return false;
}
#if 0
CommandContextRef
Device::NewCommandContext(rhi::ECommandType Type)
{
  if (!m_CmdBufManager) {
    m_CmdBufManager = CmdBufManagerRef(new CommandBufferManager(
      m_Device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_Gpu->m_GraphicsQueueIndex));
  }
  return rhi::CommandContextRef(
    new CommandContext(SharedFromThis(),
                       m_CmdBufManager->RequestCommandBuffer(),
                       VK_NULL_HANDLE,
                       Type));
  // return m_ContextPool->RequestContext(Type);
}
#endif
SamplerRef
Device::NewSampler(const rhi::SamplerState& samplerDesc)
{
  return MakeShared<Sampler>(SharedFromThis(), samplerDesc);
}

rhi::PipelineLayoutKey
HashPipelineLayoutDesc(rhi::PipelineLayoutDesc const& desc)
{
  rhi::PipelineLayoutKey key;
  key.BindingKey =
    util::Hash32((const char*)desc.Bindings.Data(),
                 desc.Bindings.Count() * sizeof(rhi::shc::Binding));
  key.SetKey = util::Hash32((const char*)desc.Sets.Data(),
                            desc.Sets.Count() * sizeof(rhi::shc::Set));
  key.UniformKey =
    util::Hash32((const char*)desc.Uniforms.Data(),
                 desc.Uniforms.Count() * sizeof(rhi::shc::Uniform));
  return key;
}

rhi::PipelineLayoutRef
Device::NewPipelineLayout(rhi::PipelineLayoutDesc const& table)
{
  // Hash the table parameter here,
  // Lookup the layout by hash code
  /*rhi::PipelineLayoutKey key = HashPipelineLayoutDesc(table);
  if (m_CachedPipelineLayout.find(key) == m_CachedPipelineLayout.end())
  {
  auto plRef = rhi::PipelineLayoutRef(new PipelineLayout(this, table));
  m_CachedPipelineLayout.insert({ key, plRef });
  }*/
  return rhi::PipelineLayoutRef(new PipelineLayout(SharedFromThis(), table));
}

DescriptorAllocRef
Device::NewDescriptorAllocator(uint32 maxSets, BindingArray const& bindings)
{
  uint32 key =
    util::Hash32((const char*)bindings.Data(),
                 bindings.Count() * sizeof(VkDescriptorSetLayoutBinding));
  /*if (m_CachedDescriptorPool.find(key) == m_CachedDescriptorPool.end())
  {
  DescriptorAllocator::Options options = {};
  auto descAllocRef = DescriptorAllocRef(new DescriptorAllocator(this, options,
  maxSets, bindings)); m_CachedDescriptorPool.insert({ key, descAllocRef });
  }*/
  DescriptorAllocator::Options options = {};
  return DescriptorAllocRef(
    new DescriptorAllocator(SharedFromThis(), options, maxSets, bindings));
}

DescriptorSetLayoutRef
Device::NewDescriptorSetLayout(BindingArray const& bindings)
{
  uint32 key =
    util::Hash32((const char*)bindings.Data(),
                 bindings.Count() * sizeof(VkDescriptorSetLayoutBinding));
  /*if (m_CachedDescriptorSetLayout.find(key) ==
  m_CachedDescriptorSetLayout.end())
  {
  auto descSetLayoutRef = DescriptorSetLayoutRef(new DescriptorSetLayout(this,
  bindings)); m_CachedDescriptorSetLayout.insert({ key, descSetLayoutRef });
  }*/
  return DescriptorSetLayoutRef(
    new DescriptorSetLayout(SharedFromThis(), bindings));
}

rhi::PipelineStateRef
Device::CreateRenderPipelineState(rhi::RenderPipelineStateDesc const& Desc,
                                  rhi::PipelineLayoutRef pPipelineLayout)
{
  return MakeShared<RenderPipelineState>(
    SharedFromThis(),
    Desc,
    static_cast<PipelineLayout*>(pPipelineLayout.Get()));
}

rhi::PipelineStateRef
Device::CreateComputePipelineState(rhi::ComputePipelineStateDesc const& Desc,
                                   rhi::PipelineLayoutRef pPipelineLayout)
{
  return MakeShared<ComputePipelineState>(
    SharedFromThis(),
    Desc,
    static_cast<PipelineLayout*>(pPipelineLayout.Get()));
}

SyncFenceRef
Device::CreateFence()
{
  return MakeShared<Fence>(SharedFromThis());
}

rhi::CommandQueueRef
Device::CreateCommandQueue(rhi::ECommandType const& Type)
{
  if (Type == rhi::ECMD_Graphics) {
    return MakeShared<CommandQueue>(
      SharedFromThis(), VK_QUEUE_GRAPHICS_BIT, m_Gpu->m_GraphicsQueueIndex, 0);
  } else if (Type == rhi::ECMD_Compute) {
    return MakeShared<CommandQueue>(
      SharedFromThis(), VK_QUEUE_COMPUTE_BIT, m_Gpu->m_ComputeQueueIndex, 0);
  }
  return nullptr;
}

GpuResourceRef
Device::NewGpuResource(rhi::ResourceDesc const& Desc)
{
  rhi::IGpuResource* resource = nullptr;
  switch (Desc.Type) {
    case rhi::EGT_Buffer:
      resource = new Buffer(SharedFromThis(), Desc);
      break;
    case rhi::EGT_Texture1D:
      break;
    case rhi::EGT_Texture2D:
      resource = new Texture(SharedFromThis(), Desc);
      break;
    default:
      break;
  }
  return GpuResourceRef(resource);
}

ShaderResourceViewRef
Device::NewShaderResourceView(rhi::GpuResourceRef pRes,
                              rhi::ResourceViewDesc const& desc)
{
  return MakeShared<ShaderResourceView>(SharedFromThis(), desc, pRes);
}

PtrCmdAlloc
Device::NewCommandAllocator(bool transient)
{
  return CommandAllocator::CreateAllocator(
    m_Gpu->m_GraphicsQueueIndex, transient, SharedFromThis());
}

PtrSemaphore
Device::NewSemaphore()
{
  return std::make_shared<Semaphore>(SharedFromThis());
}

void
Device::QueryTextureSubResourceLayout(rhi::TextureRef resource,
                                      rhi::TextureResourceSpec const& spec,
                                      rhi::SubResourceLayout* layout)
{
  K3D_ASSERT(resource);
  auto texture = StaticPointerCast<Texture>(resource);
  vkGetImageSubresourceLayout(m_Device,
                              (VkImage)resource->GetLocation(),
                              (const VkImageSubresource*)&spec,
                              (VkSubresourceLayout*)layout);
}

#if K3DPLATFORM_OS_WIN
#define PLATFORM_SURFACE_EXT VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(K3DPLATFORM_OS_LINUX) && !defined(K3DPLATFORM_OS_ANDROID)
#define PLATFORM_SURFACE_EXT VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(K3DPLATFORM_OS_ANDROID)
#define PLATFORM_SURFACE_EXT VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#endif

::k3d::DynArray<VkLayerProperties> gVkLayerProps;
::k3d::DynArray<VkExtensionProperties> gVkExtProps;

Instance::Instance(const ::k3d::String& engineName,
                   const ::k3d::String& appName,
                   bool enableValidation)
  : m_EnableValidation(enableValidation)
  , m_Instance(VK_NULL_HANDLE)
  , m_DebugMsgCallback(VK_NULL_HANDLE)
{
  LoadGlobalProcs();
  EnumExtsAndLayers();
  ExtractEnabledExtsAndLayers();

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = appName.CStr();
  appInfo.pEngineName = engineName.CStr();
  appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 1);
  appInfo.engineVersion = 1;
  appInfo.applicationVersion = 0;

  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pNext = NULL;
  instanceCreateInfo.pApplicationInfo = &appInfo;
  instanceCreateInfo.enabledExtensionCount = m_EnabledExtsRaw.Count();
  instanceCreateInfo.ppEnabledExtensionNames = m_EnabledExtsRaw.Data();
  instanceCreateInfo.enabledLayerCount = m_EnabledLayersRaw.Count();
  instanceCreateInfo.ppEnabledLayerNames = m_EnabledLayersRaw.Data();

  VkResult err = vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance);
  if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
    VKLOG(Error,
          "Cannot find a compatible Vulkan installable client driver: "
          "vkCreateInstance Failure");
  } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
    VKLOG(
      Error,
      "Cannot find a specified extension library: vkCreateInstance Failure");
  } else {
    LoadInstanceProcs();
    EnumGpus();
  }
}

Instance::~Instance()
{
  Release();
}

void
Instance::Release()
{
  VKLOG(Info,
        "Instance Destroying...  -- %0x. (tid:%d)",
        m_Instance,
        Os::Thread::GetId());
  if (m_Instance) {
    FreeDebugCallback();
    vkDestroyInstance(m_Instance, nullptr);
    VKLOG(Info, "Instance Destroyed .  -- %0x.", m_Instance);
    m_Instance = VK_NULL_HANDLE;
  }
}

void
Instance::EnumDevices(DynArray<rhi::DeviceRef>& Devices)
{
  auto GpuRefs = EnumGpus();
  for (auto& gpu : GpuRefs) {
    auto pDevice = new Device;
    pDevice->Create(gpu->SharedFromThis(), WithValidation());
    Devices.Append(rhi::DeviceRef(pDevice));
  }
}

rhi::SwapChainRef
Instance::CreateSwapchain(rhi::CommandQueueRef pCommandQueue,
                          void* nPtr,
                          rhi::SwapChainDesc& Desc)
{
  SpCmdQueue pQueue = StaticPointerCast<CommandQueue>(pCommandQueue);
  return MakeShared<SwapChain>(pQueue->SharedDevice(), nPtr, Desc);
}

#define __VK_GLOBAL_PROC_GET__(name, functor)                                  \
  gp##name = reinterpret_cast<PFN_vk##name>(functor("vk" K3D_STRINGIFY(name)))

void
Instance::LoadGlobalProcs()
{
#ifdef VK_NO_PROTOTYPES
#ifdef K3DPLATFORM_OS_WIN
  static const char* LIBVULKAN = "vulkan-1.dll";
#elif defined(K3DPLATFORM_OS_MAC)
  static const char* LIBVULKAN = "libvulkan.dylib";
#else
  static const char* LIBVULKAN = "libvulkan.so";
#endif

  m_VulkanLib = MakeShared<dynlib::Lib>(LIBVULKAN);
  // load global functions
  __VK_GLOBAL_PROC_GET__(CreateInstance, m_VulkanLib->ResolveEntry);
  __VK_GLOBAL_PROC_GET__(DestroyInstance, m_VulkanLib->ResolveEntry);
  __VK_GLOBAL_PROC_GET__(EnumerateInstanceExtensionProperties,
                         m_VulkanLib->ResolveEntry);
  __VK_GLOBAL_PROC_GET__(EnumerateInstanceLayerProperties,
                         m_VulkanLib->ResolveEntry);
  __VK_GLOBAL_PROC_GET__(GetInstanceProcAddr, m_VulkanLib->ResolveEntry);
#endif
}

void
Instance::EnumExtsAndLayers()
{
  uint32 layerCount = 0;
  K3D_VK_VERIFY(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
  if (layerCount > 0) {
    gVkLayerProps.Resize(layerCount);
    K3D_VK_VERIFY(
      vkEnumerateInstanceLayerProperties(&layerCount, gVkLayerProps.Data()));
  }
  uint32 extCount = 0;
  K3D_VK_VERIFY(
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr));
  if (extCount > 0) {
    gVkExtProps.Resize(extCount);
    K3D_VK_VERIFY(vkEnumerateInstanceExtensionProperties(
      nullptr, &extCount, gVkExtProps.Data()));
  }
  VKLOG(Info,
        ">> Instance::EnumLayersAndExts <<\n\n"
        "=================================>> layerCount = %d.\n"
        "=================================>> extensionCount = %d.\n",
        layerCount,
        extCount);
}

void
Instance::ExtractEnabledExtsAndLayers()
{
  VkBool32 surfaceExtFound = 0;
  VkBool32 platformSurfaceExtFound = 0;

  for (uint32_t i = 0; i < gVkExtProps.Count(); i++) {
    if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, gVkExtProps[i].extensionName)) {
      surfaceExtFound = 1;
      m_EnabledExts.Append(VK_KHR_SURFACE_EXTENSION_NAME);
      m_EnabledExtsRaw.Append(VK_KHR_SURFACE_EXTENSION_NAME);
    }
    if (!strcmp(PLATFORM_SURFACE_EXT, gVkExtProps[i].extensionName)) {
      platformSurfaceExtFound = 1;
      m_EnabledExts.Append(PLATFORM_SURFACE_EXT);
      m_EnabledExtsRaw.Append(PLATFORM_SURFACE_EXT);
    }
    if (m_EnableValidation) {
      m_EnabledExts.Append(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
      m_EnabledExtsRaw.Append(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
    VKLOG(Info, "available extension : %s .", gVkExtProps[i].extensionName);
  }
  if (!surfaceExtFound) {
    VKLOG(Error,
          "vkEnumerateInstanceExtensionProperties failed to find "
          "the " VK_KHR_SURFACE_EXTENSION_NAME " extension.");
  }
  if (!platformSurfaceExtFound) {
    VKLOG(Error,
          "vkEnumerateInstanceExtensionProperties failed to find "
          "the " PLATFORM_SURFACE_EXT " extension.");
  }

  if (m_EnableValidation && !gVkLayerProps.empty()) {
    for (auto prop : gVkLayerProps) {
      if (strcmp(prop.layerName, g_ValidationLayerNames[0]) ==
          0 /*|| strcmp(prop.layerName, g_ValidationLayerNames[1])==0*/) {
        m_EnabledLayers.Append(prop.layerName);
        m_EnabledLayersRaw.Append(g_ValidationLayerNames[0]);
        VKLOG(Info, "enable validation layer [%s].", prop.layerName);
        break;
      }
    }
  }
}

void
Instance::LoadInstanceProcs()
{
#ifdef VK_NO_PROTOTYPES
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceSurfaceSupportKHR);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceSurfaceFormatsKHR);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceSurfacePresentModesKHR);

  GET_INSTANCE_PROC_ADDR(m_Instance, EnumeratePhysicalDevices);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceFeatures);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceProperties);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceMemoryProperties);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceFormatProperties);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceQueueFamilyProperties);
  GET_INSTANCE_PROC_ADDR(m_Instance, CreateDevice);
  GET_INSTANCE_PROC_ADDR(m_Instance, GetDeviceProcAddr);

#if K3DPLATFORM_OS_WIN
  GET_INSTANCE_PROC_ADDR(m_Instance, CreateWin32SurfaceKHR);
#elif K3DPLATFORM_OS_ANDROID
  GET_INSTANCE_PROC_ADDR(m_Instance, CreateAndroidSurfaceKHR);
#endif
  GET_INSTANCE_PROC_ADDR(m_Instance, DestroySurfaceKHR);
#endif
}

PFN_vkDebugReportMessageEXT dbgBreakCallback;

static VkDebugReportCallbackEXT msgCallback = NULL;

const char*
DebugLevelString(VkDebugReportFlagsEXT lev)
{
  switch (lev) {
#define STR(r)                                                                 \
  case VK_DEBUG_REPORT_##r##_BIT_EXT:                                          \
    return #r
    STR(INFORMATION);
    STR(WARNING);
    STR(PERFORMANCE_WARNING);
    STR(ERROR);
    STR(DEBUG);
#undef STR
    default:
      return "UNKNOWN_ERROR";
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugReportCallback(VkDebugReportFlagsEXT flags,
                    VkDebugReportObjectTypeEXT objectType,
                    uint64_t object,
                    size_t location,
                    int32_t messageCode,
                    const char* pLayerPrefix,
                    const char* pMessage,
                    void* pUserData)
{
  static char msg[4096] = { 0 };
  switch (flags) {
    case VK_DEBUG_REPORT_WARNING_BIT_EXT:
    case VK_DEBUG_REPORT_ERROR_BIT_EXT:
      VKLOG(Error, "[%s]\t%s\t[location] %d", pLayerPrefix, pMessage, location);
      break;
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
      VKLOG(Info, "[%s]\t%s\t[location] %d", pLayerPrefix, pMessage, location);
      break;
    case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
      VKLOG(Debug, "[%s]\t%s\t[location] %d", pLayerPrefix, pMessage, location);
      break;
    default:
      VKLOG(
        Default, "[%s]\t%s\t[location] %d", pLayerPrefix, pMessage, location);
      break;
  }
  return VK_TRUE;
}

#define __VK_GLOBAL_PROC_GET__(name, functor)                                  \
  fp##name = reinterpret_cast<PFN_vk##name>(functor("vk" K3D_STRINGIFY(name)))

void
Instance::SetupDebugging(VkDebugReportFlagsEXT flags,
                         PFN_vkDebugReportCallbackEXT callBack)
{
  if (!m_Instance) {
    VKLOG(Error, "SetupDebugging Failed. (m_Instance == null)");
    return;
  }
#ifdef VK_NO_PROTOTYPES
  __VK_GLOBAL_PROC_GET__(CreateDebugReportCallbackEXT,
                         m_VulkanLib->ResolveEntry);
  if (!vkCreateDebugReportCallbackEXT)
    GET_INSTANCE_PROC_ADDR(m_Instance, CreateDebugReportCallbackEXT);
  __VK_GLOBAL_PROC_GET__(DestroyDebugReportCallbackEXT,
                         m_VulkanLib->ResolveEntry);
  if (!fpDestroyDebugReportCallbackEXT)
    GET_INSTANCE_PROC_ADDR(m_Instance, DestroyDebugReportCallbackEXT);
#endif
  VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
  dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
  dbgCreateInfo.pNext = NULL;
  dbgCreateInfo.pfnCallback = callBack;
  dbgCreateInfo.pUserData = NULL;
  dbgCreateInfo.flags = flags;
  // K3D_VK_VERIFY(vkCreateDebugReportCallbackEXT(m_Instance, &dbgCreateInfo,
  // NULL, &m_DebugMsgCallback));
}

void
Instance::FreeDebugCallback()
{
  if (m_Instance && m_DebugMsgCallback) {
    // vkDestroyDebugReportCallbackEXT(m_Instance, m_DebugMsgCallback, nullptr);
    m_DebugMsgCallback = VK_NULL_HANDLE;
  }
}

RenderViewportSp RHIRoot::s_Vp;

void
RHIRoot::AddViewport(RenderViewportSp vp)
{
  s_Vp = vp;
}

RenderViewportSp
RHIRoot::GetViewport(int index)
{
  return s_Vp;
}

void
RHIRoot::EnumLayersAndExts()
{
  // Enum Layers
  uint32 layerCount = 0;
  K3D_VK_VERIFY(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
  if (layerCount > 0) {
    gVkLayerProps.Resize(layerCount);
    K3D_VK_VERIFY(
      vkEnumerateInstanceLayerProperties(&layerCount, gVkLayerProps.Data()));
  }

  // Enum Extensions
  uint32 extCount = 0;
  K3D_VK_VERIFY(
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr));
  if (extCount > 0) {
    gVkExtProps.Resize(extCount);
    K3D_VK_VERIFY(vkEnumerateInstanceExtensionProperties(
      nullptr, &extCount, gVkExtProps.Data()));
  }
  VKLOG(Info,
        ">> RHIRoot::EnumLayersAndExts <<\n\n"
        "=================================>> layerCount = %d.\n"
        "=================================>> extensionCount = %d.\n",
        layerCount,
        extCount);
}

class RHIImpl
  : public ::k3d::IVkRHI
  , public ::k3d::EnableSharedFromThis<RHIImpl>
{
public:
  RHIImpl() {}

  ~RHIImpl() override
  {
    VKLOG(Info, "Destroying..");
    Shutdown();
  }

  void Initialize(const char* appName, bool debug) override
  {
    if (!m_Instance) {
      m_Instance = k3d::MakeShared<k3d::vk::Instance>(appName, appName, debug);
    }
  }

  rhi::FactoryRef GetFactory() { return m_Instance; }

  void Start() override
  {
    if (!m_Instance) {
      VKLOG(Fatal,
            "RHI Module uninitialized!! You should call Initialize first!");
      return;
    }
  }

  void Shutdown() override { KLOG(Info, VKRHI, "Shuting down..."); }

  const char* Name() { return "RHI_Vulkan"; }

private:
  k3d::vk::InstanceRef m_Instance;
};

K3D_VK_END

// RHI Exposed Interface
MODULE_IMPLEMENT(RHI_Vulkan, k3d::vk::RHIImpl)