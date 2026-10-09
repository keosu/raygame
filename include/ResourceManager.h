#pragma once

#include "Core.h"
#include "ResourceHandle.h"

namespace RayGame {
class RAYGAME_API ResourceManager {
public:
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
    static FontHandle GetFont(const std::string& name);
    static void UnloadFontNamed(const std::string& name);

    static ShaderHandle LoadShaderResource(const std::string& name, const std::string& vs, const std::string& fs);
    static bool ReloadShader(const std::string& name, const std::string& vs, const std::string& fs);
    static ShaderHandle GetShader(const std::string& name);
    static void UnloadShaderNamed(const std::string& name);
};
}
