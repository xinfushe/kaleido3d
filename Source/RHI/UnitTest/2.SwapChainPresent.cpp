#include <Core/App.h>
#include <Core/Message.h>
#include <Interface/IRHI.h>
#include <Kaleido3D.h>
#include <Renderer/Render.h>
#if K3DPLATFORM_OS_WIN
#include <RHI/Vulkan/Public/IVkRHI.h>
#include <RHI/Vulkan/VkCommon.h>
#elif K3DPLATFORM_OS_MAC || K3DPLATFORM_OS_IOS
#include <RHI/Metal/Common.h>
#include <RHI/Metal/Public/IMetalRHI.h>
#else
#include <RHI/Vulkan/Public/IVkRHI.h>
#include <RHI/Vulkan/VkCommon.h>
#endif

using namespace k3d;
using namespace render;

rhi::RenderViewportDesc setting{1920, 1080, rhi::EPF_RGBA8Unorm, rhi::EPF_D32Float,
                        true, 2};

class SwapchainPresent : public App {
public:
  explicit SwapchainPresent(kString const &appName)
      : App(appName, 1920, 1080) {}
  SwapchainPresent(kString const &appName, uint32 width, uint32 height)
      : App(appName, width, height) {}

  bool OnInit() override;
  void OnDestroy() override;
  void OnProcess(Message &msg) override;

private:
  SharedPtr<IModule> m_pRHI;
  rhi::FactoryRef m_pFactory;

  void InitRHIObjects();

  rhi::DeviceRef m_pDevice;
  rhi::CommandQueueRef m_pCmdQueue;
  rhi::SwapChainRef m_pSwapChain;
  rhi::SyncFenceRef m_pFrameFence;
};

K3D_APP_MAIN(SwapchainPresent);

bool SwapchainPresent::OnInit() {
  App::OnInit();
#if !(K3DPLATFORM_OS_MAC || K3DPLATFORM_OS_IOS)
  m_pRHI = ACQUIRE_PLUGIN(RHI_Vulkan);
  auto pVkRHI = k3d::StaticPointerCast<IVkRHI>(m_pRHI);
  pVkRHI->Initialize("SwapChainTest", false);
  pVkRHI->Start();
  m_pFactory = pVkRHI->GetFactory();
  DynArray<rhi::DeviceRef> Devices;
  m_pFactory->EnumDevices(Devices);
  m_pDevice = Devices[0];
#else
  m_pRHI = ACQUIRE_PLUGIN(RHI_Metal);
  auto pMtlRHI = k3d::StaticPointerCast<IMetalRHI>(m_pRHI);
  if (pMtlRHI) {
    pMtlRHI->Start();
    m_pDevice = pMtlRHI->GetPrimaryDevice();
  }
#endif
  InitRHIObjects();
  return true;
}

void SwapchainPresent::OnDestroy() {
  m_pRHI->Shutdown();
}

void SwapchainPresent::OnProcess(Message &msg) {
	auto cmdBuffer = m_pCmdQueue->ObtainCommandBuffer(rhi::ECMDUsage_OneShot);
	auto presentImage = m_pSwapChain->GetCurrentTexture();
	cmdBuffer->Transition(presentImage, rhi::ERS_Present);
	cmdBuffer->Present(m_pSwapChain, nullptr);
	cmdBuffer->Commit();
	cmdBuffer->Release();
}

void SwapchainPresent::InitRHIObjects()
{
	m_pCmdQueue = m_pDevice->CreateCommandQueue(rhi::ECMD_Graphics);
	rhi::SwapChainDesc desc = { rhi::EPF_RGBA8Unorm, 1920, 1080, 2 };
	m_pSwapChain = m_pFactory->CreateSwapchain(m_pCmdQueue, HostWindow()->GetHandle(), desc);
  m_pFrameFence = m_pDevice->CreateFence();
}
