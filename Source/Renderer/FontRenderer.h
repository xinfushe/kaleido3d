#pragma once
#include <Interface/IRHI.h>
#include <KTL/DynArray.hpp>
#include <KTL/String.hpp>
#include <Math/kMath.hpp>

#include <unordered_map>

namespace render {
class TextQuad
{
public:
  int X;
  int Y;
  int W;
  int H;
  int HSpace;
  unsigned int* Pixels;
};

typedef ::k3d::DynArray<TextQuad> TextQuads;

class K3D_API FontManager
{
public:
  FontManager();
  ~FontManager();

  void LoadLib(const char* fontPath);
  void ChangeFontSize(int height);
  void SetPaintColor(int color);

  TextQuads AcquireText(const ::k3d::String& text);

private:
  int m_PixelSize;
  int m_Color;
  void* m_pFontLib;
  void* m_pFontFace;
  // Cache
};

class CharTexture
{
public:
  CharTexture(k3d::NGFXDeviceRef device, TextQuad const& quad);
  ~CharTexture();
  k3d::NGFXTextureRef GetTexture() const { return m_Texture; }

private:
  k3d::NGFXTextureRef m_Texture;
};

class CharRenderer
{
public:
  CharRenderer();
  ~CharRenderer();

  void InitVertexBuffers(k3d::NGFXDeviceRef const& device);

  // text atlas?

private:
  static short s_Indices[];
  static float s_Vertices[];
  static float s_CharTexCoords[];

private:
  k3d::NGFXResourceRef m_VertexBuffer;
  k3d::NGFXResourceRef m_IndexBuffer;
};

class FontRenderer
{
public:
  explicit FontRenderer(k3d::NGFXDeviceRef const& device);
  ~FontRenderer();
  void InitPSO(k3d::NGFXRenderpassRef pRenderPass);
  void Draw(k3d::String const& Text, kMath::Vec3f Position);

private:
  k3d::NGFXDeviceRef m_Device;
  k3d::RenderPipelineStateRef m_TextRenderPSO;
  FontManager m_FontManager;
  std::unordered_map<char, CharTexture> m_TexCache;
};
}