#pragma once

#include "Core.h"
#include "Scene.h"
#include "GameTime.h"
#include "Input.h"
#include "Audio.h"
#include "TextToSpeech.h"
#include "ResourceManager.h"
#include "Physics.h"
#include "Tween.h"
#include "Event.h"
#include "UI.h"

namespace RayGame {

struct FrameTiming {
    int fixedSteps = 0;
    double droppedSeconds = 0;
    double totalDroppedSeconds = 0;
    double fixedTime = 0;
    float interpolationAlpha = 1;
};

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
            if (!applicationCallbacks.Token().IsAlive()) applicationCallbacks.Renew();
            ScopeActivation scope(GetCallbackScope());
            Input::Reset();
#if defined(__ANDROID__)
            SetConfigFlags(FLAG_FULLSCREEN_MODE);
            InitWindow(0, 0, windowTitle.c_str()); // Let Android supply the real surface size.
#else
            InitWindow(screenWidth, screenHeight, windowTitle.c_str());
#endif
            if (!IsWindowReady()) throw std::runtime_error("Cannot initialize the game window");
            initialized = true;
            RefreshWindowTheme(true);
#if !defined(__ANDROID__)
            SetWindowState(FLAG_WINDOW_RESIZABLE);
            SetWindowMinSize(std::max(320, screenWidth / 2), std::max(240, screenHeight / 2));
            int monitor = GetCurrentMonitor();
            float fit = std::min(1.0f, std::min(GetMonitorWidth(monitor) * 0.9f / screenWidth,
                                              GetMonitorHeight(monitor) * 0.85f / screenHeight));
            if (fit < 1) SetWindowSize((int)(screenWidth * fit), (int)(screenHeight * fit));
#endif
            SetExitKey(KEY_NULL);
            Display::Configure(screenWidth, screenHeight, canvasScaleMode);
            RefreshCanvas();
            if (startFullscreen) SetFullscreen(true);
            GameTime::Reset();
            timing = {};
            fixedAccumulator = 0;
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
        if (!initialized && !currentScene && !hasPendingScene) {
            applicationCallbacks.Cancel();
            TextToSpeech::Shutdown();
            return;
        }
        shuttingDown = true;
        running = false;
        applicationCallbacks.Cancel();
        UnloadCurrentScene();
        if (pendingScene) pendingScene->Clear();
        pendingScene.reset();
        hasPendingScene = false;
        fixedAccumulator = 0;
        Input::Reset();

        TextToSpeech::Shutdown();
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
    ScopeToken GetCallbackScope() const { return applicationCallbacks.Token(); }

    void Quit() { running = false; }

    bool IsFullscreen() const {
#if defined(__ANDROID__)
        return true;
#else
        return initialized ? IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) : startFullscreen;
#endif
    }
    void SetFullscreen(bool value) {
#if defined(__ANDROID__)
        (void)value; // NativeActivity owns the full screen surface.
#else
        startFullscreen = value;
        if (initialized && IsFullscreen() != value) {
            ToggleBorderlessWindowed();
            RefreshWindowTheme(true);
            RefreshCanvas();
        }
#endif
    }
    void ToggleFullscreen() { SetFullscreen(!IsFullscreen()); }

    void SetPaused(bool value) {
        if (paused == value) return;
        paused = value;
        if (paused) TextToSpeech::Stop();
        UI::ResetInteraction();
        fixedAccumulator = 0;
        Input::DiscardPending();
        timing.interpolationAlpha = 1;
        if (currentScene) currentScene->ResetInterpolation();
    }
    bool IsPaused() const { return paused; }

    // Games with their own menus can route Escape/P/F1/F11 themselves.
    void SetDefaultShortcutsEnabled(bool value) { defaultShortcuts = value; }
    bool AreDefaultShortcutsEnabled() const { return defaultShortcuts; }

    void ToggleDebug() { showDebug = !showDebug; }
    bool IsDebugEnabled() const { return showDebug; }

    void SetClearColor(Color c) { clearColor = c; }
    void SetFixedTimeStep(float dt) {
        if (!std::isfinite(dt) || dt <= 0 || dt == fixedDelta) return;
        if (ticking) throw std::logic_error("Change the fixed timestep between frames");
        fixedDelta = dt;
        fixedAccumulator = 0;
        timing.interpolationAlpha = 1;
        if (currentScene) currentScene->ResetInterpolation();
    }
    const FrameTiming& GetFrameTiming() const { return timing; }

    // Scales simulated time (0 freezes it, 2 runs twice as fast), clamped to [0, 8].
    // The fixed interval is unchanged: the per-frame catch-up budget of eight steps
    // grows with the scale, and unscaled time, pause and UI keep running normally.
    void SetTimeScale(float scale) {
        if (!std::isfinite(scale)) return;
        timeScale = Clamp(scale, 0.0f, 8.0f);
        GameTime::SetTimeScale(timeScale);
    }
    float GetTimeScale() const { return timeScale; }

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
    void Tick(float dt);

    Color clearColor = BLACK;
    int targetFPS = 60;
    float fixedDelta = 1.0f / 60.0f;

