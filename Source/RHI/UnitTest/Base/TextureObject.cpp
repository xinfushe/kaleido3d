#include <Kaleido3D.h>
#include "TextureObject.h"
#include <Core/LogUtil.h>

///////////////////////////////////////////////////////////////////////////////

TextureObject::TextureObject(k3d::DeviceRef pDevice, const uint8_t * dataInMemory, bool useStaging) 
	: m_pDevice(pDevice), m_width(0), m_height(0), m_format(k3d::EPF_RGB8Unorm), m_pBits(nullptr), m_DataSize(0), m_UseStaging(useStaging)
{
	InitData(dataInMemory);
	if (!useStaging)
	{
		InitTexture();
	}
}

TextureObject::~TextureObject()
{
	KLOG(Info, TextureObject, "Destroying..");
	Destroy();
}

void TextureObject::MapIntoBuffer(k3d::GpuResourceRef stageBuff)
{
	if (!m_UseStaging)
		return;
	void * pData = stageBuff->Map(0, GetSize());
	memcpy(pData, m_pBits, GetSize());
	stageBuff->UnMap();
}

void TextureObject::CopyAndInitTexture(k3d::GpuResourceRef stageBuff)
{
	if (!m_UseStaging)
		return;
	k3d::ResourceDesc texDesc;
	texDesc.Type = k3d::EGT_Texture2D;
	texDesc.ViewType = k3d::EGpuMemViewType::EGVT_SRV;
	texDesc.CreationFlag = k3d::EGpuResourceCreationFlag::EGRCF_TransferDst;
	texDesc.Flag = k3d::EGpuResourceAccessFlag::EGRAF_DeviceVisible;
	texDesc.TextureDesc.Format = k3d::EPF_RGBA8Unorm;
	texDesc.TextureDesc.Width = m_width;
	texDesc.TextureDesc.Height = m_height;
	texDesc.TextureDesc.Layers = 1;
	texDesc.TextureDesc.MipLevels = 1;
	texDesc.TextureDesc.Depth = 1;
	m_Resource = m_pDevice->CreateResource(texDesc);
  auto pQueue = m_pDevice->CreateCommandQueue(k3d::ECMD_Graphics);
  auto cmdBuf = pQueue->ObtainCommandBuffer(k3d::ECMDUsage_OneShot);
  cmdBuf->Transition(m_Resource, k3d::ERS_TransferDst);
  k3d::TextureCopyLocation copyDest(m_Resource, ::k3d::DynArray<uint32>());
  auto footprints = ::k3d::DynArray<k3d::PlacedSubResourceFootprint>();
  k3d::PlacedSubResourceFootprint fp;
  fp.BufferOffSet = 0; fp.TOffSetX = 0; fp.TOffSetY = 0; fp.TOffSetZ = 0;
  fp.Footprint.Dimension = texDesc.TextureDesc;
  fp.Footprint.SubLayout = {};
  footprints.Append(fp);
  k3d::TextureCopyLocation copySrc(stageBuff, footprints);
  cmdBuf->CopyTexture(copyDest, copySrc);
  cmdBuf->Transition(m_Resource, k3d::ERS_ShaderResource);
  cmdBuf->Commit();
}

///////////////////////////////////////////////////////////////////////////////

void TextureObject::InitData(const uint8* dataInMemory)
{
	struct TARGA_HEADER
	{
		uint8_t   IDLength, ColormapType, ImageType;
		uint8_t   ColormapSpecification[5];
		uint16_t  XOrigin, YOrigin;
		uint16_t  ImageWidth, ImageHeight;
		uint8_t   PixelDepth;
		uint8_t   ImageDescriptor;
	};
	TARGA_HEADER header;
	header = *(TARGA_HEADER*)dataInMemory;
	uint8_t nPixelSize = header.PixelDepth / 8;
	m_width = header.ImageWidth;
	m_height = header.ImageHeight;
	m_format = nPixelSize == 3 ? k3d::EPF_RGBA8Unorm : k3d::EPF_RGBA8Unorm;
	dataInMemory += sizeof(TARGA_HEADER);
	m_pBits = new uint8_t[nPixelSize * header.ImageWidth * header.ImageHeight];
	memcpy(m_pBits, dataInMemory, nPixelSize * header.ImageWidth * header.ImageHeight);
	m_DataSize = nPixelSize * header.ImageWidth * header.ImageHeight;
	uint8_t* p = m_pBits;
	for (uint32_t y = 0; y < header.ImageHeight; y++)
	{
		for (uint32_t x = 0; x < header.ImageWidth; x++)
		{
			uint8_t temp = p[2];
			p[2] = p[0];
			p[0] = temp;
			p += nPixelSize;
		}
	}
}

void TextureObject::InitTexture()
{
	k3d::ResourceDesc texDesc;
	texDesc.Type = k3d::EGT_Texture2D;
	texDesc.ViewType = k3d::EGpuMemViewType::EGVT_SRV;
	texDesc.Flag = k3d::EGpuResourceAccessFlag::EGRAF_HostVisible;
	texDesc.TextureDesc.Format = k3d::EPF_RGBA8Unorm;
	texDesc.TextureDesc.Width = m_width;
	texDesc.TextureDesc.Height = m_height;
	texDesc.TextureDesc.Layers = 1;
	texDesc.TextureDesc.MipLevels = 1;
	texDesc.TextureDesc.Depth = 1;
	m_Resource = m_pDevice->CreateResource(texDesc);

	uint64 sz = m_Resource->GetSize();
	void * pData = m_Resource->Map(0, sz);
	k3d::SubResourceLayout layout = {};
	k3d::TextureSpec spec = { k3d::ETAF_COLOR,0,0 };
	m_pDevice->QueryTextureSubResourceLayout(k3d::StaticPointerCast<k3d::ITexture>(m_Resource), spec, &layout);
	if (m_width * 4 == layout.RowPitch) // directly upload
	{
		memcpy(pData, m_pBits, sz);
	}
	else
	{
		for (int y = 0; y < m_height; y++)
		{
			uint32_t *row = (uint32_t *)((char *)pData + layout.RowPitch * y);
			for (int x = 0; x < m_width; x++)
			{
				row[x] = m_pBits[x + y * m_width];
			}
		}
	}
	m_Resource->UnMap();
#if 0
	m_pContext = m_pDevice->NewCommandContext(k3d::ECMD_Graphics);
	m_pContext->Begin();
	m_pContext->TransitionResourceBarrier(m_Resource, k3d::ERS_ShaderResource);
	m_pContext->End();
	m_pContext->Execute(false);
#endif
}

void TextureObject::InitSampler()
{
}

bool TextureObject::Destroy()
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////
