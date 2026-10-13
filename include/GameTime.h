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
    
    // Fixed callbacks observe the fixed interval and the current tick's time.
    // The frame view is restored even when a callback throws.
    class FixedStepScope {
    public:
        FixedStepScope(float dt, double time) : previousFixed(inFixedStep), previousDelta(fixedDeltaTime), previousTime(fixedTime) {
            inFixedStep = true; fixedDeltaTime = dt; fixedTime = time;
        }
        ~FixedStepScope() { inFixedStep = previousFixed; fixedDeltaTime = previousDelta; fixedTime = previousTime; }
        FixedStepScope(const FixedStepScope&) = delete;
        FixedStepScope& operator=(const FixedStepScope&) = delete;
    private:
        bool previousFixed;
        float previousDelta;
        double previousTime;
    };
    static bool IsFixedStep() { return inFixedStep; }
    static float DeltaTime() { return inFixedStep ? fixedDeltaTime : deltaTime; }
    static float UnscaledDeltaTime() { return unscaledDeltaTime; }
    static double UnscaledTime() { return unscaledTime; }
    static float FixedDeltaTime() { return fixedDeltaTime; }
    static void SetFixedDeltaTime(float dt) {
        if (std::isfinite(dt) && dt > 0) fixedDeltaTime = dt;
    }
    static void Reset() { deltaTime = 0; time = 0; frameCount = 0; unscaledDeltaTime = 0; unscaledTime = 0; }
    // Mirrors GameEngine::SetTimeScale for components that adapt effects to fast-forward.
    static float GetTimeScale() { return timeScale; }
    static void SetTimeScale(float scale) { if (std::isfinite(scale)) timeScale = Clamp(scale, 0.0f, 8.0f); }
    static float GetTime() { return inFixedStep ? (float)fixedTime : time; }
    static int GetFrameCount() { return frameCount; }
    static float FPS() { return GetFPS(); }
    
private:
    static inline bool inFixedStep = false;
    static inline double fixedTime = 0;
    static inline float deltaTime = 0.0f;
    static inline float unscaledDeltaTime = 0.0f;
    static inline double unscaledTime = 0.0;
    static inline float fixedDeltaTime = 1.0f / 60.0f;
    static inline float timeScale = 1.0f;
    static inline float time = 0.0f;
    static inline int frameCount = 0;
};

}

