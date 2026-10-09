#pragma once

#include "Core.h"

namespace RayGame {
    
class RAYGAME_API GameTime {
public:
    static void Update() { Update(GetFrameTime()); }

    static void Update(float dt) {
        deltaTime = std::isfinite(dt) ? std::max(0.0f, dt) : 0.0f;
        time += deltaTime;
        frameCount++;
    }
    
    static float DeltaTime() { return deltaTime; }
    static float FixedDeltaTime() { return fixedDeltaTime; }
    static void SetFixedDeltaTime(float dt) {
        if (std::isfinite(dt) && dt > 0) fixedDeltaTime = dt;
    }
    static void Reset() { deltaTime = 0; time = 0; frameCount = 0; }
    static float GetTime() { return time; }
    static int GetFrameCount() { return frameCount; }
    static float FPS() { return GetFPS(); }
    
private:
    static inline float deltaTime = 0.0f;
    static inline float fixedDeltaTime = 1.0f / 60.0f;
    static inline float time = 0.0f;
    static inline int frameCount = 0;
};

}

