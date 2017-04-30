#include <Base/UTRHIAppBase.h>
#include <Core/App.h>
#include <Core/AssetManager.h>
#include <Core/LogUtil.h>
#include <Core/Message.h>
#include <Kaleido3D.h>
#include <Math/kMath.hpp>

using namespace k3d;
using namespace kMath;

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
  void PreparePipeline();

private:
  rhi::PipelineStateRef m_pGfxPso;
  rhi::PipelineLayoutRef m_pGfxPl;
  rhi::PipelineStateRef m_pCompPso;
  rhi::PipelineLayoutRef m_pCompPl;
};

K3D_APP_MAIN(UTComputeParticles);

bool
UTComputeParticles::OnInit()
{
  bool inited = RHIAppBase::OnInit();
  if (!inited)
    return inited;
  PreparePipeline();
  return true;
}

void
UTComputeParticles::PreparePipeline()
{
  rhi::ShaderBundle vertSh, fragSh, compSh;
  Compile("asset://Test/particles.vert", rhi::ES_Vertex, vertSh);
  Compile("asset://Test/particles.frag", rhi::ES_Fragment, fragSh);
  Compile("asset://Test/particles.comp", rhi::ES_Compute, compSh);
  auto vertBinding = vertSh.BindingTable;
  auto fragBinding = fragSh.BindingTable;
  auto mergedBindings = vertBinding | fragBinding;
  m_pGfxPl = m_pDevice->NewPipelineLayout(mergedBindings);
  rhi::RenderPipelineStateDesc renderPipelineDesc;
  m_pGfxPso = m_pDevice->CreateRenderPipelineState(renderPipelineDesc, m_pGfxPl);
  m_pCompPl = m_pDevice->NewPipelineLayout(compSh.BindingTable);
  rhi::ComputePipelineStateDesc computePipelineDesc;
  m_pCompPso = m_pDevice->CreateComputePipelineState(computePipelineDesc, m_pCompPl);
}

void
UTComputeParticles::OnUpdate()
{
}

void
UTComputeParticles::OnProcess(Message& msg)
{
  auto renderCmdBuffer = m_pQueue->ObtainCommandBuffer(rhi::ECMDUsage_OneShot);
  auto renderCmd = renderCmdBuffer->RenderCommandEncoder(nullptr, nullptr);
  renderCmd->DrawInstanced(rhi::DrawInstancedParam(3,1));
  renderCmd->EndEncode();
  renderCmdBuffer->Present(m_pSwapChain, nullptr);
  renderCmdBuffer->Commit();

  auto computeCmdBuffer = m_pQueue->ObtainCommandBuffer(rhi::ECMDUsage_OneShot);
  auto computeCmd = computeCmdBuffer->ComputeCommandEncoder(nullptr);
  computeCmd->Dispatch(1, 1, 1);
  computeCmd->EndEncode();
  computeCmdBuffer->Commit();
}

void
UTComputeParticles::OnDestroy()
{
}