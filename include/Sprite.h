#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "Time.h"

namespace RayGame {

class RAYGAME_API SpriteRenderer : public Component {
public:
    Texture2D* texture = nullptr;
    Rectangle sourceRect = {0, 0, 0, 0};
    Color tint = WHITE;
    Vector2 pivot = {0.5f, 0.5f};  // Center by default
    int layer = 0;
    bool flipX = false;
    bool flipY = false;
    
    SpriteRenderer() = default;
    
    SpriteRenderer(Texture2D* tex) : texture(tex) {
        if (texture) {
            sourceRect = {0, 0, (float)texture->width, (float)texture->height};
        }
    }
    
    SpriteRenderer(Texture2D* tex, Rectangle src) 
        : texture(tex), sourceRect(src) {}
    
    void Render() override {
        if (!texture || !gameObject) return;
        
        auto tf = gameObject->transform;
        
        Rectangle dest = {
            tf->position.x,
            tf->position.y,
            sourceRect.width * tf->scale.x * (flipX ? -1 : 1),
            sourceRect.height * tf->scale.y * (flipY ? -1 : 1)
        };
        
        Vector2 origin = {
            sourceRect.width * pivot.x * tf->scale.x,
            sourceRect.height * pivot.y * tf->scale.y
        };
        
        DrawTexturePro(*texture, sourceRect, dest, origin, 
                      tf->rotation, tint);
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
    Texture2D* spriteSheet = nullptr;
    std::vector<AnimationFrame> frames;
    bool loop = true;
    bool playing = true;
    int currentFrame = 0;
    float frameTimer = 0.0f;
    
    Color tint = WHITE;
    Vector2 pivot = {0.5f, 0.5f};
    bool flipX = false;
    bool flipY = false;
    
    SpriteAnimation() = default;
    
    void AddFrame(Rectangle rect, float duration = 0.1f) {
        frames.push_back({rect, duration});
    }
    
    void CreateFramesFromGrid(int frameWidth, int frameHeight, 
                            int frameCount, float frameDuration = 0.1f) {
        if (!spriteSheet) return;
        
        int columns = spriteSheet->width / frameWidth;
        
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
        
        frameTimer += GameTime::DeltaTime();
        
        if (frameTimer >= frames[currentFrame].duration) {
            frameTimer = 0;
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
        if (!spriteSheet || frames.empty() || !gameObject) return;
        
        auto tf = gameObject->transform;
        auto& frame = frames[currentFrame];
        
        Rectangle dest = {
            tf->position.x,
            tf->position.y,
            frame.sourceRect.width * tf->scale.x * (flipX ? -1 : 1),
            frame.sourceRect.height * tf->scale.y * (flipY ? -1 : 1)
        };
        
        Vector2 origin = {
            frame.sourceRect.width * pivot.x * tf->scale.x,
            frame.sourceRect.height * pivot.y * tf->scale.y
        };
        
        DrawTexturePro(*spriteSheet, frame.sourceRect, dest, origin,
                      tf->rotation, tint);
    }
};

}

