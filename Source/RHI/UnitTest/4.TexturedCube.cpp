#include <Kaleido3D.h>
#include <Base/UTRHIAppBase.h>
#include <Core/App.h>
#include <Core/AssetManager.h>
#include <Core/LogUtil.h>
#include <Core/Message.h>
#include <Renderer/Render.h>
#include <Math/kMath.hpp>
#include <Base/TextureObject.h>
//#include <steam/steam_api.h>

using namespace k3d;
using namespace render;
using namespace kMath;

class CubeMesh;

struct ConstantBuffer {
	Mat4f projectionMatrix;
	Mat4f modelMatrix;
	Mat4f viewMatrix;
};

class TCubeUnitTest : public RHIAppBase
{
public:
	TCubeUnitTest(kString const & appName, uint32 width, uint32 height)
	: RHIAppBase(appName, width, height)
	{}

	explicit TCubeUnitTest(kString const & appName)
		: RHIAppBase(appName, 1920, 1080)
	{}
	
	~TCubeUnitTest()
	{
		KLOG(Info, CubeTest, "Destroying..");
	}
	bool OnInit() override;
	void OnDestroy() override;
	void OnProcess(Message & msg) override;

	void OnUpdate() override;

protected:

	rhi::GpuResourceRef CreateStageBuffer(uint64 size);
	void LoadTexture();
	void PrepareResource();
	void PreparePipeline();
	void PrepareCommandBuffer();

private:

	SharedPtr<CubeMesh>					m_CubeMesh;
	rhi::GpuResourceRef					m_ConstBuffer;
	SharedPtr<TextureObject>			m_Texture;
	ConstantBuffer						m_HostBuffer;

	rhi::PipelineStateRef			m_pPso;
	rhi::PipelineLayoutRef				m_pl;
	std::vector<rhi::CommandContextRef>	m_Cmds;
	rhi::SyncFenceRef					m_pFence;
};

K3D_APP_MAIN(TCubeUnitTest);

class CubeMesh
{
public:
	struct Vertex {
		float pos[3];
		float col[4];
		float uv[2];
	};
	typedef std::vector<Vertex> VertexList;
	typedef std::vector<uint32> IndiceList;

	explicit CubeMesh(rhi::DeviceRef device) : m_pDevice (device), vbuf(nullptr)
	{
		m_szVBuf = sizeof(CubeMesh::Vertex)*m_VertexBuffer.size();
		m_IAState.Attribs[0] = {rhi::EVF_Float3x32, 0, 					0};
		m_IAState.Attribs[1] = {rhi::EVF_Float4x32, sizeof(float)*3, 	0};
		m_IAState.Attribs[2] = {rhi::EVF_Float2x32, sizeof(float)*7, 	0};

		m_IAState.Layouts[0] = {rhi::EVIR_PerVertex, sizeof(Vertex)};
	}

	~CubeMesh()
	{
	}
	
	const rhi::VertexInputState & GetInputState() const { return m_IAState;}

	void Upload();

	void SetLoc(uint64 vbo)
	{
		vboLoc = vbo; 
	}

	const rhi::VertexBufferView VBO() const { return rhi::VertexBufferView{ vboLoc, 0, 0 }; }
	
private:
	rhi::VertexInputState 	m_IAState;

	uint64 m_szVBuf;

	VertexList m_VertexBuffer = { 
		//left
		{ -1.0f,-1.0f,-1.0f,    1.0f, 1.0f, 0.0f, 1.0f,    0.0f, 0.0f },
		{ -1.0f,-1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },
		{ -1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f,    1.0f, 1.0f },

		{ -1.0f,-1.0f,-1.0f,    1.0f, 1.0f, 0.0f, 1.0f,    0.0f, 0.0f },
		{ -1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f },
		{ -1.0f, 1.0f,-1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },

		//back
		{ 1.0f, 1.0f,-1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },
		{ -1.0f,-1.0f,-1.0f,    0.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },
		{ -1.0f, 1.0f,-1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f },

		{ 1.0f, 1.0f,-1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },
		{ 1.0f,-1.0f,-1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 0.0f },
		{ -1.0f,-1.0f,-1.0f,    1.0f, 1.0f, 0.0f, 1.0f,    1.0f, 0.0f },

		//top
		{ 1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f },
		{ -1.0f,-1.0f,-1.0f,    1.0f, 1.0f, 0.0f, 1.0f,    0.0f, 0.0f },
		{ 1.0f, -1.0f,-1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },

		{ 1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f },
		{ -1.0f,-1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },
		{ -1.0f,-1.0f,-1.0f,    1.0f, 1.0f, 0.0f, 1.0f,    0.0f, 0.0f },

		//front
		{ -1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },
		{ -1.0f,-1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f,    0.0f, 0.0f },
		{ 1.0f,-1.0f, 1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },

		{ 1.0f, 1.0f, 1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },
		{ 1.0f,-1.0f, 1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },

		//right
		{ 1.0f, 1.0f, 1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },
		{ 1.0f,-1.0f,-1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },
		{ 1.0f, 1.0f,-1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f },

		{ 1.0f,-1.0f,-1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },
		{ 1.0f, 1.0f, 1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },
		{ 1.0f,-1.0f, 1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 0.0f },

		//bottom
		{ 1.0f, 1.0f, 1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },
		{ 1.0f, 1.0f,-1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f },
		{ -1.0f, 1.0f,-1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },

		{ 1.0f, 1.0f, 1.0f,     1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f },
		{ -1.0f, 1.0f,-1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 0.0f },
	};

