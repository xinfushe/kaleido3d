#include <Kaleido3D.h>
#include <Base/UTRHIAppBase.h>
#include <Core/App.h>
#include <Core/AssetManager.h>
#include <Core/LogUtil.h>
#include <Core/Message.h>
#include <Interface/IRHI.h>
#include <Math/kMath.hpp>
#include <Renderer/Render.h>

using namespace k3d;
using namespace render;
using namespace kMath;

class TriangleMesh;

struct ConstantBuffer
{
  Mat4f projectionMatrix;
  Mat4f modelMatrix;
  Mat4f viewMatrix;
};

class TriangleDSApp : public RHIAppBase
{
public:
  TriangleDSApp(kString const& appName, uint32 width, uint32 height)
    : RHIAppBase(appName, width, height, true)
  {
  }
  explicit TriangleDSApp(kString const& appName)
    : RHIAppBase(appName, 1920, 1080, true)
  {
  }

  bool OnInit() override;
  void OnDestroy() override;
  void OnProcess(Message& msg) override;

  void OnUpdate() override;

protected:
  void PrepareResource();
  void PrepareRenderPass();
  void PreparePipeline();
  void PrepareCommandBuffer();

private:
  k3d::IShCompiler::Ptr m_Compiler;
  std::unique_ptr<TriangleMesh> m_TriMesh;

  k3d::NGFXResourceRef m_pDepthStencilTexture;

  k3d::NGFXResourceRef m_ConstBuffer;
  ConstantBuffer m_HostBuffer;

  k3d::NGFXPipelineStateRef m_pPso;
  k3d::NGFXPipelineLayoutRef m_pl;
  k3d::NGFXBindingGroupRef m_BindingGroup;
  k3d::RenderPassDesc m_RenderPassDesc;
  k3d::NGFXRenderpassRef m_pRenderPass;

  k3d::NGFXFenceRef m_pFence;
};

K3D_APP_MAIN(TriangleDSApp)

class TriangleMesh
{
public:
  struct Vertex
  {
    float pos[3];
    float col[3];
  };
  typedef std::vector<Vertex> VertexList;
  typedef std::vector<uint32> IndiceList;

  explicit TriangleMesh(k3d::NGFXDeviceRef device)
    : m_pDevice(device)
    , vbuf(nullptr)
    , ibuf(nullptr)
  {
    m_szVBuf = sizeof(TriangleMesh::Vertex) * m_VertexBuffer.size();
    m_szIBuf = sizeof(uint32) * m_IndexBuffer.size();

    m_IAState.Attribs[0] = { NGFX_VERTEX_FORMAT_FLOAT3X32, 0, 0 };
    m_IAState.Attribs[1] = { NGFX_VERTEX_FORMAT_FLOAT3X32, sizeof(float) * 3, 0 };

    m_IAState.Layouts[0] = { NGFX_VERTEX_INPUT_RATE_PER_VERTEX, sizeof(Vertex) };
  }

  ~TriangleMesh() {}

  const k3d::VertexInputState& GetInputState() const { return m_IAState; }

  void Upload(k3d::NGFXCommandQueueRef pQueue);

  void SetLoc(uint64 ibo, uint64 vbo)
  {
    iboLoc = ibo;
    vboLoc = vbo;
  }

  const k3d::VertexBufferView VBO() const
  {
    return k3d::VertexBufferView{ vboLoc, 0, 0 };
  }

  const k3d::IndexBufferView IBO() const
  {
    return k3d::IndexBufferView{ iboLoc, 0 };
  }

private:
  k3d::VertexInputState m_IAState;

  uint64 m_szVBuf;
  uint64 m_szIBuf;

