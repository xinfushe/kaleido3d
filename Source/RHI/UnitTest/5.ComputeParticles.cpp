#include <Base/UTRHIAppBase.h>
#include <Core/App.h>
#include <Core/AssetManager.h>
#include <Core/LogUtil.h>
#include <Core/Message.h>
#include <Kaleido3D.h>
#include <Math/kMath.hpp>

#include <d3d12.h>

using namespace k3d;
using namespace kMath;

enum
{
  PARTICLE_GROUP_SIZE = 1024,
  PARTICLE_GROUP_COUNT = 8192,
  PARTICLE_COUNT = (PARTICLE_GROUP_SIZE * PARTICLE_GROUP_COUNT),
  MAX_ATTRACTORS = 64
};

static inline float random_float()
{
  float res;
  unsigned int tmp;
  static unsigned int seed = 0xFFFF0C59;

  seed *= 16807;

  tmp = seed ^ (seed >> 4) ^ (seed << 15);

  *((unsigned int *)&res) = (tmp >> 9) | 0x3F800000;

  return (res - 1.0f);
}
static Vec3f random_vector(float minmag = 0.0f, float maxmag = 1.0f)
{
  Vec3f randomvec(random_float() * 2.0f - 1.0f, random_float() * 2.0f - 1.0f, random_float() * 2.0f - 1.0f);
  randomvec = Normalize(randomvec);
  randomvec = randomvec * (random_float() * (maxmag - minmag) + minmag);
  return randomvec;
}

#pragma region AppDefine

class UTComputeParticles : public RHIAppBase
{
public:
  UTComputeParticles(kString const& appName, uint32 width, uint32 height)
    : RHIAppBase(appName, width, height)
  {
  }
  explicit UTComputeParticles(kString const& appName)
    : RHIAppBase(appName, 1920, 1080)
  {
  }

  bool OnInit() override;
  void OnDestroy() override;
  void OnProcess(Message& msg) override;

  void OnUpdate() override;

protected:
  void PrepareResources();
  void GenerateParticles();
  void PreparePipeline();

private:

  float attractor_masses[MAX_ATTRACTORS];

private:
  k3d::PipelineStateRef m_pGfxPso;
  k3d::PipelineLayoutRef m_pGfxPl;

  k3d::BindingGroupRef m_GfxBindingGroup;

  k3d::PipelineStateRef m_pCompPso;
  k3d::PipelineLayoutRef m_pCompPl;

  k3d::BindingGroupRef m_CptBindingGroup;

  k3d::BufferRef m_PositionBuffer;
  k3d::UnorderedAccessViewRef m_PosUAV;
  k3d::BufferRef m_VelocityBuffer;
  k3d::UnorderedAccessViewRef m_VelUAV;
  k3d::VertexBufferView m_PosVBV;

  k3d::GpuResourceRef m_MVPBuffer;

private:
  k3d::VertexInputState m_IAState;
  k3d::RenderPassDesc m_RpDesc;
};

#pragma endregion

K3D_APP_MAIN(UTComputeParticles);

bool
UTComputeParticles::OnInit()
{
  bool inited = RHIAppBase::OnInit();
  if (!inited)
    return inited;
  PrepareResources();
  PreparePipeline();
  return true;
}

#pragma region Initialize

void UTComputeParticles::PrepareResources()
{
  GenerateParticles();
  m_IAState.Attribs[0] = { k3d::EVF_Float4x32, 0, 0 };
  m_IAState.Layouts[0] = { k3d::EVIR_PerVertex, sizeof(float)*4 };
  m_PosVBV = { m_PositionBuffer->GetLocation(), 0, 0 };

  k3d::ResourceDesc desc;
  desc.Flag = k3d::EGpuResourceAccessFlag::EGRAF_HostVisible;
  desc.ViewType = k3d::EGpuMemViewType::EGVT_CBV;
  desc.Size = sizeof(MVPMatrix);
  m_MVPBuffer = m_pDevice->CreateResource(desc);

  MVPMatrix* pMatrix = (MVPMatrix*) m_MVPBuffer->Map(0, sizeof(MVPMatrix));
  pMatrix->projectionMatrix =
    Perspective(60.0f, 1920.f / 1080.f, 0.1f, 256.0f);
  pMatrix->viewMatrix =
    Translate(Vec3f(0.0f, 0.0f, -4.5f), MakeIdentityMatrix<float>());
  pMatrix->modelMatrix = MakeIdentityMatrix<float>();
  static auto angle = 60.f;
#if K3DPLATFORM_OS_ANDROID
  angle += 0.1f;
#else
  angle += 0.001f;
#endif
  pMatrix->modelMatrix =
    Rotate(Vec3f(1.0f, 0.0f, 0.0f), angle, pMatrix->modelMatrix);
  pMatrix->modelMatrix =
    Rotate(Vec3f(0.0f, 1.0f, 0.0f), angle, pMatrix->modelMatrix);
  pMatrix->modelMatrix =
    Rotate(Vec3f(0.0f, 0.0f, 1.0f), angle, pMatrix->modelMatrix);
  m_MVPBuffer->UnMap();

}

