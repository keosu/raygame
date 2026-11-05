#include "ExploreScene.h"
#include "Assets.h"
#include "Characters.h"
#include "Draw.h"
#include "Game.h"
#include "Gui.h"

namespace UQ {

namespace {
constexpr float kStreetW = 3400.0f;
constexpr float kGroundY = 600.0f;
constexpr float kWalkSpeed = 260.0f;
constexpr float kGateX = 2700.0f;
constexpr float kRevealX = 2900.0f;
constexpr float kKaijuX = 3200.0f; // where Red King breaks out, behind the site
const char* kClues[] = {"温热的裂缝", "汤碗的波纹", "地底的吼声"};

struct House {
    float x, w, h;
    Color wall, roof;
    int type; // 0 house, 1 apartment, 2 grocery, 3 ramen shop, 4 park
};
const House kHouses[] = {
    {470, 330, 250, {226, 214, 190, 255}, {120, 70, 60, 255}, 2},
    {840, 200, 300, {200, 210, 222, 255}, {70, 80, 110, 255}, 1},
    {1080, 220, 230, {236, 226, 206, 255}, {90, 110, 140, 255}, 0},
    {1340, 300, 260, {150, 40, 40, 255}, {50, 40, 40, 255}, 3},
    {1700, 200, 280, {210, 220, 200, 255}, {80, 100, 80, 255}, 1},
    {1900, 420, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 4},
    {2360, 240, 240, {232, 220, 200, 255}, {130, 80, 60, 255}, 0},
    {2600, 180, 320, {196, 204, 220, 255}, {70, 80, 110, 255}, 1},
};
} // namespace

void ExploreScene::OnLoad() {
    Sfx::PlayMusic("explore");
    controls.Reset();
    spots = {
        {"truck", 300, "", Mark::Look},
        {"granny", 640, "granny", Mark::Talk, -1, false, -1},
        {"crack", 980, "", Mark::Clue, 0},
        {"owner", 1500, "owner", Mark::Clue, 1, false, -1},
        {"kid", 2090, "kid", Mark::Clue, 2, false, -1},
        {"worker", 2780, "worker", Mark::Talk, -1, false, -1},
    };
    Talk("intro");
}

int ExploreScene::ClueCount() const { return (int)std::count(clues.begin(), clues.end(), true); }

ExploreScene::Spot* ExploreScene::Nearby() {
    Spot* best = nullptr;
    float bestDist = 80;
    for (auto& s : spots) {
        float d = std::fabs(s.x - player);
        if (d < bestDist && !(s.id == "worker" && siteOpen)) {
            best = &s;
            bestDist = d;
        }
    }
    return best;
}

void ExploreScene::Talk(const std::string& block) { dialogue.Start(block); }

void ExploreScene::Interact(Spot& s) {
    facing = s.x > player ? 1 : -1;
    if (!s.look.empty()) s.facing = -facing;
    if (s.id == "worker") {
        Talk(ClueCount() >= 3 ? "site" : "worker_block");
        return;
    }
    if (s.clue >= 0 && !clues[s.clue]) pendingIndex = s.clue;
    s.done = true;
    Talk(s.id);
}

void ExploreScene::DebugInteract(const std::string& id) {
    for (auto& s : spots)
        if (s.id == id) {
            player = s.x - 60;
            Interact(s);
        }
}

void ExploreScene::DebugFindAll() {
    clues = {true, true, true};
    for (auto& s : spots) s.done = true;
    cluesAnnounced = true;
    siteOpen = true;
}

void ExploreScene::DebugReveal() {
    DebugFindAll();
    dialogue.Skip();
    player = kRevealX;
    phase = Phase::Reveal;
    phaseTime = 0;
    Sfx::Play("quake", 1.0f, 0);
}

void ExploreScene::Update() {
    float dt = std::min(GameTime::DeltaTime(), 0.05f);
    Gui::BeginFrame();
    controls.Update(Controls::Layout::Explore);
    if (controls.Pressed(Action::Pause) && phase == Phase::Walk) {
        paused = !paused;
        Sfx::Play("click", 0.5f);
    }
    if (paused) return;
    time += dt;
    phaseTime += dt;
    toastTime += dt;
    tremor = std::max(0.0f, tremor - dt);
    shake = Approach(shake, 0, dt * 12);
    for (auto& s : spots) s.react = std::max(0.0f, s.react - dt);

    if (dialogue.Active()) {
        if (dialogue.Update(dt, controls.Confirm())) {
            std::string done = dialogue.Current();
            if (pendingIndex >= 0) {
                clues[pendingIndex] = true;
                toast = std::string("获得线索：") + kClues[pendingIndex];
                toastTime = 0;
                Sfx::Play("clue", 0.8f, 0);
                pendingIndex = -1;
            }
            if (done == "site") siteOpen = true;
            if (done == "reveal") {
                phase = Phase::Rise;
                phaseTime = 0;
                Sfx::Play("charge", 0.8f, 0);
            }
            if (ClueCount() == 3 && !cluesAnnounced) {
                cluesAnnounced = true;
                Talk("clues_done");
            }
        }
    } else if (phase == Phase::Walk) {
        float move = controls.MoveX();
        if (std::fabs(move) > 0.1f) {
            facing = move > 0 ? 1 : -1;
            walk += dt * 11;
        } else {
            walk = 0;
        }
        player += move * kWalkSpeed * dt;
        float limit = siteOpen ? kRevealX + 40 : kGateX;
        player = Clamp(player, 80.0f, limit);
        if (!siteOpen && player >= kGateX - 2 && !gateWarned) {
            gateWarned = true;
            for (auto& s : spots)
                if (s.id == "worker") Interact(s);
        }
        if (player < kGateX - 160) gateWarned = false;
        if (controls.Pressed(Action::Attack))
            if (Spot* s = Nearby()) Interact(*s);
        if (siteOpen && player >= kRevealX) {
            phase = Phase::Reveal;
            phaseTime = 0;
            Sfx::Play("quake", 1.0f, 0);
        }
        if (time >= tremorAt) {
            tremorAt = time + Rand(12, 17);
            tremor = 1.2f;
            shake = 6;
            Sfx::Play("quake", 0.45f, 0);
            for (auto& s : spots) s.react = 1;
            for (int i = 0; i < 6; ++i) fx.Dust({camX + Rand(100, 1180), Rand(320, 420)}, 2, 30, {200, 190, 170, 160});
        }
    } else if (phase == Phase::Reveal) {
        shake = std::max(shake, 10.0f * (1 - std::min(1.0f, phaseTime / 3.2f)) + 2);
        if (phaseTime - dt < 1.6f && phaseTime >= 1.6f) Sfx::Play("roar", 1.0f, 0);
        if (std::fmod(phaseTime, 0.25f) < dt && phaseTime < 2.6f)
            fx.Debris({kKaijuX + Rand(-160, 160), kGroundY - Rand(0, 40)}, 3, {130, 110, 90, 255});
        if (phaseTime >= 2.8f && !dialogue.Active()) Talk("reveal");
    } else if (phase == Phase::Rise) {
        if (phaseTime >= 1.5f && phase != Phase::Finish) {
            phase = Phase::Finish;
            Progress::SetSkipIntro();
            GoToTransform();
        }
    }
    float goal = Clamp(player - 640 + facing * 90, 0.0f, kStreetW - kCanvasW);
    if (phase == Phase::Reveal || phase == Phase::Rise) goal = kStreetW - kCanvasW;
    camX += (goal - camX) * (1 - std::exp(-4 * dt));
    fx.Update(dt, false);
}

// ---------------------------------------------------------------- drawing

void ExploreScene::DrawSky() const {
    bool dark = phase != Phase::Walk;
    Draw::VerticalGradientRect({0, 0, (float)kCanvasW, kGroundY}, dark ? Color{70, 60, 90, 255} : Color{110, 170, 236, 255},
                               dark ? Color{210, 150, 120, 255} : Color{226, 238, 250, 255});
    Draw::BlendAdd();
    Draw::Glow({1060, 110}, 180, WithAlpha({255, 250, 220, 255}, 0.55f));
    Draw::BlendNormal();
    DrawCircleV({1060, 110}, 40, {255, 252, 236, 255});
    Rng r(5);
    for (int i = 0; i < 7; ++i) {
        float x = std::fmod(r.Range(0, 2400) - camX * 0.1f + time * 6 + 2400 * 3, 2400.0f) - 300;
        float y = r.Range(60, 220);
        Draw::Ellipse({x, y}, r.Range(70, 140), r.Range(16, 26), WithAlpha(WHITE, 0.75f));
        Draw::Ellipse({x + 50, y - 12}, r.Range(40, 80), r.Range(14, 22), WithAlpha(WHITE, 0.75f));
    }
    std::vector<Vector2> hills;
    for (int i = 0; i <= 30; ++i) hills.push_back({i * 110.0f - camX * 0.2f - 200, 380 - 60 - 40 * std::sin(i * 0.8f) - 20 * std::sin(i * 2.1f)});
    for (size_t i = 0; i + 1 < hills.size(); ++i)
        Draw::Quad(hills[i], hills[i + 1], {hills[i + 1].x, 470}, {hills[i].x, 470}, {150, 176, 206, 255});
    Rng s(8);
    for (float x = -200; x < kStreetW; x += s.Range(50, 110)) {
        float sx = x - camX * 0.4f;
        float w = s.Range(40, 90), h = s.Range(60, 170);
        if (sx + w < 0 || sx > kCanvasW) continue;
        DrawRectangleRec({sx, 470 - h, w, h + 10}, {170, 190, 214, 255});
    }
}

void ExploreScene::DrawStreet() const {
    float shakeX = shake > 0 ? Rand(-shake, shake) : 0;
    // Ground: sidewalk and road.
    DrawRectangleRec({0, 470, (float)kCanvasW, kGroundY - 470}, {196, 204, 190, 255});
    DrawRectangleRec({0, kGroundY, (float)kCanvasW, 40}, {176, 176, 182, 255});
    DrawRectangleRec({0, kGroundY, (float)kCanvasW, 4}, {140, 140, 150, 255});
    DrawRectangleRec({0, kGroundY + 40, (float)kCanvasW, kCanvasH - kGroundY - 40}, {72, 74, 84, 255});
    for (float x = -std::fmod(camX, 160.0f); x < kCanvasW; x += 160) DrawRectangleRec({x, 680, 80, 6}, {230, 230, 220, 255});
    // Buildings.
    for (const auto& b : kHouses) {
        float x = ScreenX(b.x) + shakeX * 0.5f;
        if (x + b.w < -50 || x > kCanvasW + 50) continue;
        if (b.type == 4) { // park
            DrawRectangleRec({x, kGroundY - 26, b.w, 26}, {120, 180, 100, 255});
            for (int i = 0; i < 3; ++i) {
                float tx = x + 60 + i * 150;
                DrawRectangleRec({tx - 8, kGroundY - 150, 16, 130}, {110, 80, 60, 255});
                DrawCircleV({tx, kGroundY - 180}, 62, {90, 160, 90, 255});
                DrawCircleV({tx - 30, kGroundY - 150}, 44, {100, 176, 96, 255});
                DrawCircleV({tx + 34, kGroundY - 156}, 46, {84, 150, 84, 255});
            }
            DrawRectangleRec({x + 180, kGroundY - 44, 90, 10}, {150, 100, 60, 255});
            DrawRectangleRec({x + 186, kGroundY - 34, 6, 34}, {90, 70, 50, 255});
            DrawRectangleRec({x + 258, kGroundY - 34, 6, 34}, {90, 70, 50, 255});
            continue;
        }
        float top = kGroundY - b.h;
        DrawRectangleRec({x, top, b.w, b.h}, b.wall);
        DrawRectangleRec({x, top, b.w, 6}, Darken(b.wall, 0.2f));
        if (b.type == 0) {
            Draw::Tri({x - 16, top + 4}, {x + b.w + 16, top + 4}, {x + b.w / 2, top - 70}, b.roof);
            DrawRectangleRec({x + b.w * 0.15f, top + 50, 54, 46}, {120, 170, 210, 255});
            DrawRectangleRec({x + b.w * 0.6f, top + 50, 54, 46}, {120, 170, 210, 255});
            DrawRectangleRec({x + b.w * 0.42f, kGroundY - 90, 44, 90}, Darken(b.roof, 0.1f));
        } else if (b.type == 1) {
            for (int y = 0; y < (int)(b.h / 64); ++y)
                for (int c = 0; c < (int)(b.w / 60); ++c)
                    DrawRectangleRec({x + 16 + c * 60, top + 22 + y * 64, 34, 38}, (y + c) % 3 ? Color{130, 176, 214, 255} : Color{236, 220, 170, 255});
            DrawRectangleRec({x - 6, top - 10, b.w + 12, 12}, b.roof);
        } else if (b.type == 2) { // grocery
            DrawRectangleRec({x - 8, top - 8, b.w + 16, 14}, b.roof);
            for (int i = 0; i < 6; ++i)
                DrawRectangleRec({x + i * b.w / 6, kGroundY - 150, b.w / 6, 30}, i % 2 ? Color{236, 236, 226, 255} : Color{40, 140, 90, 255});
            DrawRectangleRec({x + 20, kGroundY - 118, b.w - 40, 118}, {90, 70, 56, 255});
            for (int i = 0; i < 4; ++i) {
                DrawRectangleRec({x + 34 + i * 70, kGroundY - 100, 56, 46}, {244, 220, 160, 255});
                DrawCircleV({x + 50 + i * 70, kGroundY - 68}, 10, {230, 100, 60, 255});
                DrawCircleV({x + 72 + i * 70, kGroundY - 70}, 9, {240, 200, 60, 255});
            }
            DrawRectangleRec({x + b.w / 2 - 70, top + 30, 140, 48}, {250, 246, 230, 255});
            Gui::Text("杂货店", x + b.w / 2, top + 38, 28, {60, 50, 40, 255}, Gui::Align::Center, true);
        } else if (b.type == 3) { // ramen shop
            DrawRectangleRec({x - 10, top - 10, b.w + 20, 16}, b.roof);
            DrawRectangleRec({x + 20, top + 26, b.w - 40, 60}, {40, 30, 30, 255});
            Gui::Text("拉面", x + b.w / 2, top + 32, 40, {255, 220, 150, 255}, Gui::Align::Center, true);
            for (int i = 0; i < 4; ++i) {
                float nx = x + 40 + i * (b.w - 80) / 4.0f;
                DrawRectangleRec({nx + 2, kGroundY - 170, (b.w - 80) / 4.0f - 4, 70}, {30, 40, 90, 255});
            }
            Gui::Text("营业中", x + b.w / 2, kGroundY - 154, 20, WHITE, Gui::Align::Center, true);
            DrawRectangleRec({x + 40, kGroundY - 100, b.w - 80, 100}, {70, 40, 30, 255});
            for (float lx : {x + 18, x + b.w - 18}) {
                Draw::Ellipse({lx, kGroundY - 170}, 16, 24, {230, 60, 40, 255});
                Draw::BlendAdd();
                Draw::Glow({lx, kGroundY - 170}, 40, WithAlpha({255, 150, 90, 255}, 0.4f));
                Draw::BlendNormal();
            }
        }
    }
    // Power poles and wires.
    for (float px = 200; px < kStreetW; px += 520) {
        float x = ScreenX(px);
        if (x < -100 || x > kCanvasW + 100) continue;
        DrawRectangleRec({x - 6, kGroundY - 330, 12, 330}, {120, 110, 100, 255});
        DrawRectangleRec({x - 40, kGroundY - 310, 80, 8}, {100, 90, 84, 255});
        float nx = ScreenX(px + 520);
        Draw::Line({x - 36, kGroundY - 306}, {nx - 36, kGroundY - 306 + 8}, 2, WithAlpha(BLACK, 0.5f));
        Draw::Line({x + 36, kGroundY - 306}, {nx + 36, kGroundY - 306 + 8}, 2, WithAlpha(BLACK, 0.5f));
    }
    // The STORAGE truck.
    {
        float x = ScreenX(300);
        DrawRectangleRounded({x - 130, kGroundY - 120, 260, 96}, 0.2f, 6, {40, 52, 96, 255});
        DrawRectangleRec({x - 130, kGroundY - 70, 260, 12}, {255, 140, 40, 255});
        DrawRectangleRounded({x + 60, kGroundY - 112, 60, 40}, 0.2f, 4, {150, 200, 236, 255});
        Gui::Text("STORAGE", x - 40, kGroundY - 106, 22, WHITE, Gui::Align::Center, true);
        DrawRectangleRec({x - 60, kGroundY - 140, 6, 22}, {200, 200, 210, 255});
        Draw::Ellipse({x - 57, kGroundY - 144}, 22, 8, {210, 214, 224, 255});
        for (float wx : {x - 80, x + 80}) {
            DrawCircleV({wx, kGroundY - 22}, 24, {30, 30, 34, 255});
            DrawCircleV({wx, kGroundY - 22}, 10, {170, 170, 180, 255});
        }
    }
    // The crack with steam.
    {
        float x = ScreenX(980);
        std::vector<Vector2> crack = {{x - 90, kGroundY + 30}, {x - 50, kGroundY + 14}, {x - 20, kGroundY + 30},
                                      {x + 20, kGroundY + 10}, {x + 60, kGroundY + 26}, {x + 100, kGroundY + 16}};
        Draw::PolyLine(crack, 8, {40, 36, 40, 255}, false);
        Draw::PolyLine(crack, 3, {220, 120, 70, 200}, false);
        for (int i = 0; i < 5; ++i) {
            float k = std::fmod(time * 0.5f + i * 0.2f, 1.0f);
            DrawCircleV({x - 40 + i * 22 + std::sin(time * 2 + i) * 10, kGroundY + 10 - k * 120}, 14 + k * 24,
                        WithAlpha(WHITE, 0.45f * (1 - k)));
        }
    }
}

void ExploreScene::DrawSite() const {
    float x0 = ScreenX(2720);
    if (x0 > kCanvasW + 50) return;
    // Red King breaks out behind the site during the reveal.
    if (phase != Phase::Walk) {
        const Pose* pose = Poses::Get("rk_roar");
        float rise = EaseOut(std::min(1.0f, phaseTime / 2.6f));
        if (phase == Phase::Rise || phase == Phase::Finish) rise = 1;
        if (pose && pose->texture) {
            // The render already looks left, towards Haruki.
            const Texture2D& t = *pose->texture;
            float h = 480, w = t.width * h / t.height;
            float cx = ScreenX(kKaijuX);
            float bottom = kGroundY - 30 + (1 - rise) * h;
            Display::ClipScope clip({0, 0, (float)kCanvasW, kGroundY - 20});
            DrawTexturePro(t, {0, 0, (float)t.width, (float)t.height}, {cx - w / 2, bottom - h, w, h}, {0, 0}, 0,
                           Mix(WHITE, {150, 130, 140, 255}, 0.3f));
        }
        for (int i = 0; i < 5; ++i)
            Draw::Ellipse({ScreenX(kKaijuX) + (i - 2) * 80.0f, kGroundY - 30}, 80, 36, WithAlpha({170, 150, 120, 255}, 0.7f));
    }
    // Fence, cones, dirt and a crane.
    DrawRectangleRec({ScreenX(2850), kGroundY - 30, 600, 30}, {170, 140, 100, 255});
    for (int i = 0; i < 4; ++i) Draw::Ellipse({ScreenX(2950 + i * 120.0f), kGroundY - 30}, 70, 26, {150, 120, 86, 255});
    float cx = ScreenX(3320);
    DrawRectangleRec({cx - 14, kGroundY - 430, 28, 400}, {240, 190, 40, 255});
    for (int i = 0; i < 12; ++i) Draw::Line({cx - 14, kGroundY - 430 + i * 34}, {cx + 14, kGroundY - 396 + i * 34}, 3, {180, 140, 30, 255});
    DrawRectangleRec({cx - 260, kGroundY - 446, 320, 18}, {240, 190, 40, 255});
    Draw::Line({cx - 220, kGroundY - 428}, {cx - 220, kGroundY - 300}, 2, {60, 60, 60, 255});
    DrawRectangleRec({cx - 236, kGroundY - 300, 32, 24}, {90, 90, 100, 255});
    if (!siteOpen || phase == Phase::Walk) {
        for (int i = 0; i < 5; ++i) {
            float bx = x0 + i * 26;
            DrawRectangleRec({bx, kGroundY - 70, 22, 70}, i % 2 ? Color{30, 30, 30, 255} : Color{250, 210, 40, 255});
        }
        DrawRectangleRec({x0 - 6, kGroundY - 80, 140, 14}, {250, 210, 40, 255});
        Draw::RoundedPanel({x0 + 10, kGroundY - 140, 110, 44}, 6, {250, 250, 240, 255}, {200, 40, 40, 255}, 3);
        Gui::Text("施工中", x0 + 65, kGroundY - 132, 24, {200, 40, 40, 255}, Gui::Align::Center, true);
    }
    for (float cone : {2860.0f, 2940.0f}) {
        float x = ScreenX(cone);
        Draw::Tri({x - 14, kGroundY}, {x + 14, kGroundY}, {x, kGroundY - 40}, {255, 120, 30, 255});
        DrawRectangleRec({x - 8, kGroundY - 24, 16, 6}, WHITE);
    }
}

void ExploreScene::DrawSpots() const {
    const Spot* near = const_cast<ExploreScene*>(this)->Nearby();
    for (const auto& s : spots) {
        float x = ScreenX(s.x);
        if (x < -80 || x > kCanvasW + 80) continue;
        float headY = kGroundY - 120;
        if (!s.look.empty()) {
            if (s.id == "worker" && siteOpen) continue;
            const Look& look = LookFor(s.look);
            DrawPerson(look, {x, kGroundY}, s.facing, 0, s.react);
            headY = kGroundY - look.height - 30;
            if (s.id == "kid") { // the Red King toy
                DrawCircleV({x + s.facing * 18, kGroundY - 40}, 9, {200, 170, 90, 255});
                DrawRectangleRec({x + s.facing * 18 - 6, kGroundY - 36, 12, 16}, {200, 170, 90, 255});
            }
        }
        if (phase != Phase::Walk || dialogue.Active()) continue;
        bool open = s.clue >= 0 ? !clues[s.clue] : !s.done;
        if (s.id == "worker" && ClueCount() >= 3) open = true;
        if (!open && &s != near) continue;
        float bob = std::sin(time * 4 + s.x) * 5;
        Vector2 c = {x, headY + bob};
        Color color = s.mark == Mark::Clue ? Palette::gold : s.mark == Mark::Look ? Palette::cyan : Lighten(Palette::muted, 0.3f);
        if (!open) color = Palette::muted;
        DrawCircleV(c, 20, WithAlpha(Palette::ink, 0.8f));
        DrawRing(c, 17, 20, 0, 360, 32, color);
        const char* glyph = s.mark == Mark::Clue ? "！" : s.mark == Mark::Look ? "？" : "…";
        Gui::Text(glyph, c.x, c.y - 14, 26, color, Gui::Align::Center, true);
        if (&s == near) {
            const char* label = controls.TouchVisible() ? "调查" : "[E] 调查";
            float w = Gui::Measure(label, 18, true) + 26;
            Draw::RoundedPanel({c.x - w / 2, c.y - 62, w, 32}, 10, WithAlpha(Palette::gold, 0.95f), BLANK, 0);
            Gui::Text(label, c.x, c.y - 56, 18, Palette::ink, Gui::Align::Center, true);
        }
    }
}

void ExploreScene::DrawHud() const {
    Draw::RoundedPanel({16, 16, 420, 116}, 14, WithAlpha(Palette::ink, 0.72f), WithAlpha(Palette::cyan, 0.4f), 1.5f);
    Gui::Text("目标", 34, 26, 16, Palette::gold, Gui::Align::Left, true);
    const char* goal = ClueCount() < 3 ? TextFormat("调查连续地震的原因（%d/3）", ClueCount())
                       : !siteOpen     ? "前往北边的工地"
                                       : "进入工地深处";
    Gui::Text(goal, 34, 48, 20, Palette::text, Gui::Align::Left, true);
    for (int i = 0; i < 3; ++i) {
        Rectangle r = {34 + i * 132.0f, 84, 124, 32};
        Draw::RoundedPanel(r, 8, clues[i] ? WithAlpha(Palette::gold, 0.25f) : WithAlpha(Palette::panelLight, 0.6f),
                           clues[i] ? Palette::gold : WithAlpha(Palette::line, 0.8f), 1.5f);
        Gui::Text(clues[i] ? kClues[i] : "？？？", r.x + r.width / 2, r.y + 6, 16, clues[i] ? Palette::gold : Palette::muted,
                  Gui::Align::Center, true);
    }
    if (toastTime < 2.6f) {
        float a = std::min(1.0f, std::min(toastTime / 0.2f, (2.6f - toastTime) / 0.4f));
        float w = Gui::Measure(toast, 26, true) + 60;
        Draw::RoundedPanel({640 - w / 2, 150, w, 54}, 14, WithAlpha(Palette::ink, 0.85f * a), WithAlpha(Palette::gold, a), 2);
        Gui::Text(toast, 640, 162, 26, MulAlpha(Palette::gold, a), Gui::Align::Center, true);
    }
}

void ExploreScene::DrawPause() {
    DrawRectangleRec({0, 0, (float)kCanvasW, (float)kCanvasH}, WithAlpha(Palette::ink, 0.7f));
    Rectangle card = {460, 230, 360, 250};
    Gui::Panel(card, Palette::panel, 16);
    Gui::Text("暂停", 640, card.y + 22, 34, Palette::text, Gui::Align::Center, true);
    if (Gui::Button({card.x + 50, card.y + 90, 260, 50}, "继续调查", Gui::Style::Gold) || controls.Pressed(Action::Pause))
        paused = false;
    if (Gui::Button({card.x + 50, card.y + 156, 260, 50}, "返回标题", Gui::Style::Red)) GoToTitle();
}

void ExploreScene::Render() {
    DrawSky();
    DrawSite();
    DrawStreet();
    DrawSpots();
    float shakeX = shake > 0 ? Rand(-shake, shake) * 0.5f : 0;
    float react = tremor > 0 ? tremor : 0;
    float raise = phase == Phase::Rise ? EaseOut(phaseTime / 0.6f) : 0;
    DrawPerson(LookFor("haruki"), {ScreenX(player) + shakeX, kGroundY}, facing, walk, react, raise);
    if (phase == Phase::Rise && phaseTime > 0.8f) {
        float k = (phaseTime - 0.8f) / 0.7f;
        Draw::BlendAdd();
        Draw::Glow({ScreenX(player), kGroundY - 140}, 200 + 1600 * k, WithAlpha({170, 230, 255, 255}, std::min(1.0f, k * 1.4f)));
        Draw::BlendNormal();
        DrawRectangleRec({0, 0, (float)kCanvasW, (float)kCanvasH}, WithAlpha(WHITE, Clamp(k * 1.2f - 0.3f, 0.0f, 1.0f)));
    }
    fx.Draw([this](Vector2 p) { return Vector2{p.x - camX, p.y}; }, 1);
    if (phase == Phase::Walk) DrawHud();
    dialogue.Draw();
    if (phase == Phase::Walk && !dialogue.Active())
        controls.Draw(Controls::Layout::Explore, false, const_cast<ExploreScene*>(this)->Nearby() != nullptr);
    if (paused) DrawPause();
}

} // namespace UQ
