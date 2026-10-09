#include "GameEngine.h"
#include "WindowStyle.h"

namespace RayGame {

void GameEngine::RefreshWindowTheme(bool force) {
#ifdef _WIN32
    detail::UpdateWindowTheme(GetWindowHandle(), force);
#else
    (void)force;
#endif
}

void GameEngine::Tick(float dt) {
    if (ticking) throw std::logic_error("Engine Tick cannot be re-entered");
    ticking = true;
    struct TickScope { bool& value; ~TickScope() { value = false; } } tickScope{ticking};
    ScopeActivation scope(GetCallbackScope());
    try {
        ApplyPendingScene();
        float realDelta = std::isfinite(dt) ? std::max(0.0f, dt) : 0.0f;
        float simulationDelta = paused ? 0 : std::min(realDelta, fixedDelta * 8);
        timing.fixedSteps = 0;
        timing.droppedSeconds = paused ? 0 : (double)realDelta - simulationDelta;
        timing.totalDroppedSeconds += timing.droppedSeconds;
        GameTime::Advance(realDelta, simulationDelta);
        OnUnscaledUpdate();
        if (paused) { Input::DiscardPending(); return; }
        GameTime::SetFixedDeltaTime(fixedDelta);
        TweenManager::Update(GameTime::DeltaTime());
        if (paused) return;
        if (currentScene) {
            ScopeActivation sceneScope(currentScene->GetCallbackScope());
            currentScene->Update();
        }
        if (paused) return;
        fixedAccumulator += GameTime::DeltaTime();
        while (fixedAccumulator + fixedDelta * 1e-5 >= fixedDelta && timing.fixedSteps < 8) {
            fixedAccumulator = std::max(0.0, fixedAccumulator - fixedDelta);
            ++timing.fixedSteps;
            timing.fixedTime += fixedDelta;
            Input::FixedTickScope inputTick;
            GameTime::FixedStepScope fixedTime(fixedDelta, timing.fixedTime);
            auto scene = currentScene; // Keep the scene alive throughout callbacks.
            if (scene) {
                ScopeActivation sceneScope(scene->GetCallbackScope());
                auto generation = scene->GetGeneration();
                auto canContinue = [&] { return !paused && scene->GetGeneration() == generation; };
                scene->BeginFixedStep();
                if (canContinue()) scene->FixedUpdate();
                if (canContinue()) scene->IntegratePhysics(fixedDelta);
                if (canContinue()) scene->GetPhysicsWorld().Step();
                if (canContinue()) scene->PostFixedUpdate();
                if (!canContinue()) {
                    fixedAccumulator = 0;
                    timing.interpolationAlpha = 1;
                    scene->ResetInterpolation();
                    if (paused) return;
                    break;
                }
            }
            if (paused) return;
        }
        timing.interpolationAlpha = (float)Clamp((float)(fixedAccumulator / fixedDelta), 0.0f, 1.0f);
        if (currentScene) currentScene->SetInterpolationAlpha(timing.interpolationAlpha);
        OnUpdate();
        if (!paused && currentScene) {
            ScopeActivation sceneScope(currentScene->GetCallbackScope());
            currentScene->LateUpdate();
        }
    } catch (...) {
        fixedAccumulator = 0;
        timing.interpolationAlpha = 1;
        if (currentScene) currentScene->ResetInterpolation();
        throw;
    }
}

} // namespace RayGame