	rhi::GpuResourceRef		vbuf;
	uint64					vboLoc;
	rhi::DeviceRef			m_pDevice;
};

void CubeMesh::Upload()
{
	// create stage buffers
	rhi::ResourceDesc stageDesc;
	stageDesc.ViewType = rhi::EGpuMemViewType::EGVT_Undefined;
	stageDesc.CreationFlag = rhi::EGpuResourceCreationFlag::EGRCF_TransferSrc;
	stageDesc.Flag = rhi::EGpuResourceAccessFlag::EGRAF_HostCoherent | rhi::EGpuResourceAccessFlag::EGRAF_HostVisible;
	stageDesc.Size = m_szVBuf;
	auto vStageBuf = m_pDevice->NewGpuResource(stageDesc);
	void * ptr = vStageBuf->Map(0, m_szVBuf);
	memcpy(ptr, &m_VertexBuffer[0], m_szVBuf);
	vStageBuf->UnMap();

	rhi::ResourceDesc bufferDesc;
	bufferDesc.ViewType = rhi::EGpuMemViewType::EGVT_VBV;
	bufferDesc.Size = m_szVBuf;
	bufferDesc.CreationFlag = rhi::EGpuResourceCreationFlag::EGRCF_TransferDst;
	bufferDesc.Flag = rhi::EGpuResourceAccessFlag::EGRAF_DeviceVisible;
	vbuf = m_pDevice->NewGpuResource(bufferDesc);

  auto pQueue = m_pDevice->CreateCommandQueue(rhi::ECMD_Graphics);
  auto cmdBuf = pQueue->ObtainCommandBuffer(rhi::ECMDUsage_OneShot);
	rhi::CopyBufferRegion region = {0,0, m_szVBuf};
  cmdBuf->CopyBuffer(vbuf, vStageBuf, region);
  cmdBuf->Commit();
	m_pDevice->WaitIdle();
	SetLoc(vbuf->GetLocation());
	KLOG(Info, TCubeMesh, "finish buffer upload..");
}