void UTComputeParticles::GenerateParticles()
{
  ResourceDesc Desc;
  Desc.Type = EGT_Buffer;
  Desc.ViewType = EGVT_VBV;
  Desc.CreationFlag = EGRCF_Dynamic;
  Desc.Flag = EGRAF_HostVisible;
  Desc.Size = PARTICLE_COUNT * sizeof(Vec4f);
  auto particleBuffer = m_pDevice->CreateResource(Desc);
  auto Pointer = (Vec4f*)particleBuffer->Map(0, Desc.Size);
  for (uint32 i = 0; i < PARTICLE_COUNT; i++)
  {
    Pointer[i] = Vec4f(random_vector(-10.0f, 10.0f), random_float());
  }
  particleBuffer->UnMap();
  m_PositionBuffer = StaticPointerCast<k3d::IBuffer>(particleBuffer);

  UAVDesc uavDesc;
  m_PosUAV = m_pDevice->CreateUnorderedAccessView(m_PositionBuffer, uavDesc);

  auto velocityBuffer = m_pDevice->CreateResource(Desc); 
  Pointer = (Vec4f*)velocityBuffer->Map(0, Desc.Size);
  for (uint32 i = 0; i < PARTICLE_COUNT; i++)
  {
    Pointer[i] = Vec4f(random_vector(-0.1f, 0.1f), 0.0f);
  }
  velocityBuffer->UnMap();
  m_VelocityBuffer = StaticPointerCast<k3d::IBuffer>(velocityBuffer);
}

void
UTComputeParticles::PreparePipeline()
{
  k3d::ShaderBundle vertSh, fragSh, compSh;
  Compile("asset://Test/particles.vert", k3d::ES_Vertex, vertSh);
  Compile("asset://Test/particles.frag", k3d::ES_Fragment, fragSh);
  
  Compile("asset://Test/particles.comp", k3d::ES_Compute, compSh);

  auto vertBinding = vertSh.BindingTable;
  auto fragBinding = fragSh.BindingTable;
  auto mergedBindings = vertBinding | fragBinding;
  m_pGfxPl = m_pDevice->CreatePipelineLayout(mergedBindings);
  m_GfxBindingGroup = m_pGfxPl->ObtainBindingGroup();
  m_GfxBindingGroup->Update(0, m_MVPBuffer);

  k3d::ColorAttachmentDesc ColorAttach;
  ColorAttach.pTexture = m_pSwapChain->GetCurrentTexture();
  ColorAttach.LoadAction = k3d::ELA_Clear;
  ColorAttach.StoreAction = k3d::ESA_Store;
  ColorAttach.ClearColor = Vec4f(1, 1, 1, 1);

  m_RpDesc.ColorAttachments.Append(ColorAttach);
  auto pRenderPass = m_pDevice->CreateRenderPass(m_RpDesc);

  k3d::RenderPipelineStateDesc renderPipelineDesc;
  renderPipelineDesc.AttachmentsBlend.Append(AttachmentState());
  renderPipelineDesc.PrimitiveTopology = EPT_Points;
  renderPipelineDesc.InputState = m_IAState;
  renderPipelineDesc.VertexShader = vertSh;
  renderPipelineDesc.PixelShader = fragSh;
  m_pGfxPso = m_pDevice->CreateRenderPipelineState(renderPipelineDesc, m_pGfxPl, pRenderPass);

  m_pCompPl = m_pDevice->CreatePipelineLayout(compSh.BindingTable);
  m_CptBindingGroup = m_pCompPl->ObtainBindingGroup();
  m_CptBindingGroup->Update(0, StaticPointerCast<k3d::IGpuResource>(m_VelocityBuffer));
  m_CptBindingGroup->Update(1, StaticPointerCast<k3d::IGpuResource>(m_PositionBuffer));
  k3d::ComputePipelineStateDesc computePipelineDesc;
  computePipelineDesc.ComputeShader = compSh;
  m_pCompPso = m_pDevice->CreateComputePipelineState(computePipelineDesc, m_pCompPl);
}

#pragma endregion

void
UTComputeParticles::OnUpdate()
{
}

void
UTComputeParticles::OnProcess(Message& msg)
{
  auto currentImage = m_pSwapChain->GetCurrentTexture();
  auto ImageDesc = currentImage->GetDesc();
  m_RpDesc.ColorAttachments[0].pTexture = currentImage;

  auto renderCmdBuffer = m_pQueue->ObtainCommandBuffer(k3d::ECMDUsage_OneShot);
  renderCmdBuffer->Transition(m_PositionBuffer, k3d::ERS_Common);
  auto renderCmd = renderCmdBuffer->RenderCommandEncoder(m_RpDesc);
  renderCmd->SetBindingGroup(m_GfxBindingGroup);
  renderCmd->SetPipelineState(0, m_pGfxPso);
  renderCmd->SetVertexBuffer(0, m_PosVBV);
  renderCmd->DrawInstanced(k3d::DrawInstancedParam(PARTICLE_COUNT, 1));
  renderCmd->EndEncode();
  renderCmdBuffer->Present(m_pSwapChain, nullptr);
  renderCmdBuffer->Commit();

  auto computeCmdBuffer = m_pQueue->ObtainCommandBuffer(k3d::ECMDUsage_OneShot);
  auto computeCmd = computeCmdBuffer->ComputeCommandEncoder(nullptr);
  computeCmd->SetPipelineState(0, m_pCompPso);
  computeCmd->SetBindingGroup(m_CptBindingGroup);
  computeCmd->Dispatch(PARTICLE_COUNT, 1, 1);
  computeCmd->EndEncode();
  computeCmdBuffer->Commit();
}

void
UTComputeParticles::OnDestroy()
{
}