  VertexList m_VertexBuffer = { { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
                                { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
                                { { 0.0f, -1.0f, 0.0f },
                                  { 0.0f, 0.0f, 1.0f } } };
  IndiceList m_IndexBuffer = { 0, 1, 2 };

  uint64 iboLoc;
  uint64 vboLoc;

  k3d::NGFXDeviceRef m_pDevice;
  k3d::NGFXResourceRef vbuf, ibuf;
};

void
TriangleMesh::Upload(k3d::NGFXCommandQueueRef pQueue)
{
  // create stage buffers
  k3d::ResourceDesc stageDesc;
  stageDesc.ViewFlags = NGFX_RESOURCE_VIEW_UNDEFINED;
  stageDesc.CreationFlag = NGFX_RESOURCE_TRANSFER_SRC;
  stageDesc.Flag = NGFX_ACCESS_HOST_COHERENT |
                   NGFX_ACCESS_HOST_VISIBLE;
  stageDesc.Size = m_szVBuf;
  auto vStageBuf = m_pDevice->CreateResource(stageDesc);
  void* ptr = vStageBuf->Map(0, m_szVBuf);
  memcpy(ptr, &m_VertexBuffer[0], m_szVBuf);
  vStageBuf->UnMap();

  stageDesc.Size = m_szIBuf;
  auto iStageBuf = m_pDevice->CreateResource(stageDesc);
  ptr = iStageBuf->Map(0, m_szIBuf);
  memcpy(ptr, &m_IndexBuffer[0], m_szIBuf);
  iStageBuf->UnMap();

  k3d::ResourceDesc bufferDesc;
  bufferDesc.ViewFlags = NGFX_RESOURCE_VERTEX_BUFFER_VIEW;
  bufferDesc.Size = m_szVBuf;
  bufferDesc.CreationFlag = NGFX_RESOURCE_TRANSFER_DST;
  bufferDesc.Flag = NGFX_ACCESS_DEVICE_VISIBLE;
  vbuf = m_pDevice->CreateResource(bufferDesc);
  bufferDesc.ViewFlags = NGFX_RESOURCE_INDEX_BUFFER_VIEW;
  bufferDesc.Size = m_szIBuf;
  ibuf = m_pDevice->CreateResource(bufferDesc);

  auto cmd = pQueue->ObtainCommandBuffer(NGFX_COMMAND_USAGE_ONE_SHOT);
  k3d::CopyBufferRegion region = { 0, 0, m_szVBuf };
  cmd->CopyBuffer(vbuf, vStageBuf, region);
  region.CopySize = m_szIBuf;
  cmd->CopyBuffer(ibuf, iStageBuf, region);
  cmd->Commit();

  //	m_m_pDevice->WaitIdle();
  uint64 vboLoc = vbuf->GetLocation();
  uint64 iboLoc = ibuf->GetLocation();
  SetLoc(iboLoc, vboLoc);
  KLOG(Info, TriangleMesh, "finish buffer upload..");
}

bool
TriangleDSApp::OnInit()
{
  bool inited = RHIAppBase::OnInit();
  if (!inited)
    return inited;

  KLOG(Info, Test, __K3D_FUNC__);

  PrepareResource();
  PrepareRenderPass();
  PreparePipeline();
  PrepareCommandBuffer();

  return true;
}

void
TriangleDSApp::PrepareResource()
{
  KLOG(Info, Test, __K3D_FUNC__);
  m_TriMesh = std::make_unique<TriangleMesh>(m_pDevice);
  m_TriMesh->Upload(m_pQueue);

  k3d::ResourceDesc desc;
  desc.Flag = NGFX_ACCESS_HOST_VISIBLE;
  desc.ViewFlags = NGFX_RESOURCE_CONSTANT_BUFFER_VIEW;
  desc.Size = sizeof(ConstantBuffer);
  m_ConstBuffer = m_pDevice->CreateResource(desc);
  OnUpdate();

  desc.Flag = NGFX_ACCESS_DEVICE_VISIBLE;
  desc.Type = NGFX_TEXTURE_2D;
  desc.ViewFlags = NGFX_RESOURCE_DEPTH_STENCIL_VIEW;

  auto SwSize = m_pSwapChain->GetCurrentTexture()->GetDesc().TextureDesc;
  desc.TextureDesc = { NGFX_PIXEL_FORMAT_D24_S8, SwSize.Width, SwSize.Height, 1, 1, 1 };
  m_pDepthStencilTexture = m_pDevice->CreateResource(desc);
}

void
TriangleDSApp::PrepareRenderPass()
{
  k3d::ColorAttachmentDesc ColorAttach;
  ColorAttach.pTexture = m_pSwapChain->GetCurrentTexture();
  ColorAttach.LoadAction = NGFX_LOAD_ACTION_CLEAR;
  ColorAttach.StoreAction = NGFX_STORE_ACTION_STORE;
  ColorAttach.ClearColor = Vec4f(1, 1, 1, 1);
  m_RenderPassDesc.ColorAttachments.Append(ColorAttach);
 
  m_RenderPassDesc.pDepthAttachment = MakeShared<DepthAttachmentDesc>();
  m_RenderPassDesc.pDepthAttachment->LoadAction = NGFX_LOAD_ACTION_CLEAR;
  m_RenderPassDesc.pDepthAttachment->StoreAction = NGFX_STORE_ACTION_STORE;
  m_RenderPassDesc.pDepthAttachment->ClearDepth = 1.0f;
  m_RenderPassDesc.pDepthAttachment->pTexture = StaticPointerCast<k3d::NGFXTexture>(m_pDepthStencilTexture);

  m_RenderPassDesc.pStencilAttachment = MakeShared<StencilAttachmentDesc>();
  m_RenderPassDesc.pStencilAttachment->LoadAction = NGFX_LOAD_ACTION_CLEAR;
  m_RenderPassDesc.pStencilAttachment->StoreAction = NGFX_STORE_ACTION_STORE;
  m_RenderPassDesc.pStencilAttachment->ClearStencil = 0.0f;
  m_RenderPassDesc.pStencilAttachment->pTexture = StaticPointerCast<k3d::NGFXTexture>(m_pDepthStencilTexture);

  m_pRenderPass = m_pDevice->CreateRenderPass(m_RenderPassDesc);
}

void
TriangleDSApp::PreparePipeline()
{
  k3d::NGFXShaderBundle vertSh, fragSh;
  Compile("asset://Test/triangle.vert", NGFX_SHADER_TYPE_VERTEX, vertSh);
  Compile("asset://Test/triangle.frag", NGFX_SHADER_TYPE_FRAGMENT, fragSh);
  k3d::PipelineLayoutDesc ppldesc = vertSh.BindingTable;
  m_pl = m_pDevice->CreatePipelineLayout(ppldesc);
  if (m_pl) {
    m_BindingGroup = m_pl->ObtainBindingGroup();
    m_BindingGroup->Update(0, m_ConstBuffer);
  }
  auto attrib = vertSh.Attributes;
  k3d::RenderPipelineStateDesc desc;
  desc.AttachmentsBlend.Append(AttachmentState());
  desc.DepthStencil.DepthEnable = true;
  desc.DepthStencil.StencilEnable = true;
  desc.VertexShader = vertSh;
  desc.PixelShader = fragSh;
  desc.InputState = m_TriMesh->GetInputState();
  m_pPso = m_pDevice->CreateRenderPipelineState(desc, m_pl, m_pRenderPass);
  // m_pPso->LoadPSO("triagle.psocache");
}

void
TriangleDSApp::PrepareCommandBuffer()
{
  m_pFence = m_pDevice->CreateFence();
}

void
TriangleDSApp::OnDestroy()
{
  App::OnDestroy();
  m_TriMesh->~TriangleMesh();
}

void
TriangleDSApp::OnProcess(Message& msg)
{
  auto currentImage = m_pSwapChain->GetCurrentTexture();
  auto ImageDesc = currentImage->GetDesc();
  m_RenderPassDesc.ColorAttachments[0].pTexture = currentImage;
  auto commandBuffer = m_pQueue->ObtainCommandBuffer(NGFX_COMMAND_USAGE_ONE_SHOT);
  // command encoder like Metal does, should use desc instead, look obj from cache
  auto renderCmd = commandBuffer->RenderCommandEncoder(m_RenderPassDesc);
  renderCmd->SetBindingGroup(m_BindingGroup);
  k3d::Rect rect{ 0, 0, ImageDesc.TextureDesc.Width, ImageDesc.TextureDesc.Height };
  renderCmd->SetScissorRect(rect);
  renderCmd->SetViewport(k3d::ViewportDesc(ImageDesc.TextureDesc.Width, ImageDesc.TextureDesc.Height));
  renderCmd->SetPipelineState(0, m_pPso);
  renderCmd->SetIndexBuffer(m_TriMesh->IBO());
  renderCmd->SetVertexBuffer(0, m_TriMesh->VBO());
  renderCmd->DrawIndexedInstanced(k3d::DrawIndexedInstancedParam(3, 1));
  renderCmd->EndEncode();

  commandBuffer->Present(m_pSwapChain, m_pFence);
  commandBuffer->Commit(m_pFence);
}

void
TriangleDSApp::OnUpdate()
{
  m_HostBuffer.projectionMatrix =
    Perspective(60.0f, (float)1920 / (float)1080, 0.1f, 256.0f);
  m_HostBuffer.viewMatrix =
    Translate(Vec3f(0.0f, 0.0f, -2.5f), MakeIdentityMatrix<float>());
  m_HostBuffer.modelMatrix = MakeIdentityMatrix<float>();
  m_HostBuffer.modelMatrix =
    Rotate(Vec3f(1.0f, 0.0f, 0.0f), 0.f, m_HostBuffer.modelMatrix);
  m_HostBuffer.modelMatrix =
    Rotate(Vec3f(0.0f, 1.0f, 0.0f), 0.f, m_HostBuffer.modelMatrix);
  m_HostBuffer.modelMatrix =
    Rotate(Vec3f(0.0f, 0.0f, 1.0f), 0.f, m_HostBuffer.modelMatrix);
  void* ptr = m_ConstBuffer->Map(0, sizeof(ConstantBuffer));
  memcpy(ptr, &m_HostBuffer, sizeof(ConstantBuffer));
  m_ConstBuffer->UnMap();

}