bool TCubeUnitTest::OnInit()
{
	bool inited = RHIAppBase::OnInit();
	if(!inited)
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

rhi::GpuResourceRef TCubeUnitTest::CreateStageBuffer(uint64 size)
{
	rhi::ResourceDesc stageDesc;
	stageDesc.ViewType = rhi::EGpuMemViewType::EGVT_Undefined;
	stageDesc.CreationFlag = rhi::EGpuResourceCreationFlag::EGRCF_TransferSrc;
	stageDesc.Flag = rhi::EGpuResourceAccessFlag::EGRAF_HostCoherent | rhi::EGpuResourceAccessFlag::EGRAF_HostVisible;
	stageDesc.Size = size;
	return m_pDevice->NewGpuResource(stageDesc);
}

void TCubeUnitTest::LoadTexture()
{
	IAsset * textureFile = AssetManager::Open("asset://Test/bricks.tga");
	if (textureFile)
	{
 		uint64 length = textureFile->GetLength();
		uint8* data = new uint8[length];
		textureFile->Read(data, length);
		m_Texture = MakeShared<TextureObject>(m_pDevice, data, false);
		auto texStageBuf = CreateStageBuffer(m_Texture->GetSize());
		m_Texture->MapIntoBuffer(texStageBuf);
		m_Texture->CopyAndInitTexture(texStageBuf);
		rhi::ResourceViewDesc viewDesc;
		auto srv = m_pDevice->NewShaderResourceView(m_Texture->GetResource(), viewDesc);	// TODO: Fix circle ref of `m_Texture->m_Resource`
		auto texure = StaticPointerCast<rhi::ITexture>(m_Texture->GetResource());			//
		texure->SetResourceView(Move(srv));													// here
		rhi::SamplerState samplerDesc;
		auto sampler2D = m_pDevice->NewSampler(samplerDesc);
		texure->BindSampler(Move(sampler2D));
	}
}

void TCubeUnitTest::PrepareResource()
{
	KLOG(Info, Test, __K3D_FUNC__);
	m_CubeMesh = MakeShared<CubeMesh>(m_pDevice);
	m_CubeMesh->Upload();
	LoadTexture();
	rhi::ResourceDesc desc;
	desc.Flag = rhi::EGpuResourceAccessFlag::EGRAF_HostVisible;
	desc.ViewType = rhi::EGpuMemViewType::EGVT_CBV;
	desc.Size = sizeof(ConstantBuffer);
	m_ConstBuffer = m_pDevice->NewGpuResource(desc);
	OnUpdate();
}

void TCubeUnitTest::PreparePipeline()
{
	rhi::ShaderBundle vertSh, fragSh;
	Compile("asset://Test/cube.vert", rhi::ES_Vertex, vertSh);
	Compile("asset://Test/cube.frag", rhi::ES_Fragment, fragSh);
	auto vertBinding = vertSh.BindingTable;
	auto fragBinding = fragSh.BindingTable;
	auto mergedBindings = vertBinding | fragBinding;
	m_pl = m_pDevice->NewPipelineLayout(mergedBindings);
	auto descriptor = m_pl->GetDescriptorSet();
	descriptor->Update(0, m_ConstBuffer);
	descriptor->Update(1, m_Texture->GetResource());
	rhi::RenderPipelineStateDesc desc;
	desc.VertexShader = vertSh;
	desc.PixelShader = fragSh;
	desc.InputState = m_CubeMesh->GetInputState();
	m_pPso = m_pDevice->CreateRenderPipelineState(desc, m_pl);
}

void TCubeUnitTest::PrepareCommandBuffer()
{
	m_pFence = m_pDevice->CreateFence();
#if 0
	for (uint32 i = 0; i < m_pViewport->GetSwapChainCount(); i++)
	{
		auto pRT = m_pViewport->GetRenderTarget(i);
		auto gfxCmd = m_pDevice->NewCommandContext(rhi::ECMD_Graphics);
		gfxCmd->Begin();
		gfxCmd->TransitionResourceBarrier(pRT->GetBackBuffer(), rhi::ERS_RenderTarget);
		gfxCmd->SetPipelineLayout(m_pl);
		rhi::Rect rect{ 0,0, (long)m_pViewport->GetWidth(), (long)m_pViewport->GetHeight() };
		gfxCmd->SetRenderTarget(pRT);
		gfxCmd->SetScissorRects(1, &rect);
		gfxCmd->SetViewport(rhi::ViewportDesc(1.f*m_pViewport->GetWidth(), 1.f*m_pViewport->GetHeight()));
		gfxCmd->SetPipelineState(0, m_pPso);
		gfxCmd->SetVertexBuffer(0, m_CubeMesh->VBO());
		gfxCmd->DrawInstanced(rhi::DrawInstancedParam(36, 1));
		gfxCmd->EndRendering();
		gfxCmd->TransitionResourceBarrier(pRT->GetBackBuffer(), rhi::ERS_Present);
		gfxCmd->End();
		m_Cmds.push_back(gfxCmd);
	}
#endif
}

void TCubeUnitTest::OnDestroy()
{
	m_pFence->WaitFor(1000);
	RHIAppBase::OnDestroy();
}

void TCubeUnitTest::OnProcess(Message& msg)
{
	KLOG(Info, TCubeUnitTest, __K3D_FUNC__);
}

void TCubeUnitTest::OnUpdate()
{
//  ControllerHandle_t pHandles[STEAM_CONTROLLER_MAX_COUNT];
//  int nNumActive = SteamController()->GetConnectedControllers(pHandles);
	m_HostBuffer.projectionMatrix = Perspective(60.0f, 1920.f/1080.f, 0.1f, 256.0f);
	m_HostBuffer.viewMatrix = Translate(Vec3f(0.0f, 0.0f, -4.5f), MakeIdentityMatrix<float>());
	m_HostBuffer.modelMatrix = MakeIdentityMatrix<float>();
	static auto angle = 60.f;
#if K3DPLATFORM_OS_ANDROID
	angle += 0.1f;
#else
	angle += 0.001f;
#endif
	m_HostBuffer.modelMatrix = Rotate(Vec3f(1.0f, 0.0f, 0.0f), angle, m_HostBuffer.modelMatrix);
	m_HostBuffer.modelMatrix = Rotate(Vec3f(0.0f, 1.0f, 0.0f), angle, m_HostBuffer.modelMatrix);
	m_HostBuffer.modelMatrix = Rotate(Vec3f(0.0f, 0.0f, 1.0f), angle, m_HostBuffer.modelMatrix);
	void * ptr = m_ConstBuffer->Map(0, sizeof(ConstantBuffer));
	memcpy(ptr, &m_HostBuffer, sizeof(ConstantBuffer));
	m_ConstBuffer->UnMap();
}
