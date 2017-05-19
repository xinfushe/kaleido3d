#include <Base/TextureObject.h>
#include <Base/UTRHIAppBase.h>
#include <Core/App.h>
#include <Core/AssetManager.h>
#include <Core/LogUtil.h>
#include <Core/Message.h>
#include <Kaleido3D.h>
#include <Math/kMath.hpp>
#include <Renderer/Render.h>
//#include <steam/steam_api.h>

using namespace k3d;
using namespace render;
using namespace kMath;

class CubeMesh;

struct ConstantBuffer
{
  Mat4f projectionMatrix;
  Mat4f modelMatrix;
  Mat4f viewMatrix;
};

class TCubeUnitTest : public RHIAppBase
{
public:
  TCubeUnitTest(kString const& appName, uint32 width, uint32 height)
    : RHIAppBase(appName, width, height)
  {
  }

  explicit TCubeUnitTest(kString const& appName)
    : RHIAppBase(appName, 1920, 1080)
  {
  }

  ~TCubeUnitTest() { KLOG(Info, CubeTest, "Destroying.."); }
  bool OnInit() override;
  void OnDestroy() override;
  void OnProcess(Message& msg) override;

  void OnUpdate() override;

protected:
  k3d::GpuResourceRef CreateStageBuffer(uint64 size);
  void LoadTexture();
  void PrepareResource();
  void PreparePipeline();
  void PrepareCommandBuffer();

private:
  SharedPtr<CubeMesh> m_CubeMesh;
  k3d::GpuResourceRef m_ConstBuffer;
  SharedPtr<TextureObject> m_Texture;
  ConstantBuffer m_HostBuffer;

  k3d::PipelineStateRef m_pPso;
  k3d::PipelineLayoutRef m_pl;
  k3d::BindingGroupRef m_BindingGroup;
  k3d::SyncFenceRef m_pFence;
};

K3D_APP_MAIN(TCubeUnitTest);

class CubeMesh
{
public:
  struct Vertex
  {
    float pos[3];
    float col[4];
    float uv[2];
  };
  typedef std::vector<Vertex> VertexList;
  typedef std::vector<uint32> IndiceList;

  explicit CubeMesh(k3d::DeviceRef device)
    : m_pDevice(device)
    , vbuf(nullptr)
  {
    m_szVBuf = sizeof(CubeMesh::Vertex) * m_VertexBuffer.size();
    m_IAState.Attribs[0] = { k3d::EVF_Float3x32, 0, 0 };
    m_IAState.Attribs[1] = { k3d::EVF_Float4x32, sizeof(float) * 3, 0 };
    m_IAState.Attribs[2] = { k3d::EVF_Float2x32, sizeof(float) * 7, 0 };

    m_IAState.Layouts[0] = { k3d::EVIR_PerVertex, sizeof(Vertex) };
  }

  ~CubeMesh() {}

  const k3d::VertexInputState& GetInputState() const { return m_IAState; }

  void Upload();

  void SetLoc(uint64 vbo) { vboLoc = vbo; }

  const k3d::VertexBufferView VBO() const
  {
    return k3d::VertexBufferView{ vboLoc, 0, 0 };
  }

private:
  k3d::VertexInputState m_IAState;

  uint64 m_szVBuf;

