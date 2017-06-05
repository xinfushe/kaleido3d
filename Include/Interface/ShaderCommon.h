#ifndef __ShaderCommon_h__
#define __ShaderCommon_h__

#include "../KTL/DynArray.hpp"
#include "../KTL/String.hpp"
#include "NGFXEnums.h"

K3D_COMMON_NS
{

  /**
   * Format. Lang. Profile. Stage. Entry
   */
  struct NGFXShaderDesc
  {
    NGFXShaderFormat Format;
    NGFXShaderLang Lang;
    NGFXShaderProfile Profile;
    NGFXShaderType Stage;
    String EntryFunction;
  };

  KFORCE_INLINE::k3d::Archive& operator<<(::k3d::Archive& ar,
                                          NGFXShaderDesc const& attr)
  {
    ar << attr.Format << attr.Lang << attr.Profile << attr.Stage
       << attr.EntryFunction;
    return ar;
  }

  KFORCE_INLINE::k3d::Archive& operator>>(::k3d::Archive& ar, NGFXShaderDesc& attr)
  {
    ar >> attr.Format >> attr.Lang >> attr.Profile >> attr.Stage >>
      attr.EntryFunction;
    return ar;
  }

  struct NGFXShaderAttribute
  {
    String VarName;
    NGFXShaderSemantic VarSemantic;
    NGFXShaderDataType VarType;
    uint32 VarLocation;
    uint32 VarBindingPoint;
    uint32 VarCount;

    NGFXShaderAttribute(const String& name,
      NGFXShaderSemantic semantic,
      NGFXShaderDataType dataType,
      uint32 location,
      uint32 bindingPoint,
      uint32 varCount)
      : VarName(name)
      , VarSemantic(semantic)
      , VarType(dataType)
      , VarLocation(location)
      , VarBindingPoint(bindingPoint)
      , VarCount(varCount)
    {
    }
  };

  typedef ::k3d::DynArray<NGFXShaderAttribute> NGFXShaderAttributes;

  KFORCE_INLINE::k3d::Archive& operator<<(::k3d::Archive& ar,
    NGFXShaderAttribute const& attr)
  {
    ar << attr.VarSemantic << attr.VarType << attr.VarLocation
      << attr.VarBindingPoint << attr.VarCount << attr.VarName;
    return ar;
  }

  KFORCE_INLINE::k3d::Archive& operator>>(::k3d::Archive& ar, NGFXShaderAttribute& attr)
  {
    ar >> attr.VarSemantic >> attr.VarType >> attr.VarLocation >>
      attr.VarBindingPoint >> attr.VarCount >> attr.VarName;
    return ar;
  }

  struct NGFXShaderUniform
  {
    NGFXShaderUniform() K3D_NOEXCEPT
      : VarType(NGFXShaderDataType::NGFX_SHADER_VAR_UNKNOWN)
      , VarName("")
      , VarOffset(0)
      , VarSzArray(0)
    {
    }

    NGFXShaderUniform(NGFXShaderDataType type,
      String const& name,
      uint32 offset,
      uint32 szArray = 0)
      : VarType(type)
      , VarName(name)
      , VarOffset(offset)
      , VarSzArray(szArray)
    {
    }

    virtual ~NGFXShaderUniform() {}

    NGFXShaderDataType VarType;
    String VarName;
    uint32 VarOffset;
    uint32 VarSzArray;
  };

  KFORCE_INLINE::k3d::Archive& operator<<(::k3d::Archive& ar,
    NGFXShaderUniform const& attr)
  {
    ar << attr.VarType << attr.VarOffset << attr.VarSzArray << attr.VarName;
    return ar;
  }

  KFORCE_INLINE::k3d::Archive& operator>>(::k3d::Archive& ar, NGFXShaderUniform& attr)
  {
    ar >> attr.VarType >> attr.VarOffset >> attr.VarSzArray >> attr.VarName;
    return ar;
  }

  KFORCE_INLINE bool operator==(NGFXShaderUniform const& lhs, NGFXShaderUniform const& rhs)
  {
    return rhs.VarType == lhs.VarType && rhs.VarOffset == lhs.VarOffset &&
      rhs.VarSzArray == lhs.VarSzArray && rhs.VarName == lhs.VarName;
  }

  class NGFXShaderConstant : public NGFXShaderUniform
  {
  public:
    NGFXShaderConstant() {}
    ~NGFXShaderConstant() override {}
  };

  struct NGFXShaderBinding
  {
    NGFXShaderBinding() K3D_NOEXCEPT
      : VarType(NGFXShaderBindType::NGFX_SHADER_BIND_UNDEFINED)
      , VarName("")
      , VarStage(NGFX_SHADER_TYPE_VERTEX)
      , VarNumber(0)
    {
    }

    NGFXShaderBinding(NGFXShaderBindType t, std::string n, NGFXShaderType st, uint32 num)
      : VarType(t)
      , VarName(n.c_str())
      , VarStage(st)
      , VarNumber(num)
    {
    }

    NGFXShaderBindType VarType;
    String VarName;
    NGFXShaderType VarStage;
    uint32 VarNumber;
  };

  KFORCE_INLINE::k3d::Archive& operator<<(::k3d::Archive& ar,
    NGFXShaderBinding const& attr)
  {
    ar << attr.VarType << attr.VarStage << attr.VarNumber << attr.VarName;
    return ar;
  }

  KFORCE_INLINE::k3d::Archive& operator>>(::k3d::Archive& ar, NGFXShaderBinding& attr)
  {
    ar >> attr.VarType >> attr.VarStage >> attr.VarNumber >> attr.VarName;
    return ar;
  }

  KFORCE_INLINE bool operator==(NGFXShaderBinding const& lhs, NGFXShaderBinding const& rhs)
  {
    return rhs.VarType == lhs.VarType && rhs.VarStage == lhs.VarStage &&
      rhs.VarNumber == lhs.VarNumber && rhs.VarName == lhs.VarName;
  }

  struct NGFXShaderSet
  {
    typedef uint32 VarIndex;
    /*bool operator==(NGFXShaderSet const &rhs)
    {
    return ...
    }*/
  };

  struct NGFXShaderBindingTable
  {
    ::k3d::DynArray<NGFXShaderBinding> Bindings;
    ::k3d::DynArray<NGFXShaderUniform> Uniforms;
    ::k3d::DynArray<NGFXShaderSet::VarIndex> Sets;

    NGFXShaderBindingTable() K3D_NOEXCEPT = default;

    NGFXShaderBindingTable(NGFXShaderBindingTable const& rhs)
    {
      Bindings = rhs.Bindings;
      Uniforms = rhs.Uniforms;
      Sets = rhs.Sets;
    }

    NGFXShaderBindingTable& AddBinding(NGFXShaderBinding&& binding)
    {
      this->Bindings.Append(binding);
      return *this;
    }

    NGFXShaderBindingTable& AddUniform(NGFXShaderUniform&& uniform)
    {
      this->Uniforms.Append(uniform);
      return *this;
    }

    NGFXShaderBindingTable& AddSet(NGFXShaderSet::VarIndex const& set)
    {
      this->Sets.Append(set);
      return *this;
    }
  };

  KFORCE_INLINE::k3d::Archive& operator<<(::k3d::Archive& ar,
    NGFXShaderBindingTable const& attr)
  {
    ar << attr.Bindings << attr.Uniforms;
    return ar;
  }

  KFORCE_INLINE::k3d::Archive& operator>>(::k3d::Archive& ar,
    NGFXShaderBindingTable& attr)
  {
    ar >> attr.Bindings >> attr.Uniforms;
    return ar;
  }

  KFORCE_INLINE NGFXShaderBindingTable operator|(NGFXShaderBindingTable const& a,
    NGFXShaderBindingTable const& b)
  {
#define MERGE_A_B(Member)                                                      \
  \
if(!a.Member.empty())                                                          \
  {                                                                            \
    \
result.Member.AddAll(a.Member);                                                \
    \
if(!b.Member.empty())                                                          \
    \
{                                                                       \
      \
for(auto ele                                                                   \
        : b.Member)                                                            \
      \
{                                                                     \
        \
if(!a.Member.Contains(ele))                                                    \
        \
{                                                                   \
          \
result.Member.Append(ele);                                                     \
        \
}                                                                   \
      \
}                                                                     \
    \
}                                                                       \
  \
}
    NGFXShaderBindingTable result = {};
    MERGE_A_B(Bindings)
      MERGE_A_B(Uniforms)
      MERGE_A_B(Sets)
#undef MERGE_A_B
      return result;
  }

  enum NGFXShaderCompileResult
  {
    NGFX_SHADER_COMPILE_OK,
    NGFX_SHADER_COMPILE_FAILED,
  };

  ///
  struct NGFXShaderBundle
  {
    NGFXShaderDesc Desc;
    NGFXShaderBindingTable BindingTable;
    NGFXShaderAttributes Attributes;
    String RawData;
  };

  KFORCE_INLINE::k3d::Archive& operator<<(::k3d::Archive& ar,
                                          NGFXShaderBundle const& attr)
  {
    ar << attr.Desc << attr.BindingTable << attr.Attributes << attr.RawData;
    return ar;
  }

  KFORCE_INLINE::k3d::Archive& operator>>(::k3d::Archive& ar,
                                          NGFXShaderBundle& attr)
  {
    ar >> attr.Desc >> attr.BindingTable >> attr.Attributes >> attr.RawData;
    return ar;
  }
}

#endif