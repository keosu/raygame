#pragma once
namespace Garden {
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
    Click({640, 508}, true);
    check(screen == Campaign::Screen::Help, "instructions via touch");
    Capture("help");
    Click({640, 624});
    Click({640, 376}, true);
    check(screen == Campaign::Screen::Play, "start adventure");
    check(HudVisible(), "top HUD is fully visible after starting");
    CheckPauseSettings(check);
    CheckFullscreenHud(check);
    auto drag = [&](Vector2 from, Vector2 to, bool touch, bool cancel = false) {
        auto origin = Campaign::Origin();
        auto mapped = [&](Vector2 p) {
            return RayGame::Display::CanvasToScreen({p.x + origin.x, p.y + origin.y});
        };
        InputFrame input;
        if (touch) {
            input.touchCount = 1;
            input.touches[0] = {71, mapped(from)};
        } else {
            input.mousePosition = mapped(from);
            input.mouse.held[0] = input.mouse.pressed[0] = true;
        }
        Step(input);
        if (touch)
            input.touches[0].position = mapped(to);
        else {
            input.mousePosition = mapped(to);
            input.mouse.pressed[0] = false;
        }
        Step(input);
        check(dragActive, "drag creates live plant preview");
        Capture(touch ? "drag-touch" : "drag-mouse");
        input.focused = !cancel;
        if (!cancel) {
            if (touch)
                input.touchCount = 0;
            else {
                input.mouse.held[0] = false;
                input.mouse.released[0] = true;
            }
        }
        Step(input);
        Step();
    };
    drag({304, 140}, Center(1, 0), true);
    check(model.PlantAt(1, 0) >= 0 && model.sun == 200 && !dragActive,
          "touch drag plants once and spends sun on release");
    drag({304, 140}, Center(1, 1), false);
    check(model.PlantAt(1, 1) < 0 && model.sun == 200, "drag respects seed cooldown");
    drag({458, 140}, {100, 660}, false);
    check(model.sun == 200 && model.plants.size() == 1, "outside drop cancels without spending");
    model.suns.push_back({901, Center(2, 1)});
    drag({458, 140}, Center(2, 1), false);
    check(model.PlantAt(2, 1) >= 0 && model.sun == 100 && model.suns.size() == 1,
          "mouse drag plants without collecting a sun under the drop");
    drag({1180, 140}, Center(2, 1), true);
    check(model.PlantAt(2, 1) < 0, "dragged shovel removes plant");
    drag({458, 140}, Center(3, 1), true, true);
    check(screen == Campaign::Screen::Pause && model.PlantAt(3, 1) < 0 && !dragActive,
          "focus loss cancels drag without planting");
    Begin(0);
    Step();
    Click({304, 140}, true);
    Click(Center(0, 0), true);
    check(model.PlantAt(0, 0) >= 0 && model.plants[0].kind == Kind::Sunflower && model.sun == 200,
          "seed selection, cost and touch planting");
    int count = static_cast<int>(model.plants.size());
    Click(Center(0, 0));
    check(static_cast<int>(model.plants.size()) == count, "occupied tile rejected");
    Click({458, 140});
    Click(Center(2, 2));
    check(model.PlantAt(2, 2) >= 0, "peashooter placed");
    Capture("plant-pop");
    model.suns.push_back({100, {985, 420}});
    int sun = model.sun;
    Click({985, 420}, true);
    check(model.sun == sun + 25, "real sun token collection");
    check(flights.size() == 1, "collected sun flies toward the counter");
    for (int i = 0; i < 12; ++i)
        Step();
    Capture("sun-flight");
    for (int i = 0; i < 22 && !flights.empty(); ++i)
        Step();
    check(flights.empty() && counterPulse > 0.8f, "sun lands on the counter and pulses it");
    Step();
    Capture("sun-landed");
    model.suns.push_back({101, {700, 420}, 0, {700, Top - 130}, 3});
    for (int i = 0; i < 60; ++i)
        Step();
    check(model.suns.back().At().y < 420 - 50, "sky sun drifts down before resting");
    Capture("sky-sun");
    Click({1180, 140});
    Click(Center(0, 0));
    check(model.PlantAt(0, 0) < 0, "shovel removes plant");
    Click({304, 140});
    auto origin = Campaign::Origin();
    auto cell = Center(1, 0);
    InputFrame held;
    held.touchCount = 1;
    held.touches[0] = {17,
                       RayGame::Display::CanvasToScreen({cell.x + origin.x, cell.y + origin.y})};
    Step(held);
    held.focused = false;
    Step(held);
    Step();
    check(screen == Campaign::Screen::Pause && model.PlantAt(1, 0) < 0,
          "focus loss cancels held planting touch");
    Click({640, 418});
    for (int i = 0; i < 180; ++i)
        Step();
    Capture("garden");
    float elapsed = model.elapsed;
    Click({1090, 49}, true);
    for (int i = 0; i < 20; ++i)
        Step();
    check(screen == Campaign::Screen::Pause && model.elapsed - elapsed < 0.08f,
          "pause freezes waves");
    Capture("pause");
    Click({640, 418});
    InputFrame focus;
    focus.focused = false;
    Step(focus);
    check(screen == Campaign::Screen::Pause, "focus loss pauses");
    Step();
    Click({640, 418});
    for (int stage = 0; stage < 3; ++stage) {
        Begin(stage);
        Step();
        // Render every plant/enemy type in their real world objects.
        for (int i = 0; i < 6; ++i)
            model.plants.push_back({i % 5, i, static_cast<Kind>(i), 110, 9});
        for (int i = 0; i < 5; ++i)
            model.SpawnZombie(i, i % 3, 1030);
        for (int i = 0; i < 6; ++i)
            Step();
        Capture(stage == 0 ? "stage1" : stage == 1 ? "stage2" : "stage3");
        chinese = false;
        ApplySettings();
        Step();
        Capture(TextFormat("stage%d-en", stage + 1));
        chinese = true;
        ApplySettings();
        SetTargetFPS(0);
        model.zombies.clear();
        model.spawned = static_cast<int>(model.schedule.size());
        Step();
        check(screen == Campaign::Screen::Won && unlocked >= std::min(3, stage + 2),
              "victory unlock/save");
        Capture(stage == 2 ? "campaign-complete" : "victory");
        Click({640, 418});
    }
    model.mowerReady[0] = false;
    model.SpawnZombie(0, 0, Left - 50);
    Step();
    check(screen == Campaign::Screen::Lost, "second breach loses");
    Capture("defeat");
    Click({530, 482});
    check(screen == Campaign::Screen::Play && model.outcome == 0 && model.mowerReady[0],
          "retry resets lane defence");
    for (auto size : {Vector2{1024, 768}, {1600, 720}, {720, 1000}}) {
        SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
        Step();
        Step();
        check(HudVisible(), "adaptive layout retains the full top HUD");
        Click({1090, 49}, true);
        check(screen == Campaign::Screen::Pause, "adaptive touch navigation");
        Capture(TextFormat("layout-%dx%d", static_cast<int>(size.x), static_cast<int>(size.y)));
        Click({640, 418}, true);
        SetScreen(Campaign::Screen::Menu);
        Step();
        Capture(TextFormat("menu-%dx%d", static_cast<int>(size.x), static_cast<int>(size.y)));
        check(MenuPanelVisible(), "menu renders at its adaptive logical position");
        Click({640, 376}, true);
        check(screen == Campaign::Screen::Play, "responsive menu touch mapping");
    }
    SaveSystem saved;
    check(saved.Load(data / "campaign.save") && saved.GetInt("unlocked") == 3,
          "persistent campaign");
    muted = true;
    Save();
    Shutdown();
    Game restored(data);
    restored.Init();
    restored.Step();
    check(restored.unlocked == 3 && restored.muted,
          "progress and sound restored in new application");
    check(restored.chinese && restored.smooth && restored.fps == 120,
          "language, quality and frame settings restored");
    restored.Shutdown();
    log << failures << " failures\n";
    return failures ? 1 : 0;
}
} // namespace Garden
