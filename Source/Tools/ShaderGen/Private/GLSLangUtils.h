#pragma once
#include <Interface/ICrossShaderCompiler.h>
#include <glslang/GlslangToSpv.h>

void sInitializeGlSlang();
void sFinializeGlSlang();

void initResources(TBuiltInResource &resources);
EShLanguage findLanguage(const NGFXShaderType shader_type);
NGFXShaderDataType glTypeToRHIAttribType(int glType);
NGFXShaderDataType glTypeToRHIUniformType(int glType);
NGFXShaderDataType glslangDataTypeToRHIDataType(const glslang::TType& type);
NGFXShaderBindType glslangTypeToRHIType(const glslang::TBasicType& type);

void ExtractAttributeData(const glslang::TProgram& program, k3d::NGFXShaderAttributes& shAttributes);
void ExtractUniformData(NGFXShaderType const& type, const glslang::TProgram& program, k3d::NGFXShaderBindingTable& outUniformLayout);
