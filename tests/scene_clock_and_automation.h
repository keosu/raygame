#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

// SceneClock and the shared fixed step, audio ducks, canvas clipping math and scripted runs.
// None of these checks need a window.
namespace clock_tests {

struct PhaseLog : Component {
    std::string* log = nullptr;
    Scene* clearOnFixed = nullptr;
    bool fixedFlag = false;
    float fixedDelta = 0;
    void Update() override { *log += 'U'; }
    void FixedUpdate() override {
        *log += 'F';
        fixedFlag = GameTime::IsFixedStep();
        fixedDelta = GameTime::DeltaTime();
        if (clearOnFixed) clearOnFixed->Clear();
    }
    void PostFixedUpdate() override { *log += 'P'; }
    void LateUpdate() override { *log += 'L'; }
};

static void SceneClockFrames() {
    Scene scene;
    std::string log;
    auto phases = scene.CreateGameObject()->AddComponent<PhaseLog>();
    phases->log = &log;
    scene.Start();

    SceneClock clock(0.1f);
    CHECK(clock.Advance(scene, 0.25f) == 2);
    CHECK(log == "UFPFPL"); // engine order: update, fixed steps, late update
    CHECK(phases->fixedFlag && Approx(phases->fixedDelta, 0.1f));
    CHECK(Approx(clock.GetInterpolationAlpha(), 0.5f) && Approx(scene.GetInterpolationAlpha(), 0.5f));
    CHECK(Approx((float)clock.GetTime(), 0.2f));
    CHECK(!GameTime::IsFixedStep());

    log.clear();
    CHECK(clock.Advance(scene, 0.05f) == 1); // the remainder carries over
    CHECK(log == "UFPL");

    log.clear();
    clock.SetTimeScale(0);
    CHECK(clock.Advance(scene, 1.0f) == 0 && log == "UL"); // frozen simulation, frame callbacks still run

    clock.SetTimeScale(1);
    clock.Reset();
    CHECK(clock.Advance(scene, 5.0f) == 8); // catch-up budget; the rest is dropped
    CHECK(clock.Advance(scene, 0.0f) == 0);

    log.clear();
    CHECK(clock.Step(scene) && log == "FP"); // headless step: no frame callbacks
    CHECK(clock.Advance(scene, NAN) == 0);
}

static void FixedStepStops() {
    Scene scene;
    std::string log;
    auto phases = scene.CreateGameObject()->AddComponent<PhaseLog>();
    phases->log = &log;
    scene.Start();
    {
        GameTime::FixedStepScope fixed(0.1f, 0.1);
        CHECK(!scene.RunFixedStep(0.1f, [] { return false; }));
        CHECK(log.empty()); // only the pose capture ran
        CHECK(scene.RunFixedStep(0.1f) && log == "FP");
    }

    log.clear();
    phases->clearOnFixed = &scene;
    SceneClock clock(0.1f);
    CHECK(clock.Advance(scene, 0.35f) == 1); // the clear ends the step and the frame's catch-up
    CHECK(log == "UF");
}

static void AudioDucks() {
    CHECK(Approx(AudioManager::GetDuckFactor(AudioChannel::Sfx), 1));
    {
        std::optional<AudioManager::Duck> first, second;
        first.emplace(AudioChannel::Sfx, 0.5f);
        second.emplace(AudioChannel::Sfx, 0.4f);
        AudioManager::Duck music(AudioChannel::Music, 0.25f);
        CHECK(Approx(AudioManager::GetDuckFactor(AudioChannel::Sfx), 0.2f));
        CHECK(Approx(AudioManager::GetDuckFactor(AudioChannel::Music), 0.25f));
        first.reset(); // out of order
        CHECK(Approx(AudioManager::GetDuckFactor(AudioChannel::Sfx), 0.4f));
        AudioManager::Duck loud(AudioChannel::Sfx, 3.0f), invalid(AudioChannel::Sfx, NAN);
        CHECK(Approx(AudioManager::GetDuckFactor(AudioChannel::Sfx), 0.4f)); // clamped to [0, 1]
    }
    CHECK(Approx(AudioManager::GetDuckFactor(AudioChannel::Sfx), 1));
    CHECK(Approx(AudioManager::GetDuckFactor(AudioChannel::Music), 1));
    CHECK(Approx(AudioManager::GetChannelVolume(AudioChannel::Sfx), 1)); // the player's setting is untouched
}

static void CanvasClipMath() {
    // A 1280×720 canvas on a 1280×768 texture (padded to 64-pixel steps).
    Rectangle r = Display::CanvasToTargetPixels({100, 100, 200, 100}, 1280, 768, {1280, 720});
    CHECK(Approx(r.x, 100) && Approx(r.y, 106) && Approx(r.width, 200) && Approx(r.height, 108));
    // Supersampled twice for fullscreen.
    r = Display::CanvasToTargetPixels({100, 100, 200, 100}, 2560, 1472, {1280, 720});
    CHECK(Approx(r.x, 200) && Approx(r.y, 204) && Approx(r.width, 400) && Approx(r.height, 205));
    r = Display::CanvasToTargetPixels({10, 10, -5, -5}, 100, 100, {100, 100});
    CHECK(r.width == 0 && r.height == 0);
}

static void ScriptedRuns() {
    namespace fs = std::filesystem;
    fs::path folder = fs::temp_directory_path() / "raygame_automation_test";
    std::error_code error;
    fs::remove_all(folder, error);
    CHECK(!Automation::Active() && !Automation::HiddenWindow());
    CHECK(Automation::Start(folder, {true}));
    CHECK(Automation::Active() && Automation::HiddenWindow() && fs::is_directory(folder));
    CHECK(Automation::Frame() == 0 && Automation::Directory() == folder);

    std::string order;
    Automation::Every([&](int frame) { order += 'e' + std::to_string(frame); });
    Automation::At(2, [&] { order += 'a'; });
    Automation::At(1, [&] {
        order += 'b';
        Automation::Click({640, 360});
        Automation::PressKey(KEY_SPACE);
        Automation::Tap({10, 20}, 3);
    });
    InputFrame unfocused;
    unfocused.focused = false;

    Automation::BeginFrame();
    Input::CaptureFrame(unfocused);
    CHECK(Input::GetFrame().focused); // scripted runs never lose focus
    CHECK(Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT) && Input::GetKeyDown(KEY_SPACE));
    Vector2 mouse = Input::GetMousePosition();
    CHECK(Approx(mouse.x, 640, 0.01f) && Approx(mouse.y, 360, 0.01f));
    CHECK(Input::GetTouchCount() == 1 && Input::GetTouchId(0) == 3);

