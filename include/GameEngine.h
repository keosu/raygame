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
        if (width <= 0 || height <= 0) throw std::invalid_argument("Canvas dimensions must be positive");
        currentScene = nullptr;
        pendingScene = nullptr;
        hasPendingScene = false;
        running = false;
        showDebug = false;
    }

    virtual ~GameEngine() { Shutdown(); }

    void Init() {
        if (initialized) return;
        try {
            InitWindow(screenWidth, screenHeight, windowTitle.c_str());
            if (!IsWindowReady()) throw std::runtime_error("Cannot initialize the game window");
            initialized = true;
            RefreshWindowTheme(true);
            SetWindowState(FLAG_WINDOW_RESIZABLE);
            SetWindowMinSize(std::max(320, screenWidth / 2), std::max(240, screenHeight / 2));
            SetExitKey(KEY_NULL);
            int monitor = GetCurrentMonitor();
            float fit = std::min(1.0f, std::min(GetMonitorWidth(monitor) * 0.9f / screenWidth,
                                              GetMonitorHeight(monitor) * 0.85f / screenHeight));
            if (fit < 1) SetWindowSize((int)(screenWidth * fit), (int)(screenHeight * fit));
            Display::Configure(screenWidth, screenHeight, canvasScaleMode);
            RefreshCanvas();
            if (startFullscreen) SetFullscreen(true);
            GameTime::Reset();
            SetTargetFPS(targetFPS);

            AudioManager::Init();
            ResourceManager::Init();

            OnInit();
        } catch (...) {
            Diagnostics::Write(LogLevel::Error, "Game initialization failed");
            Shutdown();
            throw;
        }
    }

    void Run() {
        try {
            Init();

            ApplyPendingScene();

            running = true;

            while (!WindowShouldClose() && running) {
                Update();
                Render();
            }

            Shutdown();
        } catch (...) {
            Diagnostics::Write(LogLevel::Error, "Game stopped after an exception");
            Shutdown();
            throw;
        }
    }

    void Shutdown() noexcept {
        if (shuttingDown) return;
        if (!initialized && !currentScene && !hasPendingScene) return;
        shuttingDown = true;
        running = false;
        UnloadCurrentScene();
        if (pendingScene) pendingScene->Clear();
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
        shuttingDown = false;
    }

    // Deferred — the swap happens at the start of the next frame.
    void LoadScene(Ref<Scene> scene) {
        if (shuttingDown) throw std::logic_error("Cannot load a scene during shutdown");
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
            RefreshCanvas();
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

    int GetScreenWidth() const { return initialized ? (int)std::lround(Display::GetWidth()) : screenWidth; }
    int GetScreenHeight() const { return initialized ? (int)std::lround(Display::GetHeight()) : screenHeight; }
    void SetCanvasScaleMode(CanvasScaleMode mode) {
        canvasScaleMode = mode;
        if (initialized) { Display::Configure(screenWidth, screenHeight, mode); RefreshCanvas(); }
    }
    CanvasScaleMode GetCanvasScaleMode() const { return canvasScaleMode; }
    void SetCanvasSize(int width, int height) {
        if (width <= 0 || height <= 0) return;
        screenWidth = width;
        screenHeight = height;
        if (initialized) { Display::Configure(width, height, canvasScaleMode); RefreshCanvas(); }
    }

protected:
    virtual void OnInit() {}
    virtual void OnUnscaledUpdate() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}

    // Advance logic without drawing; also useful for deterministic simulations.
    void Tick(float dt) {
        ApplyPendingScene();
        float realDelta = std::isfinite(dt) ? std::max(0.0f, dt) : 0.0f;
        GameTime::Advance(realDelta, paused ? 0 : std::min(realDelta, fixedDelta * 8));
        OnUnscaledUpdate();
        if (paused) return;
        GameTime::SetFixedDeltaTime(fixedDelta);
        TweenManager::Update(GameTime::DeltaTime());
        if (currentScene) currentScene->Update();
        fixedAccumulator += GameTime::DeltaTime();
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
    bool shuttingDown = false;
    CanvasScaleMode canvasScaleMode = CanvasScaleMode::Fit;
    Vector2 lastCanvasSize = {0, 0};
    float fixedAccumulator = 0.0f;

    void RefreshWindowTheme(bool force = false);

    void RefreshCanvas() {
        Display::Refresh();
        Vector2 size = Display::GetSize();
        if (size.x == lastCanvasSize.x && size.y == lastCanvasSize.y) return;
        Vector2 previous = lastCanvasSize;
        lastCanvasSize = size;
        if (currentScene && previous.x > 0 && previous.y > 0) currentScene->OnViewportChanged(previous, size);
    }

    void UnloadCurrentScene() noexcept {
        auto old = std::move(currentScene);
        if (!old) return;
        try { old->OnUnload(); }
        catch (...) { Diagnostics::Write(LogLevel::Error, "Scene OnUnload failed; continuing cleanup"); }
        old->Clear();
    }

    void ApplyPendingScene() {
        if (!hasPendingScene) return;
        auto next = std::move(pendingScene);
        hasPendingScene = false;

        UnloadCurrentScene();
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
        RefreshCanvas();
        AudioManager::UpdateMusic();

        if (Input::GetKeyDown(KEY_F1)) ToggleDebug();
        Tick(GetFrameTime());
    }

    void Render() {
        Display::Begin();
        struct FrameScope {
            ~FrameScope() { UI::inputEnabled = true; Display::End(); }
        } frame;
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
            float width = Display::GetWidth(), height = Display::GetHeight();
            DrawRectangle(0, 0, (int)std::ceil(width), (int)std::ceil(height), {4, 7, 16, 190});
            UI::Panel({width / 2 - 190, height / 2 - 78, 380, 156});
            UI::TitleCentered(width / 2, height / 2 - 50, "PAUSED", 52);
            UI::LabelCentered(width / 2, height / 2 + 18, "Press P to continue", 22);
        }
    }
};

}
