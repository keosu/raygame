#pragma once

#include "Core.h"
#include "ResourceHandle.h"

namespace RayGame {

// Texture sampling for a font atlas. Mipmaps keep large atlases (e.g. CJK text loaded at
// a big size and drawn small) from shimmering; use TEXTURE_FILTER_TRILINEAR with them.
struct FontOptions {
    bool mipmaps = false;
    int filter = -1; // a raylib TextureFilter value; -1 keeps raylib's default
};

class RAYGAME_API ResourceManager {
public:
    // Unique, sorted Unicode scalars used by UTF-8 `text`, ready for LoadFontResource.
    // Invalid sequences, surrogates and control characters are skipped. With
    // `includeAscii`, printable ASCII (32-126) is always included.
    static std::vector<int> CodepointsFromText(const std::string& text, bool includeAscii = true);
    // Applies now and after every successful reload of the named font.
    static void SetFontOptions(const std::string& name, const FontOptions& options);

    static void Init() {}
    static void Shutdown();
    // Load reuses ready entries. Reload replaces their native value; failure
    // preserves the old value and records Error(). Unload invalidates all copies.
    static TextureHandle LoadTexture(const std::string& name, const std::string& filepath);
    static bool ReloadTexture(const std::string& name, const std::string& filepath);
    static TextureHandle GetTexture(const std::string& name);
    static void UnloadTextureNamed(const std::string& name);
    static TextureHandle CreateColorTexture(const std::string& name, int width, int height, Color color);
    static TextureHandle CreateCircleTexture(const std::string& name, int radius, Color color);

    static FontHandle LoadFontResource(const std::string& name, const std::string& filepath, int size = 32);
    static bool ReloadFont(const std::string& name, const std::string& filepath, int size = 32);
    // Explicit Unicode scalars; the manager copies the list for raylib's loader.
    static FontHandle LoadFontResource(const std::string& name, const std::string& filepath, int size,
                                      const std::vector<int>& codepoints);
    static bool ReloadFont(const std::string& name, const std::string& filepath, int size,
                           const std::vector<int>& codepoints);
    static FontHandle GetFont(const std::string& name);
    static void UnloadFontNamed(const std::string& name);

    static ShaderHandle LoadShaderResource(const std::string& name, const std::string& vs, const std::string& fs);
    static bool ReloadShader(const std::string& name, const std::string& vs, const std::string& fs);
    static ShaderHandle GetShader(const std::string& name);
    static void UnloadShaderNamed(const std::string& name);
};
}