private:
    CallbackScope applicationCallbacks;
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
    float timeScale = 1;
    int MaxFixedSteps() const { return 8 * std::max(1, (int)std::ceil(timeScale)); }
    double fixedAccumulator = 0.0;
    FrameTiming timing;
    bool ticking = false;
    bool defaultShortcuts = true;

    void RefreshWindowTheme(bool force = false);

    void RefreshCanvas() {
        Display::Refresh();
        Vector2 size = Display::GetSize();
        if (size.x == lastCanvasSize.x && size.y == lastCanvasSize.y) return;
        Vector2 previous = lastCanvasSize;
        lastCanvasSize = size;
        if (currentScene && previous.x > 0 && previous.y > 0) {
            ScopeActivation sceneScope(currentScene->GetCallbackScope());
            currentScene->OnViewportChanged(previous, size);
            currentScene->ResetInterpolation();
        }
    }

    void UnloadCurrentScene() noexcept {
        auto old = std::move(currentScene);
        if (!old) return;
        try { ScopeActivation sceneScope(old->GetCallbackScope()); old->OnUnload(); }
        catch (...) { Diagnostics::Write(LogLevel::Error, "Scene OnUnload failed; continuing cleanup"); }
        old->Clear();
    }

    void ApplyPendingScene() {
        if (!hasPendingScene) return;
        auto next = std::move(pendingScene);
        hasPendingScene = false;

        UnloadCurrentScene();
        TextToSpeech::Stop();
        Input::DiscardPending();
        fixedAccumulator = 0;
        UI::ResetInteraction();

        currentScene = std::move(next);

        if (currentScene) {
            currentScene->AttachCallbackScope(GetCallbackScope());
            currentScene->Load();
            currentScene->ResetInterpolation();
        }
    }

    void Update() {
        Input::CaptureFrame();
        RefreshWindowTheme();
#if defined(__ANDROID__)
        if (defaultShortcuts && Input::GetKeyDown(KEY_BACK)) SetPaused(!paused);
        if (defaultShortcuts && !paused && Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT)
            && CheckCollisionPointRec(Input::GetMousePosition(), PauseButtonBounds())) SetPaused(true);
#endif
        if (defaultShortcuts && (Input::GetKeyDown(KEY_F11) || (Input::GetKeyDown(KEY_ENTER)
            && (Input::GetKey(KEY_LEFT_ALT) || Input::GetKey(KEY_RIGHT_ALT))))) ToggleFullscreen();
        if (defaultShortcuts && Input::GetKeyDown(KEY_ESCAPE)) {
            if (IsFullscreen()) SetFullscreen(false);
            else Quit();
        }
        if (defaultShortcuts && Input::GetKeyDown(KEY_P)) SetPaused(!paused);
        RefreshCanvas();
        AudioManager::UpdateMusic();

        if (defaultShortcuts && Input::GetKeyDown(KEY_F1)) ToggleDebug();
        Tick(GetFrameTime());
    }

    void Render() {
        ScopeActivation scope(GetCallbackScope());
        Display::Begin();
        UI::BeginFrame();
        struct FrameScope {
            ~FrameScope() { UI::inputEnabled = true; UI::EndFrame(); Display::End(); }
        } frame;
        ClearBackground(clearColor);
        UI::inputEnabled = !paused;

        if (currentScene) {
            ScopeActivation sceneScope(currentScene->GetCallbackScope());
            currentScene->Render();
        }

        OnRender();

        if (showDebug) {
            UI::Panel({12, 84, 250, 122});
            UI::Label(24, 92, TextFormat("FPS  %d", GetFPS()), 18);
            UI::Label(24, 114, TextFormat("OBJECTS  %d",
                currentScene ? (int)currentScene->GetAllGameObjects().size() : 0), 18);
            UI::Label(24, 136, TextFormat("COLLIDERS  %d", currentScene ? currentScene->GetPhysicsWorld().GetColliderCount() : 0), 18);
            UI::Label(24, 158, TextFormat("FIXED STEPS  %d", timing.fixedSteps), 18);
            UI::Label(24, 180, TextFormat("DROPPED  %.3f s", timing.totalDroppedSeconds), 18);
            auto* camera = currentScene ? currentScene->GetActiveCamera() : nullptr;
            if (camera) camera->Begin();
            if (currentScene) currentScene->GetPhysicsWorld().DebugDraw();
            if (camera) camera->End();
        }

#if defined(__ANDROID__)
        if (defaultShortcuts && !paused) {
            UI::Panel(PauseButtonBounds());
            UI::LabelCentered(Display::GetWidth() - 70, Display::GetHeight() - 38, "PAUSE", 18);
        }
#endif
        if (paused) {
            float width = Display::GetWidth(), height = Display::GetHeight();
            DrawRectangle(0, 0, (int)std::ceil(width), (int)std::ceil(height), {4, 7, 16, 190});
            UI::Panel({width / 2 - 190, height / 2 - 78, 380, 156});
            UI::TitleCentered(width / 2, height / 2 - 50, "PAUSED", 52);
#if defined(__ANDROID__)
            UI::inputEnabled = true;
            if (UI::Button({width / 2 - 170, height / 2 + 18, 160, 44}, "RESUME")) SetPaused(false);
            if (UI::Button({width / 2 + 10, height / 2 + 18, 160, 44}, "EXIT")) Quit();
#else
            UI::LabelCentered(width / 2, height / 2 + 18, "Press P to continue", 22);
#endif
        }
    }

    static Rectangle PauseButtonBounds() {
        return {Display::GetWidth() - 124, Display::GetHeight() - 46, 108, 34};
    }
};

}
