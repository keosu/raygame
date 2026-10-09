#pragma once

#include "Core.h"

namespace RayGame {
    
class RAYGAME_API GameTime {
public:
    static void Update() { Update(GetFrameTime()); }

    static void Update(float dt) {
        Advance(dt, dt);
    }

    static void Advance(float realDelta, float simulationDelta) {
        unscaledDeltaTime = std::isfinite(realDelta) ? std::max(0.0f, realDelta) : 0.0f;
        deltaTime = std::isfinite(simulationDelta) ? std::max(0.0f, simulationDelta) : 0.0f;
        unscaledTime += unscaledDeltaTime;
        time += deltaTime;
        frameCount++;
    }
    
    static float DeltaTime() { return deltaTime; }
    static float UnscaledDeltaTime() { return unscaledDeltaTime; }
    static double UnscaledTime() { return unscaledTime; }
    static float FixedDeltaTime() { return fixedDeltaTime; }
    static void SetFixedDeltaTime(float dt) {
        if (std::isfinite(dt) && dt > 0) fixedDeltaTime = dt;
    }
    static void Reset() { deltaTime = 0; time = 0; frameCount = 0; unscaledDeltaTime = 0; unscaledTime = 0; }
    static float GetTime() { return time; }
    static int GetFrameCount() { return frameCount; }
    static float FPS() { return GetFPS(); }
    
private:
    static inline float deltaTime = 0.0f;
    static inline float unscaledDeltaTime = 0.0f;
    static inline double unscaledTime = 0.0;
    static inline float fixedDeltaTime = 1.0f / 60.0f;
    static inline float time = 0.0f;
    static inline int frameCount = 0;
};

}

