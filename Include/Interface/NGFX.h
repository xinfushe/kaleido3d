#ifndef __NGFX_h__
#define __NGFX_h__

// for c
#include "NGFXEnums.h"

#if __cplusplus
#include "NGFXObjectBase.h"
/*
// Gpu Device enumerator
struct NGFXFactory : public NGFXNamedObject<false>
{
  void EnumDevice(uint32_t* Count, class NGFXDevice** Devices);
};

// Gpu Device
struct NGFXDevice : public NGFXNamedObject<false>
{
  virtual class NGFXRenderpassDescriptor* CreateRenderpassDescriptor() = 0;
  virtual class NGFXRenderpass* CreateRenderPass(class RenderpassDescriptor *) = 0;

  virtual class NGFXRenderPipeline* CreateRenderPipeline() = 0;
  virtual class NGFXComputePipeline* CreateComputePipeline() = 0;

  virtual class NGFXPipelineLayout* CreatePipelineLayout() = 0;

  virtual class NGFXFence* CreateFence() = 0;

  virtual class NGFXCommandQueue* CreateCommandQueue(NGFXCommandType const&) = 0;

  virtual class NGFXBuffer* CreateBuffer() = 0;
  virtual class NGFXTexture* CreateTexture() = 0;
  virtual class NGFXSampler* CreateSampler() = 0;

};

// Command Queue
struct NGFXCommandQueue : public NGFXNamedObject<false>
{
  virtual class NGFXCommandBuffer* CommandBuffer() = 0;
};

// Command Buffer
struct NGFXCommandBuffer : public NGFXNamedObject<false>
{
  virtual class NGFXRenderCommandEncoder* RenderCommandEncoder() = 0;
  virtual class NGFXComputeCommandEncoder* ComputeCommandEncoder() = 0;
  virtual class NGFXParallelRenderCommandEncoder* ParallelRenderCommandEncoder() = 0;

  virtual void Commit(class NGFXFence*) = 0;
};

// Command Encoder
struct NGFXCommandEncoder : public NGFXNamedObject<false>
{
  virtual void SetPipeline(class NGFXPipeline *) = 0;
  virtual void SetBindingGroup(class NGFXBindingGroup *) = 0;
  virtual void EndEncode() = 0;
};

// Render Command Encoder
struct NGFXRenderCommandEncoder : public NGFXCommandEncoder
{
  
};

// Compute Command Encoder
struct NGFXComputeCommandEncoder : public NGFXCommandEncoder
{
  
};

// Parallel Render Command Endcoder
struct NGFXParallelRenderCommandEncoder : public NGFXCommandEncoder
{
  virtual NGFXRenderCommandEncoder* RenderEncoder() = 0;
};
*/
#endif


#endif