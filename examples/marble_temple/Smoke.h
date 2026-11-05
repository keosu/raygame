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
    CheckStartup(check);
    CheckSettings(check);
    chinese = false;
    ApplySettings();
    Step();
    Capture("menu-en");
    chinese = true;
    ApplySettings();
    SetTargetFPS(0);
    check(BallTextures[0] && BallTextures[4] && BallShadow, "filtered marble assets loaded");
    check(screen == Campaign::Screen::Menu, "starts in menu");
    ClickButton(3, true);
    check(screen == Campaign::Screen::Help, "touch opens instructions");
    Capture("help");
    Click({640, 624});
    ClickButton(2);
    check(screen == Campaign::Screen::Levels, "level selection");
    Capture("levels");
    ClickButton(21);
    check(screen == Campaign::Screen::Levels, "locked level cannot start");
    ClickButton(5);
    ClickButton(1, true);
    check(screen == Campaign::Screen::Play && model.stage == 0, "start adventure");
    check(HudVisible(), "top HUD is fully visible after starting");
    CheckPauseSettings(check);
    CheckFullscreenHud(check);
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
    {
        // An inserted marble glides in from the impact; the balls ahead slide instead of jumping.
        size_t hit = model.balls.size() / 2;
        int color = 0;
        while (color == model.balls[hit - 1].color || color == model.balls[hit].color ||
               color == model.balls[hit + 1].color)
            ++color;
        auto impact = model.path.At(model.balls[hit].distance);
        model.Insert(hit, color, {impact.x + 40, impact.y - 40});
        auto gliding = [&] {
            return std::any_of(model.balls.begin(), model.balls.end(), [](const Ball& b) { return b.settle < 1; });
        };
        Step();
        check(model.balls[0].lag > 1 && gliding(), "insertion animates the chain");
        Capture("insert-glide");
        for (int i = 0; i < 40; ++i)
            Step();
        check(model.balls[0].lag == 0 && !gliding(), "insertion settles");
        model.Erase(0, 3, true);
        for (int i = 0; i < 10; ++i)
            Step();
        check(!model.popups.empty(), "cleared group shows its points");
        Capture("clear-popup");
    }
    float elapsed = model.elapsed;
    Click({1090, 49}, true);
    for (int i = 0; i < 20; ++i)
        Step();
    check(screen == Campaign::Screen::Pause && model.elapsed - elapsed < 0.08f,
          "pause freezes fixed simulation");
    Capture("pause");
    ClickButton(6);
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
    ClickButton(6);
    // Fixtures exercise the full result/menu transitions; normal-command campaigns are in
    // campaign_tests.
    for (int stage = 0; stage < 3; ++stage) {
        Begin(stage);
        Step();
        Capture(stage == 0 ? "stage1" : stage == 1 ? "stage2" : "stage3");
        chinese = false;
        ApplySettings();
        Step();
        Capture(TextFormat("stage%d-en", stage + 1));
        chinese = true;
        ApplySettings();
        SetTargetFPS(0);
        model.balls.clear();
        model.remaining = 0;
        Step();
        check(screen == Campaign::Screen::Won && unlocked >= std::min(3, stage + 2),
              "victory unlocks and saves");
        Capture(stage == 2 ? "campaign-complete" : "victory");
        ClickButton(6);
        check(screen == Campaign::Screen::Play, "next level or replay through result button");
    }
    model.balls.front().distance = model.path.length;
    Step();
    check(screen == Campaign::Screen::Lost, "gate breach loses");
    Capture("defeat");
    ClickButton(7);
    check(screen == Campaign::Screen::Play && model.outcome == 0, "restart after defeat");
    for (auto size : {Vector2{1024, 768}, {1600, 720}, {720, 1000}}) {
        SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
        Step();
        Step();
        check(HudVisible(), "adaptive layout retains the full top HUD");
        Click({1090, 49}, true);
        check(screen == Campaign::Screen::Pause, "adaptive touch hit boxes");
        Capture(TextFormat("layout-%dx%d", static_cast<int>(size.x), static_cast<int>(size.y)));
        ClickButton(6, true);
        SetScreen(Campaign::Screen::Menu);
        Step();
        Capture(TextFormat("menu-%dx%d", static_cast<int>(size.x), static_cast<int>(size.y)));
        check(MenuPanelVisible(), "menu renders at its adaptive logical position");
        ClickButton(1, true);
        check(screen == Campaign::Screen::Play, "responsive menu touch mapping");
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
    check(restored.chinese && restored.smooth && restored.fps == 120,
          "language, quality and frame settings restored");
    restored.Shutdown();
    log << failures << " failures\n";
    return failures ? 1 : 0;
}
} // namespace Marble
