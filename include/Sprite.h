#pragma once

#include "Core.h"
#include "ResourceHandle.h"
#include "Component.h"
#include "GameObject.h"
#include "GameTime.h"

namespace RayGame {

namespace detail {
inline void DrawMissingSprite(Rectangle source, const Transform& transform, Vector2 pivot) {
    auto position = transform.GetRenderPosition();
    auto scale = transform.GetRenderScale();
    Rectangle dest = {position.x, position.y,
        std::fabs((source.width != 0 ? source.width : 16) * scale.x),
        std::fabs((source.height != 0 ? source.height : 16) * scale.y)};
    DrawRectanglePro(dest, {dest.width * pivot.x, dest.height * pivot.y},
                     transform.GetRenderRotation(), MAGENTA);
}
inline void DrawSprite(Texture2D texture, Rectangle source, const Transform& transform,
                       Vector2 pivot, bool flipX, bool flipY, Color tint) {
    Vector2 position = transform.GetRenderPosition();
    Vector2 scale = transform.GetRenderScale();
    Rectangle dest = {position.x, position.y,
                      std::fabs(source.width * scale.x), std::fabs(source.height * scale.y)};
    if (flipX != (scale.x < 0)) source.width = -source.width;
    if (flipY != (scale.y < 0)) source.height = -source.height;
    DrawTexturePro(texture, source, dest, {dest.width * pivot.x, dest.height * pivot.y},
                   transform.GetRenderRotation(), tint);
}
}


class RAYGAME_API SpriteRenderer : public Component {
public:
    TextureHandle texture = nullptr;
    Rectangle sourceRect = {0, 0, 0, 0};
    Color tint = WHITE;
    Vector2 pivot = {0.5f, 0.5f};  // Center by default
    int layer = 0;
    bool flipX = false;
    bool flipY = false;
    
    SpriteRenderer() = default;
    
    SpriteRenderer(TextureHandle tex) : texture(tex) {
        if (texture) {
            sourceRect = {0, 0, (float)texture->width, (float)texture->height};
        }
    }
    
    SpriteRenderer(TextureHandle tex, Rectangle src)
        : texture(tex), sourceRect(src) {}

    int RenderLayer() const override { return layer; }

    void Render() override {
        if (!gameObject) return;
        if (!texture) { detail::DrawMissingSprite(sourceRect, *gameObject->transform, pivot); return; }
        
        detail::DrawSprite(*texture, sourceRect, *gameObject->transform, pivot, flipX, flipY, tint);
    }
    
    Vector2 GetSize() const {
        return {sourceRect.width, sourceRect.height};
    }
};

// Animation frame
struct AnimationFrame {
    Rectangle sourceRect;
    float duration;
};

class RAYGAME_API SpriteAnimation : public Component {
public:
    TextureHandle spriteSheet = nullptr;
    std::vector<AnimationFrame> frames;
    bool loop = true;
    bool playing = true;
    int currentFrame = 0;
    float frameTimer = 0.0f;
    
    Color tint = WHITE;
    Vector2 pivot = {0.5f, 0.5f};
    bool flipX = false;
    bool flipY = false;
    int layer = 0;

    int RenderLayer() const override { return layer; }

    SpriteAnimation() = default;
    
    void AddFrame(Rectangle rect, float duration = 0.1f) {
        frames.push_back({rect, duration});
    }
    
    void CreateFramesFromGrid(int frameWidth, int frameHeight, 
                            int frameCount, float frameDuration = 0.1f) {
        if (!spriteSheet || frameWidth <= 0 || frameHeight <= 0) return;
        
        int columns = spriteSheet->width / frameWidth;
        if (columns <= 0) return;
        
        for (int i = 0; i < frameCount; i++) {
            int x = (i % columns) * frameWidth;
            int y = (i / columns) * frameHeight;
            AddFrame({(float)x, (float)y, (float)frameWidth, (float)frameHeight}, 
                    frameDuration);
        }
    }
    
    void Play() { playing = true; }
    void Pause() { playing = false; }
    void Stop() { playing = false; currentFrame = 0; frameTimer = 0; }
    
    void Update() override {
        if (!playing || frames.empty()) return;
        if (currentFrame < 0 || currentFrame >= (int)frames.size()) currentFrame = 0;
        
        frameTimer += GameTime::DeltaTime();
        
        while (playing && frameTimer >= std::max(0.0001f, frames[currentFrame].duration)) {
            frameTimer -= std::max(0.0001f, frames[currentFrame].duration);
            currentFrame++;
            
            if (currentFrame >= frames.size()) {
                if (loop) {
                    currentFrame = 0;
                } else {
                    currentFrame = frames.size() - 1;
                    playing = false;
                }
            }
        }
    }
    
    void Render() override {
        if (frames.empty() || !gameObject) return;
        
        if (currentFrame < 0 || currentFrame >= (int)frames.size()) return;
        if (!spriteSheet) {
            detail::DrawMissingSprite(frames[currentFrame].sourceRect, *gameObject->transform, pivot);
            return;
        }
        detail::DrawSprite(*spriteSheet, frames[currentFrame].sourceRect,
                           *gameObject->transform, pivot, flipX, flipY, tint);
    }
};

}

