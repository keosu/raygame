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
    Capture("menu");
    Click({640, 508}, true);
    check(screen == Campaign::Screen::Help, "instructions via touch");
    Capture("help");
    Click({640, 624});
    Click({640, 376}, true);
    check(screen == Campaign::Screen::Play, "start adventure");
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
    model.suns.push_back({100, {985, 420}});
    int sun = model.sun;
    Click({985, 420}, true);
    check(model.sun == sun + 25, "real sun token collection");
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
        Click({1090, 49}, true);
        check(screen == Campaign::Screen::Pause, "adaptive touch navigation");
        Capture(TextFormat("layout-%dx%d", static_cast<int>(size.x), static_cast<int>(size.y)));
        Click({640, 418}, true);
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
    restored.Shutdown();
    log << failures << " failures\n";
    return failures ? 1 : 0;
}
} // namespace Garden