    Automation::BeginFrame();
    Input::CaptureFrame(unfocused);
    CHECK(Input::GetMouseButtonUp(MOUSE_BUTTON_LEFT) && !Input::GetMouseButton(MOUSE_BUTTON_LEFT));
    CHECK(Input::GetKeyUp(KEY_SPACE) && Input::GetTouchCount() == 0);

    Automation::BeginFrame();
    CHECK(order == "e1be2ae3"); // every-frame actions first, timed actions once
    CHECK(Automation::Frame() == 3);
    CHECK(!Automation::SaveCanvas("none")); // no canvas without a window

    Automation::Every([](int frame) { if (frame == 4) Automation::Stop(); });
    Automation::BeginFrame(); // stopping from inside an action is safe
    CHECK(!Automation::Active() && Automation::Frame() == 0);
    Input::CaptureFrame(unfocused);
    CHECK(!Input::GetFrame().focused); // the capture filter is gone

    fs::path file = folder / "file.txt";
    { std::ofstream(file) << "x"; }
    CHECK(!Automation::Start(file) && !Automation::Active());
    Input::Reset();
    fs::remove_all(folder, error);
}

static void Run() {
    printf("[scene clock and automation]\n");
    SceneClockFrames();
    FixedStepStops();
    AudioDucks();
    CanvasClipMath();
    ScriptedRuns();
}

} // namespace clock_tests
