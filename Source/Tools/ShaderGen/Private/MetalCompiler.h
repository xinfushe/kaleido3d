#pragma once

#include "Public/ShaderCompiler.h"

namespace k3d
{
    class MetalCompiler : public k3d::IShCompiler
    {
    public:
        MetalCompiler();
        ~MetalCompiler() override;
        k3d::NGFXShaderCompileResult	Compile(String const& src,
                                    k3d::NGFXShaderDesc const& inOp,
                                    k3d::NGFXShaderBundle & bundle) override;
        const char *        GetVersion();
    
    private:
        bool m_IsMac = true;
    };
}
