#pragma once
namespace Goo {
inline int Game::Smoke() {
    int failures = 0;
    std::ofstream log(data / "smoke.log");
    auto check = [&](bool ok, const char* label) {
        log << (ok ? "PASS " : "FAIL ") << label << '\n';
        log.flush();
        if (!ok)
            ++failures;
    };
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    muted = true;
    unlocked = 1;
    Save();
    Init();
    SetTargetFPS(0);
    Step();
    Step();
    CheckStartup(check);
    CheckSettings(check);
    ClickButton(3, true);
    check(screen == Campaign::Screen::Help, "touch opens instructions");
    Capture("help");
    Click({640, 624});
    ClickButton(1, true);
    check(screen == Campaign::Screen::Play, "start adventure through menu");
    CheckPauseSettings(check);
    auto dragGesture = [&](Vector2 from, Vector2 to, bool touch, bool cancel) {
        auto map = [&](Vector2 p) {
            return Display::CanvasToScreen(Vector2Add(p, Campaign::Origin()));
        };
        InputFrame frame;
        frame.mousePosition = map(from);
        if (touch) {
            frame.touchCount = 1;
            frame.touches[0] = {17, map(from)};
        } else
            frame.mouse.held[0] = frame.mouse.pressed[0] = true;
        Step(frame);
        frame.mouse.pressed[0] = false;
        frame.mousePosition = map(to);
        if (touch)
            frame.touches[0].position = map(to);
        Step(frame);
        if (cancel) {
            frame.focused = false;
            Step(frame);
            Step();
        } else {
            frame = {};
            frame.mousePosition = map(to);
            if (!touch)
                frame.mouse.released[0] = true;
            Step(frame);
            Step();
        }
    };
    int budget = M().reserve;
    dragGesture({130, 670}, {400, 490}, true, false);
    check(M().reserve == budget - 1 && M().nodes.size() == 6,
          "captured touch drags a new ball from BUILD");
    auto position = M().Pos(5);
    dragGesture(position, {position.x + 15, position.y - 15}, false, false);
    check(M().dragged == -1 && M().world.JointCount() == M().links.size(),
          "mouse drag releases its temporary constraint");
    budget = M().reserve;
    dragGesture({130, 670}, {470, 550}, true, true);
    check(screen == Campaign::Screen::Pause && M().reserve == budget && !placing,
          "focus loss cancels placement and pauses");
    ClickButton(6);
    Click({345, 670});
    Click(M().Pos(5));
    check(!M().nodes[5].alive && M().reserve == budget + 1,
          "recycle returns the ball and removes its joints");
    Click({130, 670});
    for (int stage = 0; stage < 3; ++stage) {
        Begin(stage);
        Step();
        std::vector<Vector2> solution;
        if (stage == 0)
            for (int i = 0; i < 11; ++i)
                solution.push_back({400.0f + i * 60, i % 2 == 0 ? 490.0f : 560.0f});
        if (stage == 1)
            solution = {{550, 460}, {690, 450}, {615, 380}, {685, 320}, {600, 260}};
        if (stage == 2)
            solution = {{380, 480}, {400, 580}, {470, 480}, {450, 395}, {535, 355}, {530, 440},
                        {610, 365}, {680, 420}, {740, 345}, {810, 420}, {870, 370}, {950, 420}};
        for (auto p : solution) {
            int before = M().reserve;
            Click(p, true);
            check(M().reserve == before - 1, "level solution uses real touch placements");
            for (int i = 0; i < 12; ++i)
                Tick(1.0f / 60);
        }
        Step();
        Capture(TextFormat("stage%d", stage + 1));
        Click({1110, 665}, true);
        check(M().sending, "connected route enables the launch button");
        for (int i = 0; i < 5000 && screen == Campaign::Screen::Play; ++i) {
            Input::SetFrame({});
            Tick(1.0f / 60);
            if (i == 120) {
                Render();
                Capture(TextFormat("travel%d", stage + 1));
            }
        }
        Step();
        check(screen == Campaign::Screen::Won && unlocked >= std::min(3, stage + 2),
              "real simulation completes and unlocks each level");
        Capture(stage == 2 ? "complete" : "victory");
    }
    Begin(0);
    Step();
    for (int i = 0; i < 14410 && screen == Campaign::Screen::Play; ++i) {
        Input::SetFrame({});
        Tick(1.0f / 60);
    }
    Step();
    check(screen == Campaign::Screen::Lost, "dusk deadline produces a loss");
    Capture("defeat");
    ClickButton(7);
    check(screen == Campaign::Screen::Play && M().elapsed < 0.1f,
          "retry starts a fresh simulation");
    for (auto size : {Vector2{1024, 768}, {1600, 720}, {720, 1000}}) {
        SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
        Step();
        Step();
        check(HudVisible(), "adaptive canvas keeps HUD visible");
        Capture(TextFormat("layout-%dx%d", static_cast<int>(size.x), static_cast<int>(size.y)));
        Click({1090, 49}, true);
        check(screen == Campaign::Screen::Pause, "adaptive touch pause mapping");
        ClickButton(6, true);
    }
    Save();
    Shutdown();
    Game restored(data);
    restored.Init();
    restored.Step();
    check(restored.unlocked == 3 && restored.muted && restored.best[0] > 0 && restored.best[2] > 0,
          "new application restores progress, scores and settings");
    restored.Shutdown();
    log << failures << " failures\n";
    return failures ? 1 : 0;
}
} // namespace Goo
