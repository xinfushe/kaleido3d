#ifndef __UTRHIBaseApp_h__
#define __UTRHIBaseApp_h__

#include <Core/App.h>
#include <Core/AssetManager.h>
#include <Core/LogUtil.h>
#include <Core/Message.h>
#include <Interface/IRHI.h>
#include <Kaleido3D.h>

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
using namespace kMath;

class RHIAppBase : public App
{
public:
  RHIAppBase(kString const& appName,
             uint32 width,
             uint32 height,
             bool valid = false)
    : App(appName, width, height)
    , m_Width(width)
    , m_Height(height)
    , m_EnableValidation(valid)
  {
  }

  virtual ~RHIAppBase() override {}

  virtual bool OnInit() override
  {
    bool inited = App::OnInit();
    if (!inited)
      return inited;
    LoadGlslangCompiler();
    LoadRHI();
    InitializeRHIObjects();
    return true;
  }

  void Compile(const char* shaderPath,
               rhi::EShaderType const& type,
               rhi::ShaderBundle& shader);

protected:
  SharedPtr<rhi::IShModule> m_ShaderModule;
  rhi::IShCompiler::Ptr m_RHICompiler;
  SharedPtr<IModule> m_RHIModule;

  rhi::FactoryRef m_pFactory;
  rhi::DeviceRef m_pDevice;
  rhi::CommandQueueRef m_pQueue;
  rhi::SwapChainRef m_pSwapChain;
  
  uint32 m_Width;
  uint32 m_Height;

private:
  void LoadGlslangCompiler();
  void LoadRHI();
  void InitializeRHIObjects();

private:
  rhi::SwapChainDesc m_SwapChainDesc = { rhi::EPF_RGB8Unorm,
                                         m_Width,
                                         m_Height,
                                         2 };
  DynArray<rhi::DeviceRef> m_Devices;
  bool m_EnableValidation;
};

void
RHIAppBase::LoadGlslangCompiler()
{
  m_ShaderModule =
    k3d::StaticPointerCast<rhi::IShModule>(ACQUIRE_PLUGIN(ShaderCompiler));
  if (m_ShaderModule) {
#if K3DPLATFORM_OS_MAC
    m_RHICompiler = m_ShaderModule->CreateShaderCompiler(rhi::ERHI_Metal);
#else
    m_RHICompiler = m_ShaderModule->CreateShaderCompiler(rhi::ERHI_Vulkan);
#endif
  }
}

void
RHIAppBase::LoadRHI()
{
#if !(K3DPLATFORM_OS_MAC || K3DPLATFORM_OS_IOS)
  m_RHIModule = SharedPtr<IModule>(ACQUIRE_PLUGIN(RHI_Vulkan));
  auto pRHI = StaticPointerCast<IVkRHI>(m_RHIModule);
  pRHI->Initialize("RenderContext", m_EnableValidation);
  pRHI->Start();
  m_pFactory = pRHI->GetFactory();
  m_pFactory->EnumDevices(m_Devices);
  m_pDevice = m_Devices[0];
#else
  m_RHIModule = ACQUIRE_PLUGIN(RHI_Metal);
  auto pRHI = StaticPointerCast<IMetalRHI>(m_RHIModule);
  if (pRHI) {
    pRHI->Start();
    m_pDevice = pRHI->GetPrimaryDevice();
  }
#endif
}

inline void
RHIAppBase::InitializeRHIObjects()
{
  m_pQueue = m_pDevice->CreateCommandQueue(rhi::ECMD_Graphics);
  m_pSwapChain = m_pFactory->CreateSwapchain(
    m_pQueue, HostWindow()->GetHandle(), m_SwapChainDesc);

  auto cmd = m_pQueue->ObtainCommandBuffer(rhi::ECMDUsage_OneShot);
}

void
RHIAppBase::Compile(const char* shaderPath,
                    rhi::EShaderType const& type,
                    rhi::ShaderBundle& shader)
{
  IAsset* shaderFile = AssetManager::Open(shaderPath);
  if (!shaderFile) {
    KLOG(Fatal, "RHIAppBase", "Error opening %s.", shaderPath);
    return;
  }
  std::vector<char> buffer;
  uint64 len = shaderFile->GetLength();
  buffer.resize(len + 1);
  shaderFile->Read(buffer.data(), shaderFile->GetLength());
  buffer[len] = 0;
  String src(buffer.data());
  rhi::ShaderDesc desc = {
    rhi::EShFmt_Text, rhi::EShLang_HLSL, rhi::EShProfile_Modern, type, "main"
  };
  m_RHICompiler->Compile(src, desc, shader);
}

#endif
