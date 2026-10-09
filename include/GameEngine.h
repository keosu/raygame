#pragma once

#include "Core.h"
#include "Scene.h"
#include "GameTime.h"
#include "Input.h"
#include "Audio.h"
#include "ResourceManager.h"
#include "Physics.h"
#include "Tween.h"
#include "Event.h"

namespace RayGame {

class RAYGAME_API GameEngine {
public:
    GameEngine(const std::string& title, int width, int height)
        : windowTitle(title), screenWidth(width), screenHeight(height) {
        currentScene = nullptr;
        pendingScene = nullptr;
        hasPendingScene = false;
        running = false;
        showDebug = false;
    }

    virtual ~GameEngine() = default;

    void Init() {
        if (initialized) return;
        InitWindow(screenWidth, screenHeight, windowTitle.c_str());
        initialized = true;
        GameTime::Reset();
        SetTargetFPS(targetFPS);

        AudioManager::Init();
        ResourceManager::Init();

        OnInit();
    }

    void Run() {
        Init();

        ApplyPendingScene();

        running = true;

        while (!WindowShouldClose() && running) {
            Update();
            Render();
        }

        Shutdown();
    }

    void Shutdown() {
        running = false;
        if (currentScene) {
            currentScene->OnUnload();
            currentScene->Clear();
            currentScene.reset();
        }
        pendingScene.reset();
        hasPendingScene = false;
        fixedAccumulator = 0;

        PhysicsWorld::Clear();
        TweenManager::Clear();
        EventBus::Clear();
        if (initialized) {
            ResourceManager::Shutdown();
            AudioManager::Shutdown();

            CloseWindow();
            initialized = false;
        }
    }

    // Deferred — the swap happens at the start of the next frame.
    void LoadScene(Ref<Scene> scene) {
        pendingScene = scene;
        hasPendingScene = true;
    }

    Ref<Scene> GetCurrentScene() { return currentScene; }

    void Quit() { running = false; }

    void ToggleDebug() { showDebug = !showDebug; }
    bool IsDebugEnabled() const { return showDebug; }

    void SetClearColor(Color c) { clearColor = c; }
    void SetFixedTimeStep(float dt) {
        if (std::isfinite(dt) && dt > 0) fixedDelta = dt;
    }

    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }

protected:
    virtual void OnInit() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}

    // Advance logic without drawing; also useful for deterministic simulations.
    void Tick(float dt) {
        ApplyPendingScene();
        GameTime::Update(dt);
        GameTime::SetFixedDeltaTime(fixedDelta);
        TweenManager::Update(GameTime::DeltaTime());
        if (currentScene) currentScene->Update();
        fixedAccumulator += std::min(GameTime::DeltaTime(), fixedDelta * 8);
        int steps = 0;
        while (fixedAccumulator >= fixedDelta && steps++ < 8) {
            if (currentScene) currentScene->FixedUpdate();
            PhysicsWorld::Step();
            fixedAccumulator -= fixedDelta;
        }
        OnUpdate();
    }

    Color clearColor = BLACK;
    int targetFPS = 60;
    float fixedDelta = 1.0f / 60.0f;

private:
    std::string windowTitle;
    int screenWidth, screenHeight;
    Ref<Scene> currentScene;
    Ref<Scene> pendingScene;
    bool hasPendingScene;
    bool running;
    bool showDebug;
    bool initialized = false;
    float fixedAccumulator = 0.0f;

    void ApplyPendingScene() {
        if (!hasPendingScene) return;
        auto next = std::move(pendingScene);
        hasPendingScene = false;

        if (currentScene) {
            currentScene->OnUnload();
            currentScene->Clear();
            currentScene.reset();
        }
        PhysicsWorld::Clear();
        TweenManager::Clear();
        EventBus::Clear();
        fixedAccumulator = 0;

        currentScene = std::move(next);

        if (currentScene) {
            currentScene->OnLoad();
            currentScene->Start();
        }
    }

    void Update() {
        AudioManager::UpdateMusic();

        if (Input::GetKeyDown(KEY_F1)) ToggleDebug();
        Tick(GetFrameTime());
    }

    void Render() {
        BeginDrawing();
        ClearBackground(clearColor);

        if (currentScene) currentScene->Render();

        OnRender();

        if (showDebug) {
            DrawFPS(10, 10);
            DrawText(TextFormat("Objects: %d",
                currentScene ? (int)currentScene->GetAllGameObjects().size() : 0),
                10, 30, 20, YELLOW);
            DrawText(TextFormat("Colliders: %d", PhysicsWorld::GetColliderCount()), 10, 50, 20, YELLOW);
            auto* camera = currentScene ? currentScene->GetActiveCamera() : nullptr;
            if (camera) camera->Begin();
            PhysicsWorld::DebugDraw();
            if (camera) camera->End();
        }

        EndDrawing();
    }
};

}
