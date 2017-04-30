#pragma once
#ifndef __MetalRHIResource_h__
#define __MetalRHIResource_h__

#include "../Common.h"
#include <Interface/IRHI.h>

NS_K3D_METAL_BEGIN

class Device;

class BufferRef
{
public:
    BufferRef(id<MTLBuffer> iBuf)
    : m_InterCnt(0)
    , m_ExtCnt(0)
    {
        m_iBuffer = iBuf;
    }
    
    ~BufferRef()
    {
    }
    
    void Retain()
    {
        
    }
    
    void Release()
    {
        
    }
    
private:
    id<MTLBuffer> m_iBuffer;
    int m_InterCnt;
    int m_ExtCnt;
};

class Buffer : public rhi::IGpuResource
{
public:
    Buffer(Device* device, rhi::ResourceDesc const & desc);
    ~Buffer();
    
    void *                      Map(uint64 start, uint64 size) override;
    void                        UnMap() override;
    
    uint64                      GetLocation() const override;
    rhi::ResourceDesc           GetDesc() const override;
    rhi::EResourceState         GetState() const override;
//    rhi::EGpuResourceType       GetType() const	override;
    uint64                      GetSize() const override;
    
private:
    NSRange             m_MapRange;
    id<MTLBuffer>       m_Buf;
    
    rhi::ResourceDesc   m_Desc;
    Device*             m_Device;
};

class Texture : public rhi::ITexture
{
public:
    Texture(Device* device, rhi::ResourceDesc const & desc);
    ~Texture();
    
    void *                      Map(uint64 start, uint64 size) override;
    void                        UnMap() override;
    
    uint64                      GetLocation() const override;
    rhi::ResourceDesc           GetDesc() const override;
    rhi::EResourceState         GetState() const override { return rhi::ERS_Unknown; }
//    rhi::EGpuResourceType       GetResourceType() const	override { return rhi::ResourceTypeNum; }
    uint64                      GetSize() const override;
    
    rhi::SamplerCRef			GetSampler() const override;
    void                        BindSampler(rhi::SamplerRef) override;
    void                        SetResourceView(rhi::ShaderResourceViewRef) override;
    rhi::ShaderResourceViewRef 	GetResourceView() const override;
    
private:
    id<MTLTexture>          m_Tex;
    MTLTextureDescriptor*   m_TexDesc;
    rhi::ResourceDesc       m_Desc;
    MTLStorageMode          storageMode;
    
    MTLTextureUsage         usage;
    Device*                 m_Device;
};


NS_K3D_METAL_END

#endif
