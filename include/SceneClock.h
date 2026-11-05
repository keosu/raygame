#pragma once

#include "GameTime.h"
#include "Scene.h"

namespace RayGame {

// Drives a scene the engine does not own: a preview inside a menu, a picture-in-picture view or
// a windowless simulation. Frames follow the engine's order (Update, fixed steps, interpolation,
// LateUpdate). Update and LateUpdate see the host frame's GameTime; fixed steps run on this
// clock's own time, so the driven scene never touches the engine's accumulator or time scale.
class SceneClock {
public:
    explicit SceneClock(float fixedDelta = 1.0f / 60.0f) { SetFixedDelta(fixedDelta); }

    void SetFixedDelta(float dt) {
        if (std::isfinite(dt) && dt > 0) fixedDelta = dt;
        accumulator = 0;
    }
    float GetFixedDelta() const { return fixedDelta; }
    // Simulated seconds per real second, clamped to [0, 8] like GameEngine::SetTimeScale.
    void SetTimeScale(float scale) {
        if (std::isfinite(scale)) timeScale = Clamp(scale, 0.0f, 8.0f);
    }
    float GetTimeScale() const { return timeScale; }
    double GetTime() const { return time; }
    float GetInterpolationAlpha() const { return (float)Clamp((float)(accumulator / fixedDelta), 0.0f, 1.0f); }
    void Reset() { accumulator = 0; time = 0; }

    // One rendered frame of `dt` real seconds; returns the fixed steps run. Like the engine it
    // runs at most 8 × ceil(time scale) steps per frame and drops the rest.
    int Advance(Scene& scene, float dt) {
        float real = std::isfinite(dt) ? std::max(0.0f, dt) : 0.0f;
        int maxSteps = 8 * std::max(1, (int)std::ceil(timeScale));
        ScopeActivation scope(scene.GetCallbackScope());
        scene.Update();
        accumulator += std::min(real * timeScale, fixedDelta * maxSteps);
        int steps = 0;
        while (accumulator + fixedDelta * 1e-5 >= fixedDelta && steps < maxSteps) {
            accumulator = std::max(0.0, accumulator - fixedDelta);
            ++steps;
            if (!Step(scene)) {
                accumulator = 0;
                scene.ResetInterpolation();
                break;
            }
        }
        scene.SetInterpolationAlpha(GetInterpolationAlpha());
        scene.LateUpdate();
        return steps;
    }

    // A single fixed step without Update or LateUpdate, for simulations that never render.
    // Returns false if the step cleared or reloaded the scene.
    bool Step(Scene& scene) {
        time += fixedDelta;
        GameTime::FixedStepScope fixed(fixedDelta, time);
        return scene.RunFixedStep(fixedDelta);
    }

private:
    float fixedDelta = 1.0f / 60.0f;
    float timeScale = 1;
    double accumulator = 0;
    double time = 0;
};

} // namespace RayGame
