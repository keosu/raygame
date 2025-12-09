#pragma once

#include "Core.h"

namespace RayGame {
    
class RAYGAME_API GameTime {
public:
    static void Update() {
        deltaTime = GetFrameTime();
        time += deltaTime;
        frameCount++;
    }
    
    static float DeltaTime() { return deltaTime; }
    static float GetTime() { return time; }
    static int GetFrameCount() { return frameCount; }
    static float FPS() { return GetFPS(); }
    
private:
    static inline float deltaTime = 0.0f;
    static inline float time = 0.0f;
    static inline int frameCount = 0;
};

}

