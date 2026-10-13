#pragma once
namespace Marble {
using RayGame::InputFrame;
using RayGame::SaveSystem;
inline int Game::Smoke() {
    int failures = 0;
    std::ofstream log(data / "smoke.log");
    auto check = [&](bool ok, const char* what) {
        log << (ok ? "PASS " : "FAIL ") << what << '\n';
        if (!ok)
            ++failures;
    };
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    // --smoke requires an explicit directory; reset only this test's campaign fixture.
    unlocked = 1;
    muted = true;
    Save();
    Init();
    SetTargetFPS(0);
    Step();
    Step();
    Capture("menu");
    check(screen == Campaign::Screen::Menu, "starts in menu");
    Click({640, 508}, true);
    check(screen == Campaign::Screen::Help, "touch opens instructions");
    Capture("help");
    Click({640, 624});
    Click({640, 442});
    check(screen == Campaign::Screen::Levels, "level selection");
    Click({640, 401});
    check(screen == Campaign::Screen::Levels, "locked level cannot start");
    Click({640, 584});
    Click({640, 376}, true);
    check(screen == Campaign::Screen::Play && model.stage == 0, "start adventure");
    int shots = model.shotsFired;
    InputFrame held;
    held.touchCount = 1;
    auto origin = Campaign::Origin();
    auto target = RayGame::Display::CanvasToScreen({1000 + origin.x, 180 + origin.y});
    held.touches[0] = {11, target};
    Step(held);
    held.touchCount = 2;
    held.touches[1] = {22, target};
    Step(held);
    held.touchCount = 1;
    held.touches[0] = {22, target};
    Step(held);
    Step();
    check(model.shotsFired == shots + 1,
          "one captured finger fires once without promoting second finger");
    for (int i = 0; i < 15; ++i)
        Step();
    shots = model.shotsFired;
    Click({1000, 180}, true);
    check(model.shotsFired == shots + 1, "touch release fires real projectile");
    int current = model.current, next = model.next;
    Click({640, 649});
    check(model.current == next && model.next == current, "swap button exchanges ammunition");
    for (int i = 0; i < 60; ++i)
        Step();
    Capture("courtyard");
    float elapsed = model.elapsed;
    Click({1090, 49}, true);
    for (int i = 0; i < 20; ++i)
        Step();
    check(screen == Campaign::Screen::Pause && model.elapsed - elapsed < 0.08f,
          "pause freezes fixed simulation");
    Capture("pause");
    Click({640, 418});
    int shotsBeforeFocus = model.shotsFired;
    held.touchCount = 1;
    held.touches[0] = {11, target};
    Step(held);
    InputFrame focus;
    focus.focused = false;
    Step(focus);
    check(screen == Campaign::Screen::Pause, "focus loss pauses");
    Step();
    check(model.shotsFired == shotsBeforeFocus, "focus loss cancels held shot");
    Click({640, 418});
    // Fixtures exercise the full result/menu transitions; normal-command campaigns are in
    // campaign_tests.
    for (int stage = 0; stage < 3; ++stage) {
        Begin(stage);
        Step();
        Capture(stage == 0 ? "stage1" : stage == 1 ? "stage2" : "stage3");
        model.balls.clear();
        model.remaining = 0;
        Step();
        check(screen == Campaign::Screen::Won && unlocked >= std::min(3, stage + 2),
              "victory unlocks and saves");
        Capture(stage == 2 ? "campaign-complete" : "victory");
        Click({640, 418});
        check(screen == Campaign::Screen::Play, "next level or replay through result button");
    }
    model.balls.front().distance = model.path.length;
    Step();
    check(screen == Campaign::Screen::Lost, "gate breach loses");
    Capture("defeat");
    Click({530, 482});
    check(screen == Campaign::Screen::Play && model.outcome == 0, "restart after defeat");
    for (auto size : {Vector2{1024, 768}, {1600, 720}, {720, 1000}}) {
        SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
        Step();
        Step();
        Click({1090, 49}, true);
        check(screen == Campaign::Screen::Pause, "adaptive touch hit boxes");
        Capture(TextFormat("layout-%dx%d", static_cast<int>(size.x), static_cast<int>(size.y)));
        Click({640, 418}, true);
    }
    SaveSystem saved;
    check(saved.Load(data / "campaign.save") && saved.GetInt("unlocked") == 3,
          "versioned campaign save");
    muted = true;
    Save();
    Shutdown();
    check(!IsWindowReady(), "clean graphics shutdown");
    Game restored(data);
    restored.Init();
    restored.Step();
    check(restored.unlocked == 3 && restored.muted, "progress and mute restored after restart");
    restored.Shutdown();
    log << failures << " failures\n";
    return failures ? 1 : 0;
}
} // namespace Marble
