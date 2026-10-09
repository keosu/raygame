#pragma once

#include "Core.h"

namespace RayGame {

class RAYGAME_API ResourceManager {
public:
    static void Init() {}

    static void Shutdown() {
        for (auto& pair : textures) UnloadTexture(pair.second);
        textures.clear();
        for (auto& pair : fonts) UnloadFont(pair.second);
        fonts.clear();
        for (auto& pair : shaders) UnloadShader(pair.second);
        shaders.clear();
    }

    // ---- Textures ----
    static Texture2D* LoadTexture(const std::string& name, const std::string& filepath) {
        auto it = textures.find(name);
        if (it != textures.end()) return &it->second;

        Texture2D tex = ::LoadTexture(filepath.c_str());
        if (tex.id == 0) {
            TraceLog(LOG_ERROR, "Failed to load texture: %s", filepath.c_str());
            return nullptr;
        }
        textures[name] = tex;
        return &textures[name];
    }

    static Texture2D* GetTexture(const std::string& name) {
        auto it = textures.find(name);
        return it != textures.end() ? &it->second : nullptr;
    }

    static void UnloadTextureNamed(const std::string& name) {
        auto it = textures.find(name);
        if (it != textures.end()) { UnloadTexture(it->second); textures.erase(it); }
    }

    static Texture2D* CreateColorTexture(const std::string& name, int width, int height, Color color) {
        if (auto* cached = GetTexture(name)) return cached;
        if (width <= 0 || height <= 0) return nullptr;
        Image img = GenImageColor(width, height, color);
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        if (tex.id == 0) return nullptr;
        textures[name] = tex;
        return &textures[name];
    }

    static Texture2D* CreateCircleTexture(const std::string& name, int radius, Color color) {
        if (auto* cached = GetTexture(name)) return cached;
        if (radius <= 0) return nullptr;
        int size = radius * 2;
        Image img = GenImageColor(size, size, BLANK);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int dx = x - radius, dy = y - radius;
                if (dx*dx + dy*dy <= radius*radius) ImageDrawPixel(&img, x, y, color);
            }
        }
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        if (tex.id == 0) return nullptr;
        textures[name] = tex;
        return &textures[name];
    }

    // ---- Fonts ----
    static Font* LoadFontResource(const std::string& name, const std::string& filepath, int size = 32) {
        auto it = fonts.find(name);
        if (it != fonts.end()) return &it->second;
        Font f = LoadFontEx(filepath.c_str(), size, nullptr, 0);
        fonts[name] = f;
        return &fonts[name];
    }

    static Font* GetFont(const std::string& name) {
        auto it = fonts.find(name);
        return it != fonts.end() ? &it->second : nullptr;
    }

    static void UnloadFontNamed(const std::string& name) {
        auto it = fonts.find(name);
        if (it != fonts.end()) { UnloadFont(it->second); fonts.erase(it); }
    }

    // ---- Shaders ----
    static Shader* LoadShaderResource(const std::string& name, const std::string& vs, const std::string& fs) {
        auto it = shaders.find(name);
        if (it != shaders.end()) return &it->second;
        Shader s = ::LoadShader(vs.empty() ? nullptr : vs.c_str(),
                                fs.empty() ? nullptr : fs.c_str());
        shaders[name] = s;
        return &shaders[name];
    }

    static Shader* GetShader(const std::string& name) {
        auto it = shaders.find(name);
        return it != shaders.end() ? &it->second : nullptr;
    }

    static void UnloadShaderNamed(const std::string& name) {
        auto it = shaders.find(name);
        if (it != shaders.end()) { UnloadShader(it->second); shaders.erase(it); }
    }

private:
    static inline std::unordered_map<std::string, Texture2D> textures;
    static inline std::unordered_map<std::string, Font> fonts;
    static inline std::unordered_map<std::string, Shader> shaders;
};

}
