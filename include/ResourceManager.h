#pragma once

#include "Core.h"

namespace RayGame {

class RAYGAME_API ResourceManager {
public:
    static void Init() {
        // Default initialization
    }
    
    static void Shutdown() {
        // Unload all textures
        for (auto& pair : textures) {
            UnloadTexture(pair.second);
        }
        textures.clear();
    }
    
    static Texture2D* LoadTexture(const std::string& name, const std::string& filepath) {
        if (textures.find(name) != textures.end()) {
            return &textures[name];
        }
        
        Texture2D tex = ::LoadTexture(filepath.c_str());
        if (tex.id == 0) {
            TraceLog(LOG_ERROR, "Failed to load texture: %s", filepath.c_str());
            return nullptr;
        }
        
        textures[name] = tex;
        return &textures[name];
    }
    
    static Texture2D* GetTexture(const std::string& name) {
        if (textures.find(name) != textures.end()) {
            return &textures[name];
        }
        return nullptr;
    }
    
    static Texture2D* CreateColorTexture(const std::string& name, int width, int height, Color color) {
        Image img = GenImageColor(width, height, color);
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        
        textures[name] = tex;
        return &textures[name];
    }
    
    static Texture2D* CreateCircleTexture(const std::string& name, int radius, Color color) {
        int size = radius * 2;
        Image img = GenImageColor(size, size, BLANK);
        
        // Draw circle on image (simple algorithm)
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int dx = x - radius;
                int dy = y - radius;
                if (dx*dx + dy*dy <= radius*radius) {
                    ImageDrawPixel(&img, x, y, color);
                }
            }
        }
        
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        
        textures[name] = tex;
        return &textures[name];
    }
    
private:
    static inline std::unordered_map<std::string, Texture2D> textures;
};

}

