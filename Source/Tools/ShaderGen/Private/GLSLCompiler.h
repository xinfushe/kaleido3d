#pragma once
#ifndef __GLSLCompiler_h__
#define __GLSLCompiler_h__

#include "Public/ShaderCompiler.h"
#include <cassert>
#include <vector>

namespace k3d 
{
	typedef std::vector<uint32> SPIRV_T;

    struct GLSLangCompiler : public k3d::IShCompiler
    {
        typedef ::k3d::SharedPtr<IShCompiler> Ptr;
        virtual k3d::NGFXShaderCompileResult Compile(
                                          String const& src,
                                          k3d::NGFXShaderDesc const& inOp,
                                          k3d::NGFXShaderBundle & bundle) override;
		GLSLangCompiler();
        ~GLSLangCompiler() override;
    };
    
}

#endif