  VertexList m_VertexBuffer = {
    // left
    { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
    { -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    { -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },

    // back
    { 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
    { -1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
    { -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    { 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f },
    { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f },

    // top
    { 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },

    { 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    { -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
    { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f },

    // front
    { -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
    { -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f },
    { 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },

    { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    { -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },

    // right
    { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f },

    // bottom
    { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    { -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },

    { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
    { -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
    { -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f },
  };

  k3d::GpuResourceRef vbuf;
  uint64 vboLoc;
  k3d::DeviceRef m_pDevice;
};

void
CubeMesh::Upload()
{
  // create stage buffers
  k3d::ResourceDesc stageDesc;
  stageDesc.ViewType = k3d::EGpuMemViewType::EGVT_Undefined;
  stageDesc.CreationFlag = k3d::EGpuResourceCreationFlag::EGRCF_TransferSrc;
  stageDesc.Flag = k3d::EGpuResourceAccessFlag::EGRAF_HostCoherent |
                   k3d::EGpuResourceAccessFlag::EGRAF_HostVisible;
  stageDesc.Size = m_szVBuf;
  auto vStageBuf = m_pDevice->CreateResource(stageDesc);
  void* ptr = vStageBuf->Map(0, m_szVBuf);
  memcpy(ptr, &m_VertexBuffer[0], m_szVBuf);
  vStageBuf->UnMap();

  k3d::ResourceDesc bufferDesc;
  bufferDesc.ViewType = k3d::EGpuMemViewType::EGVT_VBV;
  bufferDesc.Size = m_szVBuf;
  bufferDesc.CreationFlag = k3d::EGpuResourceCreationFlag::EGRCF_TransferDst;
  bufferDesc.Flag = k3d::EGpuResourceAccessFlag::EGRAF_DeviceVisible;
  vbuf = m_pDevice->CreateResource(bufferDesc);

  auto pQueue = m_pDevice->CreateCommandQueue(k3d::ECMD_Graphics);
  auto cmdBuf = pQueue->ObtainCommandBuffer(k3d::ECMDUsage_OneShot);
  k3d::CopyBufferRegion region = { 0, 0, m_szVBuf };
  cmdBuf->CopyBuffer(vbuf, vStageBuf, region);
  cmdBuf->Commit();
  m_pDevice->WaitIdle();
  SetLoc(vbuf->GetLocation());
  KLOG(Info, TCubeMesh, "finish buffer upload..");
}

bool
TCubeUnitTest::OnInit()
{
  bool inited = RHIAppBase::OnInit();
  if (!inited)
    return inited;
#if 0
  bool res = SteamAPI_RestartAppIfNecessary(0);
  bool init = SteamAPI_Init();
  auto controller = SteamController();
  controller->Init();
  ControllerHandle_t pHandles[STEAM_CONTROLLER_MAX_COUNT];
  int nNumActive = SteamController()->GetConnectedControllers(pHandles);
#endif
  KLOG(Info, Test, __K3D_FUNC__);
  PrepareResource();
  PreparePipeline();
  PrepareCommandBuffer();
  return true;
}

k3d::GpuResourceRef
TCubeUnitTest::CreateStageBuffer(uint64 size)
{
  k3d::ResourceDesc stageDesc;
  stageDesc.ViewType = k3d::EGpuMemViewType::EGVT_Undefined;
  stageDesc.CreationFlag = k3d::EGpuResourceCreationFlag::EGRCF_TransferSrc;
  stageDesc.Flag = k3d::EGpuResourceAccessFlag::EGRAF_HostCoherent |
                   k3d::EGpuResourceAccessFlag::EGRAF_HostVisible;
  stageDesc.Size = size;
  return m_pDevice->CreateResource(stageDesc);
}

void
TCubeUnitTest::LoadTexture()
{
  IAsset* textureFile = AssetManager::Open("asset://Test/bricks.tga");
  if (textureFile) {
    uint64 length = textureFile->GetLength();
    uint8* data = new uint8[length];
    textureFile->Read(data, length);
    m_Texture = MakeShared<TextureObject>(m_pDevice, data, false);
    auto texStageBuf = CreateStageBuffer(m_Texture->GetSize());
    m_Texture->MapIntoBuffer(texStageBuf);
    m_Texture->CopyAndInitTexture(texStageBuf);
    k3d::SRVDesc viewDesc;
    auto srv = m_pDevice->CreateShaderResourceView(m_Texture->GetResource(), viewDesc);
    auto texure = StaticPointerCast<k3d::ITexture>(m_Texture->GetResource()); //
    texure->SetResourceView(Move(srv)); // here
    k3d::SamplerState samplerDesc;
    auto sampler2D = m_pDevice->CreateSampler(samplerDesc);
    texure->BindSampler(Move(sampler2D));
  }
}

void
TCubeUnitTest::PrepareResource()
{
  KLOG(Info, Test, __K3D_FUNC__);
  m_CubeMesh = MakeShared<CubeMesh>(m_pDevice);
  m_CubeMesh->Upload();
  LoadTexture();
  k3d::ResourceDesc desc;
  desc.Flag = k3d::EGpuResourceAccessFlag::EGRAF_HostVisible;
  desc.ViewType = k3d::EGpuMemViewType::EGVT_CBV;
  desc.Size = sizeof(ConstantBuffer);
  m_ConstBuffer = m_pDevice->CreateResource(desc);
  MVPMatrix* ptr = (MVPMatrix*)m_ConstBuffer->Map(0, sizeof(ConstantBuffer));
  (*ptr).projectionMatrix = Perspective(60.0f, 1920.f / 1080.f, 0.1f, 256.0f);
  (*ptr).viewMatrix = Translate(Vec3f(0.0f, 0.0f, -4.5f), MakeIdentityMatrix<float>());
  (*ptr).modelMatrix = MakeIdentityMatrix<float>();
  static auto angle = 60.f;
#if K3DPLATFORM_OS_ANDROID
  angle += 0.1f;
#else
  angle += 0.001f;
#endif
  (*ptr).modelMatrix = Rotate(Vec3f(1.0f, 0.0f, 0.0f), angle, (*ptr).modelMatrix);
  (*ptr).modelMatrix = Rotate(Vec3f(0.0f, 1.0f, 0.0f), angle, (*ptr).modelMatrix);
  (*ptr).modelMatrix = Rotate(Vec3f(0.0f, 0.0f, 1.0f), angle, (*ptr).modelMatrix);
  m_ConstBuffer->UnMap();
}

void
TCubeUnitTest::PreparePipeline()
{
  k3d::ShaderBundle vertSh, fragSh;
  Compile("asset://Test/cube.vert", k3d::ES_Vertex, vertSh);
  Compile("asset://Test/cube.frag", k3d::ES_Fragment, fragSh);
  auto vertBinding = vertSh.BindingTable;
  auto fragBinding = fragSh.BindingTable;
  auto mergedBindings = vertBinding | fragBinding;
  m_pl = m_pDevice->CreatePipelineLayout(mergedBindings);
  m_BindingGroup = m_pl->ObtainBindingGroup();
  m_BindingGroup->Update(0, m_ConstBuffer);
  m_BindingGroup->Update(1, m_Texture->GetResource());

  k3d::RenderPipelineStateDesc desc;
  desc.AttachmentsBlend.Append(AttachmentState());
  desc.VertexShader = vertSh;
  desc.PixelShader = fragSh;
  desc.InputState = m_CubeMesh->GetInputState();

  k3d::ColorAttachmentDesc ColorAttach;
  ColorAttach.pTexture = m_pSwapChain->GetCurrentTexture();
  ColorAttach.LoadAction = k3d::ELA_Clear;
  ColorAttach.StoreAction = k3d::ESA_Store;
  ColorAttach.ClearColor = Vec4f(1, 1, 1, 1);

  k3d::RenderPassDesc Desc;
  Desc.ColorAttachments.Append(ColorAttach);
  auto pRenderPass = m_pDevice->CreateRenderPass(Desc);

  m_pPso = m_pDevice->CreateRenderPipelineState(desc, m_pl, pRenderPass);
}

void
TCubeUnitTest::PrepareCommandBuffer()
{
  m_pFence = m_pDevice->CreateFence();
}

void
TCubeUnitTest::OnDestroy()
{
  m_pFence->WaitFor(1000);
  RHIAppBase::OnDestroy();
}

void
TCubeUnitTest::OnProcess(Message& msg)
{
}

void
TCubeUnitTest::OnUpdate()
{
  MVPMatrix* ptr = (MVPMatrix*)m_ConstBuffer->Map(0, sizeof(ConstantBuffer));
  (*ptr).projectionMatrix = Perspective(60.0f, 1920.f / 1080.f, 0.1f, 256.0f);
  (*ptr).viewMatrix = Translate(Vec3f(0.0f, 0.0f, -4.5f), MakeIdentityMatrix<float>());
  (*ptr).modelMatrix = MakeIdentityMatrix<float>();
  static auto angle = 60.f;
#if K3DPLATFORM_OS_ANDROID
  angle += 0.1f;
#else
  angle += 0.001f;
#endif
  (*ptr).modelMatrix = Rotate(Vec3f(1.0f, 0.0f, 0.0f), angle, (*ptr).modelMatrix);
  (*ptr).modelMatrix = Rotate(Vec3f(0.0f, 1.0f, 0.0f), angle, (*ptr).modelMatrix);
  (*ptr).modelMatrix = Rotate(Vec3f(0.0f, 0.0f, 1.0f), angle, (*ptr).modelMatrix);
  m_ConstBuffer->UnMap();

  auto currentImage = m_pSwapChain->GetCurrentTexture();
  auto ImageDesc = currentImage->GetDesc();
  k3d::ColorAttachmentDesc ColorAttach;
  ColorAttach.pTexture = currentImage;
  ColorAttach.LoadAction = k3d::ELA_Clear;
  ColorAttach.StoreAction = k3d::ESA_Store;
  ColorAttach.ClearColor = Vec4f(1, 1, 1, 1);

  k3d::RenderPassDesc Desc;
  Desc.ColorAttachments.Append(ColorAttach);

  auto commandBuffer = m_pQueue->ObtainCommandBuffer(k3d::ECMDUsage_OneShot);
  // command encoder like Metal does, should use desc instead, look obj from cache
  auto renderCmd = commandBuffer->RenderCommandEncoder(Desc);
  renderCmd->SetBindingGroup(m_BindingGroup);
  k3d::Rect rect{ 0, 0, ImageDesc.TextureDesc.Width, ImageDesc.TextureDesc.Height };
  renderCmd->SetScissorRect(rect);
  renderCmd->SetViewport(k3d::ViewportDesc(ImageDesc.TextureDesc.Width, ImageDesc.TextureDesc.Height));
  renderCmd->SetPipelineState(0, m_pPso);
  renderCmd->SetVertexBuffer(0, m_CubeMesh->VBO());
  renderCmd->DrawInstanced(k3d::DrawInstancedParam(36, 1));
  renderCmd->EndEncode();

  commandBuffer->Present(m_pSwapChain, m_pFence);
  commandBuffer->Commit(m_pFence);
}
