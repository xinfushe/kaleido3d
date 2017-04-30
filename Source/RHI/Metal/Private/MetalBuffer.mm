#include "Public/MetalRHIResource.h"
#include "Private/MetalRHI.h"

NS_K3D_METAL_BEGIN

Buffer::Buffer(Device* device, rhi::ResourceDesc const & desc)
: m_Desc(desc)
, m_Device(device)
{
    m_Buf = [m_Device->GetDevice()
             newBufferWithLength:desc.Size
             options: MTLResourceStorageModeManaged];
}

Buffer::~Buffer()
{
    if(m_Buf)
    {
        [m_Buf release];
        m_Buf = nil;
    }
}

void * Buffer::Map(uint64 start, uint64 size)
{
    m_MapRange.location = start;
    m_MapRange.length = size;
    return [m_Buf contents];
}

void Buffer::UnMap()
{
#if K3DPLATFORM_OS_MAC
    [m_Buf didModifyRange:m_MapRange];
#endif
}

uint64 Buffer::GetLocation() const
{
    return (uint64)m_Buf;
}

rhi::ResourceDesc Buffer::GetDesc() const
{
    return m_Desc;
}

rhi::EResourceState Buffer::GetState() const
{
    return rhi::ERS_Unknown;
}

uint64 Buffer::GetSize() const
{
    return m_Buf.length;
}

NS_K3D_METAL_END
