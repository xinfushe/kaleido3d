#ifndef __UTRHIBaseApp_h__
#define __UTRHIBaseApp_h__

#include <Core/App.h>
#include <Core/AssetManager.h>
#include <Core/LogUtil.h>
#include <Core/Os.h>
#include <Core/Message.h>
#include <Interface/IRHI.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
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

struct MVPMatrix
{
  Mat4f projectionMatrix;
  Mat4f modelMatrix;
  Mat4f viewMatrix;
};

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

  virtual ~RHIAppBase() override 
  {
  }

  virtual bool OnInit() override
  {
    bool inited = App::OnInit();
    if (!inited)
      return inited;
    LoadGlslangCompiler();
    LoadConfig();
    LoadRHI();
    InitializeRHIObjects();
    return true;
  }

  void Compile(const char* shaderPath,
               k3d::EShaderType const& type,
               k3d::ShaderBundle& shader);


  template <typename ElementT>
  GpuResourceRef AllocateConstBuffer(std::function<void(ElementT*ptr)> InitFunction, uint64 Count)
  {
    ResourceDesc desc;
    desc.Type = EGT_Buffer;
    desc.Flag = EGRAF_HostVisible | EGRAF_HostCoherent;
    desc.ViewType = EGVT_CBV;
    desc.Size = Count * sizeof(ElementT);
    auto pResource = m_pDevice->CreateResource(desc);
    ElementT* ptr = (ElementT*)pResource->Map(0, sizeof(ElementT) * Count);
    InitFunction(ptr);
    pResource->UnMap();
    return pResource;
  }


  template <typename ElementT>
  GpuResourceRef AllocateConstBuffer(ElementT Value)
  {
    ResourceDesc desc;
    desc.Type = EGT_Buffer;
    desc.Flag = EGRAF_HostVisible | EGRAF_HostCoherent;
    desc.ViewType = EGVT_CBV;
    desc.Size = sizeof(ElementT);
    auto pResource = m_pDevice->CreateResource(desc);
    ElementT* ptr = (ElementT*)pResource->Map(0, sizeof(ElementT));
    *ptr = Value;
    pResource->UnMap();
    return pResource;
  }

  using Thread = std::thread;
  typedef SharedPtr<Thread> ThreadPtr;
  typedef std::pair<ThreadPtr, GpuResourceRef> AsyncResourceRef;
  /**
   * Async Load Buffers
   */
  template <typename VertexElementT>
  AsyncResourceRef AllocateVertexBuffer(std::function<void(VertexElementT *ptr)> InitFunction, uint64 Count)
  {
    ResourceDesc Desc;
    Desc.Type = EGT_Buffer;
    Desc.ViewType = EGVT_VBV;
    Desc.Flag = EGRAF_DeviceVisible;
    Desc.CreationFlag = EGRCF_TransferDst;
    Desc.Size = Count * sizeof(VertexElementT);
    auto pDevice = m_pDevice;
    auto pQueue = m_pQueue;
    auto DeviceBuffer = m_pDevice->CreateResource(Desc);
    auto UploadThread = MakeShared<Thread>([=]() {
      ResourceDesc HostDesc;
      HostDesc.ViewType = EGVT_Undefined;
      HostDesc.Flag = EGRAF_HostVisible;
      HostDesc.Size = Count * sizeof(VertexElementT);
      HostDesc.CreationFlag = EGRCF_TransferSrc;
      auto HostBuffer = pDevice->CreateResource(HostDesc);
      VertexElementT* Ptr = (VertexElementT*)HostBuffer->Map(0, Count * sizeof(VertexElementT));
      InitFunction(Ptr);
      HostBuffer->UnMap();
      auto copyCmd = pQueue->ObtainCommandBuffer(ECMDUsage_OneShot);
      copyCmd->CopyBuffer(DeviceBuffer, HostBuffer, CopyBufferRegion{ 0, 0, HostDesc.Size });
      copyCmd->Commit();
    }/*, "VertexUploadThread"*/);
    //UploadThread->Start();
    return std::make_pair(UploadThread, (DeviceBuffer));
  }

protected:
  SharedPtr<k3d::IShModule> m_ShaderModule;
  k3d::IShCompiler::Ptr m_RHICompiler;
  SharedPtr<IModule> m_RHIModule;

  k3d::FactoryRef m_pFactory;
  k3d::DeviceRef m_pDevice;
  k3d::CommandQueueRef m_pQueue;
  k3d::SwapChainRef m_pSwapChain;
  
  k3d::SyncFenceRef m_pFence;

  uint32 m_Width;
  uint32 m_Height;

private:
  void LoadGlslangCompiler();
  void LoadConfig();
  void LoadRHI();
  void InitializeRHIObjects();

private:
  k3d::SwapChainDesc m_SwapChainDesc = { k3d::EPF_RGBA8Unorm_sRGB,
                                         m_Width,
                                         m_Height,
                                         2 };
  DynArray<k3d::DeviceRef> m_Devices;
  bool m_EnableValidation;
};

void
RHIAppBase::LoadGlslangCompiler()
{
  m_ShaderModule =
    k3d::StaticPointerCast<k3d::IShModule>(ACQUIRE_PLUGIN(ShaderCompiler));
  if (m_ShaderModule) {
#if K3DPLATFORM_OS_MAC
    m_RHICompiler = m_ShaderModule->CreateShaderCompiler(k3d::ERHI_Metal);
#else
    m_RHICompiler = m_ShaderModule->CreateShaderCompiler(k3d::ERHI_Vulkan);
#endif
  }
}

inline void RHIAppBase::LoadConfig()
{
  if (Os::Exists(KT("RHI_Config.json")))
  {
    Os::File file("RHI_Config.json");
    if(!file.Open(IORead))
      return;
    auto Length = file.GetSize();
    DynArray<char> Data;
    Data.Resize(Length+1);
    file.Read(Data.Data(), Length);
    Data[Length] = 0;
    file.Close();
    using rapidjson::Document;
    Document doc;
    doc.Parse<0>(Data.Data());
    if (doc.HasParseError()) 
    {
      rapidjson::ParseErrorCode code = doc.GetParseError();
      KLOG(Fatal, ConfigParse, "Error %d.", code);
      return;
    }
    using rapidjson::Value;
    Value & v = doc["EnableValidation"];
    m_EnableValidation = v.GetBool();
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
  m_pQueue = m_pDevice->CreateCommandQueue(k3d::ECMD_Graphics);
  m_pSwapChain = m_pFactory->CreateSwapchain(
    m_pQueue, HostWindow()->GetHandle(), m_SwapChainDesc);
  m_pFence = m_pDevice->CreateFence();
  auto cmd = m_pQueue->ObtainCommandBuffer(k3d::ECMDUsage_OneShot);
}

void
RHIAppBase::Compile(const char* shaderPath,
                    k3d::EShaderType const& type,
                    k3d::ShaderBundle& shader)
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
  k3d::ShaderDesc desc = {
    k3d::EShFmt_Text, k3d::EShLang_HLSL, k3d::EShProfile_Modern, type, "main"
  };
  m_RHICompiler->Compile(src, desc, shader);
}

#endif
