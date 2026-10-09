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
#include "UI.h"

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
        RefreshWindowTheme(true);
        SetWindowState(FLAG_WINDOW_RESIZABLE);
        SetWindowMinSize(std::max(320, screenWidth / 2), std::max(240, screenHeight / 2));
        SetExitKey(KEY_NULL);
        int monitor = GetCurrentMonitor();
        float fit = std::min(1.0f, std::min(GetMonitorWidth(monitor) * 0.9f / screenWidth,
                                          GetMonitorHeight(monitor) * 0.85f / screenHeight));
        if (fit < 1) SetWindowSize((int)(screenWidth * fit), (int)(screenHeight * fit));
        Display::Configure(screenWidth, screenHeight);
        Display::Refresh();
        if (startFullscreen) SetFullscreen(true);
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
            UI::ResetFonts();
            Display::Shutdown();
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

    bool IsFullscreen() const {
        return initialized ? IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) : startFullscreen;
    }
    void SetFullscreen(bool value) {
        startFullscreen = value;
        if (initialized && IsFullscreen() != value) {
            ToggleBorderlessWindowed();
            RefreshWindowTheme(true);
            Display::Refresh();
        }
    }
    void ToggleFullscreen() { SetFullscreen(!IsFullscreen()); }

    void SetPaused(bool value) { paused = value; }
    bool IsPaused() const { return paused; }

    void ToggleDebug() { showDebug = !showDebug; }
    bool IsDebugEnabled() const { return showDebug; }

    void SetClearColor(Color c) { clearColor = c; }
    void SetFixedTimeStep(float dt) {
        if (std::isfinite(dt) && dt > 0) fixedDelta = dt;
    }

    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }
    void SetCanvasSize(int width, int height) {
        if (width <= 0 || height <= 0) return;
        screenWidth = width;
        screenHeight = height;
        if (initialized) { Display::Configure(width, height); Display::Refresh(); }
    }

protected:
    virtual void OnInit() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}

    // Advance logic without drawing; also useful for deterministic simulations.
    void Tick(float dt) {
        ApplyPendingScene();
        if (paused) { GameTime::Update(0); return; }
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
    bool startFullscreen = false;
    bool paused = false;
    float fixedAccumulator = 0.0f;

    void RefreshWindowTheme(bool force = false);

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
        RefreshWindowTheme();
        if (Input::GetKeyDown(KEY_F11) || (Input::GetKeyDown(KEY_ENTER)
            && (Input::GetKey(KEY_LEFT_ALT) || Input::GetKey(KEY_RIGHT_ALT)))) ToggleFullscreen();
        if (Input::GetKeyDown(KEY_ESCAPE)) {
            if (IsFullscreen()) SetFullscreen(false);
            else Quit();
        }
        if (Input::GetKeyDown(KEY_P)) paused = !paused;
        Display::Refresh();
        AudioManager::UpdateMusic();

        if (Input::GetKeyDown(KEY_F1)) ToggleDebug();
        Tick(GetFrameTime());
    }

    void Render() {
        Display::Begin();
        ClearBackground(clearColor);
        UI::inputEnabled = !paused;

        if (currentScene) currentScene->Render();

        OnRender();

        if (showDebug) {
            UI::Panel({12, 84, 210, 78});
            UI::Label(24, 92, TextFormat("FPS  %d", GetFPS()), 18);
            UI::Label(24, 114, TextFormat("OBJECTS  %d",
                currentScene ? (int)currentScene->GetAllGameObjects().size() : 0), 18);
            UI::Label(24, 136, TextFormat("COLLIDERS  %d", PhysicsWorld::GetColliderCount()), 18);
            auto* camera = currentScene ? currentScene->GetActiveCamera() : nullptr;
            if (camera) camera->Begin();
            PhysicsWorld::DebugDraw();
            if (camera) camera->End();
        }

        if (paused) {
            DrawRectangle(0, 0, screenWidth, screenHeight, {4, 7, 16, 190});
            UI::Panel({screenWidth / 2.0f - 190, screenHeight / 2.0f - 78, 380, 156});
            UI::TitleCentered(screenWidth / 2.0f, screenHeight / 2.0f - 50, "PAUSED", 52);
            UI::LabelCentered(screenWidth / 2.0f, screenHeight / 2.0f + 18, "Press P to continue", 22);
        }
        Display::End();
        UI::inputEnabled = true;
    }
};

}
