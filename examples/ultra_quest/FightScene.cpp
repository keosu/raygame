#include "FightScene.h"
#include "Assets.h"
#include "Draw.h"
#include "Game.h"
#include "Gui.h"

namespace UQ {

namespace {
constexpr float kGround = 640.0f;          // canvas y of the ground line
constexpr float kHeroWpp = 400.0f / 800.0f;  // world units per pose pixel (tools/make_poses.py heights)
constexpr float kKaijuWpp = 440.0f / 860.0f;
const char* kRankNames[] = {"C", "B", "A", "S", "SS"};
const Color kRankColors[] = {{170, 180, 200, 255}, {120, 210, 255, 255}, {120, 240, 150, 255}, {255, 206, 84, 255},
                             {255, 120, 200, 255}};

const char* MoveName(KaijuMove m) {
    switch (m) {
    case KaijuMove::Swing: return "重击";
    case KaijuMove::Charge: return "冲撞";
    case KaijuMove::Boulder: return "投石";
    case KaijuMove::Stomp: return "跺地";
    default: return "";
    }
}
const char* MoveHint(KaijuMove m) {
    switch (m) {
    case KaijuMove::Swing: return "防御 / 后撤";
    case KaijuMove::Charge: return "冲刺穿过 / 跳跃";
    case KaijuMove::Boulder: return "击碎岩石 / 躲开";
    case KaijuMove::Stomp: return "跳起躲避冲击波";
    default: return "";
    }
}
Color MoveColor(KaijuMove m) {
    switch (m) {
    case KaijuMove::Swing: return {255, 170, 60, 255};
    case KaijuMove::Charge: return {255, 70, 70, 255};
    case KaijuMove::Boulder: return {210, 160, 110, 255};
    case KaijuMove::Stomp: return {255, 220, 80, 255};
    default: return WHITE;
    }
}

float Lerpf(float a, float b, float t) { return a + (b - a) * t; }
} // namespace

FightScene::FightScene(bool story) : story(story), fight(story ? 1 : (std::uint32_t)GetRandomValue(1, 1 << 30)) {}

void FightScene::OnLoad() {
    Sfx::PlayMusic("battle");
    controls.Reset();
    // Low buildings along the arena at fighter depth: giants tower over them and knock them down.
    float x = -60;
    while (x < kArenaW + 60) {
        Building b;
        b.w = rng.Range(90, 190);
        b.h = rng.Range(70, 210);
        b.style = rng.Int(0, 2);
        static const Color tones[] = {{86, 96, 130, 255}, {110, 96, 104, 255}, {74, 104, 120, 255}, {120, 112, 96, 255}};
        b.color = tones[rng.Int(0, 3)];
        b.x = x + b.w / 2;
        buildings.push_back(b);
        x += b.w + rng.Range(10, 50);
    }
    // Taller towers at both ends take the kaiju's wall crashes.
    for (float edge : {60.0f, kArenaW - 60.0f}) buildings.push_back({edge, 170, 300, 1, {90, 100, 136, 255}});
    camX = (fight.hero().x + fight.kaiju().x) / 2;
    if (!story) phaseTime = 1.2f; // a rematch skips the landing and starts at the kaiju's roar
}

void FightScene::OnUnload() { SetGameSpeed(1); }

void FightScene::SetPhase(Phase p) {
    phase = p;
    phaseTime = 0;
}

void FightScene::DebugSkipIntro() {
    SetPhase(Phase::Fight);
    hintTime = 99;
}

void FightScene::Update() {
    float dt = std::min(GameTime::DeltaTime(), 0.05f);
    sceneTime += dt;
    Gui::BeginFrame();
    controls.Update(phase == Phase::Fight ? Controls::Layout::Fight : Controls::Layout::None);
    if ((phase == Phase::Fight || phase == Phase::Intro) && controls.Pressed(Action::Pause)) {
        paused = !paused;
        showHelp = false;
        Sfx::Play("click", 0.5f);
    }
    if (paused) return;

    phaseTime += dt;
    float modelDt = dt;
    if (slowmo > 0) {
        slowmo -= dt;
        modelDt *= 0.3f;
    }
    switch (phase) {
    case Phase::Intro: {
        float landAt = story ? 0.9f : 0.0f;
        if (story && phaseTime - dt < landAt && phaseTime >= landAt) {
            fx.Flash({fight.hero().x, 200}, 420, {180, 230, 255, 255}, 0.5f);
            fx.Ring({fight.hero().x, 10}, 420, WithAlpha(Palette::cyan, 0.8f), 0.6f);
            fx.Dust({fight.hero().x, 0}, 26, 260);
            shake = 14;
            Sfx::Play("land", 1.0f);
        }
        if (phaseTime - dt < 1.6f && phaseTime >= 1.6f) {
            Sfx::Play("roar", 0.9f, 0.02f);
            shake = std::max(shake, 10.0f);
        }
        if (phaseTime >= 3.0f) SetPhase(Phase::Fight);
        break;
    }
    case Phase::Fight:
        hintTime += dt;
        StepModel(modelDt);
        if (fight.result() != FightResult::Running) {
            SetPhase(Phase::Finish);
            Sfx::StopMusic();
            if (fight.result() == FightResult::Won) slowmo = 1.0f;
        }
        break;
    case Phase::Finish:
        StepModel(modelDt);
        if (fight.result() == FightResult::Won) {
            if (exploded && sceneTime - explodedAt > 0.6f && sceneTime - explodedAt - dt <= 0.6f) Sfx::Play("victory", 0.9f, 0);
            if (exploded && sceneTime - explodedAt > 3.4f) {
                rank = fight.Rank();
                if (story) {
                    SetPhase(Phase::Epilogue);
                    dialogue.Start("victory");
                } else {
                    SetPhase(Phase::Result);
                }
            }
        } else if (phaseTime > 2.8f) {
            SetPhase(Phase::Defeat);
        }
        break;
    case Phase::Epilogue:
        StepModel(dt);
        if (dialogue.Update(dt, controls.Confirm())) SetPhase(Phase::Result);
        break;
    case Phase::Result:
    case Phase::Defeat:
        StepModel(dt);
        break;
    }
    if (phase == Phase::Result && !recorded) {
        recorded = true;
        newBest = Progress::RecordFight(std::max(0, rank), fight.elapsed());
    }

    fx.Update(dt, true);
    for (auto& g : ghosts) g.life -= dt;
    ghosts.erase(std::remove_if(ghosts.begin(), ghosts.end(), [](const Ghost& g) { return g.life <= 0; }), ghosts.end());
    for (auto& b : buildings)
        if (b.down) b.fall = std::min(1.0f, b.fall + dt * 1.6f);
    shake = Approach(shake, 0, dt * 40);
    float amount = Progress::Options().shake ? shake : shake * 0.15f;
    shakeOffset = {Rand(-amount, amount), Rand(-amount, amount) * 0.6f};
    UpdateCamera(dt);

    if (phase == Phase::Fight && fight.timerWarning()) {
        timerBeep -= dt;
        if (timerBeep <= 0) {
            timerBeep = fight.remaining() < 20 ? 0.5f : 0.9f;
            Sfx::Play("timer", 0.45f, 0);
        }
    }
}

void FightScene::StepModel(float dt) {
    // Edges are latched until a fixed step consumes them.
    if (phase == Phase::Fight && !autoplay) {
        latched.moveX = controls.MoveX();
        latched.guard = controls.Held(Action::Guard);
        latched.jump |= controls.Pressed(Action::Jump);
        latched.attack |= controls.Pressed(Action::Attack);
        latched.dash |= controls.Pressed(Action::Dash);
        latched.beam |= controls.Pressed(Action::Beam);
    }
    accumulator += dt;
    int steps = 0;
    while (accumulator >= kFightStep && steps < 6) {
        accumulator -= kFightStep;
        ++steps;
        FightInput in = phase == Phase::Fight ? (autoplay ? bot.Think(fight) : latched) : FightInput{};
        fight.Step(in);
        latched.jump = latched.attack = latched.dash = latched.beam = false;
        HandleEvents();
        const Hero& h = fight.hero();
        ghostTimer -= kFightStep;
        if ((h.state == HeroState::Dash || h.state == HeroState::AirKick) && ghostTimer <= 0) {
            ghostTimer = 0.035f;
            ghosts.push_back({h.x, h.y, h.facing, h.state == HeroState::Dash ? 14.0f * h.facing : 32.0f * h.facing, 0.22f,
                              "z_step"});
        }
        if (fight.kaiju().state == KaijuState::Active && fight.kaiju().move == KaijuMove::Charge && rng.Chance(0.5f))
            fx.Dust({fight.kaiju().x - fight.kaiju().facing * 60, 0}, 1, 80);
    }
}

void FightScene::CollapseNear(float x, float radius) {
    for (auto& b : buildings) {
        if (!b.down && std::fabs(b.x - x) < radius + b.w / 2) {
            b.down = true;
            fx.Debris({b.x, b.h * 0.6f}, 10, b.color);
            fx.Dust({b.x, 0}, 8, b.w);
        }
    }
}

void FightScene::HandleEvents() {
    for (const auto& e : fight.events()) {
        bool sound = !e.text.empty() && e.kind != EventKind::Text;
        switch (e.kind) {
        case EventKind::Hit:
            if (e.onKaiju) {
                bool heavy = e.amount >= 40;
                fx.Sparks(e.at, {255, 230, 140, 255}, heavy ? 22 : 12, heavy ? 900 : 600, heavy ? 7 : 5);
                fx.Flash(e.at, heavy ? 150 : 90, {255, 240, 200, 255});
                if (e.text.empty()) fx.Embers(e.at, 2, {150, 230, 255, 255}); // beam ticks
                else fx.Ring(e.at, heavy ? 130 : 80, WithAlpha(WHITE, 0.8f), 0.25f);
                if (heavy) shake = std::max(shake, 7.0f);
            } else {
                fx.Sparks(e.at, {255, 90, 80, 255}, 14, 700, 6);
                fx.Flash(e.at, 120, {255, 110, 90, 255});
            }
            break;
        case EventKind::Guard:
            fx.Sparks(e.at, {130, 200, 255, 255}, 12, 600, 5);
            fx.Ring(e.at, 110, WithAlpha(Palette::cyan, 0.8f), 0.3f);
            break;
        case EventKind::Parry:
            fx.Flash(e.at, 260, WHITE, 0.3f);
            fx.Ring(e.at, 240, WithAlpha(WHITE, 0.9f), 0.4f);
            fx.Sparks(e.at, {200, 240, 255, 255}, 24, 1000, 6);
            slowmo = std::max(slowmo, 0.35f);
            break;
        case EventKind::Shake:
            shake = std::max(shake, e.amount);
            break;
        case EventKind::Text:
            fx.Text(e.at, e.text, e.onKaiju ? Palette::red : Palette::gold, 34);
            break;
        case EventKind::Dust:
            fx.Dust(e.at, (int)e.amount, 160);
            if (e.amount >= 20) CollapseNear(e.at.x, 160);
            break;
        case EventKind::Debris:
            fx.Debris(e.at, (int)e.amount, {150, 140, 130, 255});
            fx.Dust({e.at.x, 0}, 14, 200);
            CollapseNear(e.at.x, 200);
            break;
        case EventKind::RockBreak:
            fx.Debris(e.at, 12, {130, 100, 76, 255});
            fx.Dust(e.at, 5, 80, {150, 120, 90, 200});
            break;
        case EventKind::Enrage:
            fx.Ring(e.at, 520, WithAlpha(Palette::red, 0.8f), 0.7f);
            fx.Flash(e.at, 400, {255, 60, 40, 255}, 0.5f);
            break;
        case EventKind::Explode:
            exploded = true;
            explodedAt = sceneTime;
            for (int i = 0; i < 6; ++i)
                fx.Flash({e.at.x + Rand(-120, 120), e.at.y + Rand(-100, 160)}, Rand(220, 380),
                         {255, (unsigned char)Rand(140, 220), 80, 255}, Rand(0.4f, 0.9f));
            fx.Ring(e.at, 640, WithAlpha({255, 200, 120, 255}, 0.8f), 0.8f);
            fx.Debris(e.at, 40, {160, 120, 80, 255});
            fx.Embers(e.at, 40, {255, 170, 70, 255});
            fx.Dust({e.at.x, 0}, 30, 420);
            CollapseNear(e.at.x, 320);
            break;
        case EventKind::Beam:
            fx.Flash({e.at.x + fight.hero().facing * 70, e.at.y}, 200, {180, 240, 255, 255}, 0.3f);
            shake = std::max(shake, 6.0f);
            break;
        case EventKind::Sound:
            break;
        }
        if (sound) {
            float volume = e.text == "hurt" ? 0.8f : e.text == "swing" ? 0.45f : 0.9f;
            Sfx::Play(e.text.c_str(), volume);
        }
    }
    fight.events().clear();
}

void FightScene::UpdateCamera(float dt) {
    const Hero& h = fight.hero();
    const Kaiju& k = fight.kaiju();
    bool kaijuGone = exploded;
    float mid = kaijuGone ? h.x : (h.x + k.x) / 2;
    float dist = kaijuGone ? 0 : std::fabs(h.x - k.x);
    float target = Clamp(1180.0f / (dist + 640.0f), 0.66f, 1.0f);
    if (phase == Phase::Intro && story && phaseTime < 1.5f) target = 0.75f;
    zoom = Lerpf(zoom, target, 1 - std::exp(-3.0f * dt));
    float half = 640 / zoom;
    float goal = Clamp(mid, half, kArenaW - half);
    camX = Lerpf(camX, goal, 1 - std::exp(-5.0f * dt));
}

Vector2 FightScene::ToScreen(Vector2 w) const {
    return {(w.x - camX) * zoom + 640 + shakeOffset.x, kGround - w.y * zoom + shakeOffset.y};
}

// ---------------------------------------------------------------- drawing

void FightScene::DrawBackground() const {
    bool enraged = fight.kaiju().enraged && !exploded;
    Color top = enraged ? Color{40, 18, 40, 255} : Color{22, 34, 86, 255};
    Color horizon = enraged ? Color{220, 90, 60, 255} : Color{238, 150, 104, 255};
    Draw::VerticalGradientRect({0, 0, (float)kCanvasW, 470}, top, horizon);
    DrawRectangleRec({0, 470, (float)kCanvasW, 260}, horizon);
    // Low sun and clouds.
    float sunX = 640 - (camX - kArenaW / 2) * 0.05f;
    Draw::BlendAdd();
    Draw::Glow({sunX, 420}, 300, WithAlpha({255, 190, 120, 255}, 0.5f));
    Draw::BlendNormal();
    DrawCircleV({sunX, 420}, 56, {255, 214, 150, 255});
    Rng r(11);
    for (int i = 0; i < 8; ++i) {
        float x = std::fmod(r.Range(0, 2600) - camX * 0.08f + 2600 * 4, 2600.0f) - 400;
        float y = r.Range(90, 300);
        Color c = WithAlpha(Mix(horizon, WHITE, 0.3f), 0.35f);
        Draw::Ellipse({x, y}, r.Range(90, 180), r.Range(14, 26), c);
        Draw::Ellipse({x + 60, y - 10}, r.Range(50, 100), r.Range(12, 20), c);
    }
    // Parallax layers: mountains, far skyline, mid city.
    auto layerX = [&](float x, float p) { return (x - camX * p) * Lerpf(1, zoom, p) + 640; };
    std::vector<Vector2> hills;
    for (int i = 0; i <= 40; ++i) {
        float wx = i * 120.0f - 800;
        float hgt = 70 + 50 * std::sin(i * 0.9f) + 30 * std::sin(i * 2.3f);
        hills.push_back({layerX(wx, 0.15f), 470 - hgt});
    }
    for (size_t i = 0; i + 1 < hills.size(); ++i)
        Draw::Quad(hills[i], hills[i + 1], {hills[i + 1].x, 560}, {hills[i].x, 560}, Mix({70, 64, 120, 255}, horizon, 0.35f));
    Rng s(23);
    for (float x = -900; x < kArenaW + 900; x += s.Range(40, 90)) {
        float w = s.Range(36, 80), hgt = s.Range(60, 180);
        float sx = layerX(x, 0.35f);
        if (sx + w < 0 || sx > kCanvasW) continue;
        DrawRectangleRec({sx, 560 - hgt, w, hgt}, Mix({52, 56, 104, 255}, horizon, 0.18f));
        for (int wy = 0; wy < (int)(hgt / 18); ++wy)
            for (int wx = 0; wx < (int)(w / 14); ++wx)
                if (s.Chance(0.25f)) DrawRectangleRec({sx + 5 + wx * 14, 566 - hgt + wy * 18, 5, 7}, WithAlpha({255, 220, 140, 255}, 0.6f));
    }
    // The middle district sits well behind the fighters: low and hazy, so the giants tower over it.
    Rng m(37);
    float midScale = Lerpf(1, zoom, 0.6f);
    for (float x = -700; x < kArenaW + 700; x += m.Range(60, 120)) {
        float w = m.Range(50, 110) * midScale, hgt = m.Range(50, 130) * midScale;
        float sx = layerX(x, 0.6f);
        float base = kGround - 30 * midScale;
        if (sx + w < 0 || sx > kCanvasW) continue;
        DrawRectangleRec({sx, base - hgt, w, hgt + 30 * midScale}, Mix({46, 48, 86, 255}, horizon, 0.3f));
        DrawRectangleRec({sx, base - hgt, w, 3}, WithAlpha(horizon, 0.4f));
    }
}

void FightScene::DrawBuildings() const {
    for (const auto& b : buildings) {
        Vector2 base = ToScreen({b.x, 0});
        float w = b.w * zoom, hgt = b.h * zoom * (1 - 0.82f * EaseInOut(b.fall));
        if (base.x + w < -40 || base.x - w > kCanvasW + 40) continue;
        Rectangle r = {base.x - w / 2, base.y - hgt, w, hgt};
        if (b.down && b.fall >= 1) {
            // Rubble mound.
            Draw::Ellipse({base.x, base.y}, w * 0.6f, b.h * zoom * 0.16f + 4, Darken(b.color, 0.35f));
            for (int i = 0; i < 5; ++i)
                DrawRectanglePro({base.x - w * 0.4f + i * w * 0.2f, base.y - 6 * zoom, 18 * zoom, 10 * zoom}, {9 * zoom, 5 * zoom},
                                 i * 37.0f, Darken(b.color, 0.15f + i * 0.05f));
            continue;
        }
        float tilt = b.down ? b.fall * 14 : 0;
        DrawRectanglePro({base.x, base.y, w, hgt}, {w / 2, hgt}, tilt, b.color);
        if (tilt == 0) {
            DrawRectangleRec({r.x, r.y, r.width, 5 * zoom}, Darken(b.color, 0.3f));
            if (b.style == 0)
                Draw::Tri({r.x - 6 * zoom, r.y}, {r.x + r.width + 6 * zoom, r.y}, {base.x, r.y - 30 * zoom}, Darken(b.color, 0.25f));
            int cols = std::max(1, (int)(b.w / 26)), rows = std::max(1, (int)(b.h / 30));
            for (int y = 0; y < rows; ++y)
                for (int x = 0; x < cols; ++x) {
                    float wx = r.x + (x + 0.5f) * r.width / cols - 5 * zoom, wy = r.y + 14 * zoom + y * 30 * zoom;
                    bool lit = ((int)(b.x * 7) + x * 13 + y * 7) % 5 < 2;
                    DrawRectangleRec({wx, wy, 10 * zoom, 13 * zoom}, lit ? Color{255, 214, 140, 230} : Color{40, 50, 80, 230});
                }
            if (b.style == 2) DrawRectangleRec({r.x + 6 * zoom, base.y - 34 * zoom, r.width - 12 * zoom, 12 * zoom}, {220, 70, 60, 255});
        }
    }
}

void FightScene::DrawGround() const {
    DrawRectangleRec({0, kGround + shakeOffset.y, (float)kCanvasW, (float)kCanvasH}, {58, 54, 60, 255});
    DrawRectangleRec({0, kGround + shakeOffset.y, (float)kCanvasW, 6}, {96, 88, 84, 255});
    for (float x = 0; x < kArenaW; x += 160) {
        Vector2 a = ToScreen({x, 0});
        DrawRectangleRec({a.x, a.y + 30, 70 * zoom, 5}, WithAlpha({220, 200, 120, 255}, 0.5f));
    }
    // Arena edges.
    for (float edge : {kArenaMargin - 80, kArenaW - kArenaMargin + 80}) {
        Vector2 a = ToScreen({edge, 0});
        DrawRectangleRec({a.x - 2, a.y, 4, 40}, WithAlpha(BLACK, 0.3f));
    }
}

void FightScene::DrawPoseAt(const std::string& key, Vector2 feet, float wpp, int facing, float rotation, Vector2 scale,
                            Color tint, float flash) const {
    const Pose* pose = Poses::Get(key);
    if (!pose) return;
    const Texture2D* t = pose->texture.Get();
    if (!t) return;
    bool flip = pose->facing == 0 ? facing < 0 : pose->facing != facing;
    float w = t->width * wpp * zoom * scale.x, h = t->height * wpp * zoom * scale.y;
    float anchor = flip ? 1 - pose->anchor : pose->anchor;
    Vector2 s = ToScreen(feet);
    Rectangle src = {0, 0, flip ? -(float)t->width : (float)t->width, (float)t->height};
    Rectangle dst = {s.x, s.y, w, h};
    Vector2 origin = {anchor * w, h};
    DrawTexturePro(*t, src, dst, origin, rotation, tint);
    if (flash > 0) {
        Draw::BlendAdd();
        DrawTexturePro(*t, src, dst, origin, rotation, WithAlpha(WHITE, flash));
        Draw::BlendNormal();
    }
}

void FightScene::DrawHero() const {
    const Hero& h = fight.hero();
    float t = h.stateTime;
    int f = h.facing;
    std::string key = fight.timerWarning() ? "z_hurt" : "z_idle";
    float rot = 0;
    Vector2 scale = {1, 1};
    Vector2 offset = {0, 0};
    switch (h.state) {
    case HeroState::Idle:
        scale.y = 1 + 0.008f * std::sin(sceneTime * 3);
        break;
    case HeroState::Walk:
        key = std::fmod(t, 0.52f) < 0.26f ? "z_step" : "z_idle";
        offset.y = std::fabs(std::sin(t * 12)) * 6;
        rot = 3.0f * f;
        break;
    case HeroState::Air:
        key = "z_step";
        rot = 6.0f * f;
        break;
    case HeroState::AirKick:
        key = "z_step";
        rot = 32.0f * f;
        break;
    case HeroState::Dash:
        key = "z_step";
        rot = 14.0f * f;
        break;
    case HeroState::Attack: {
        key = h.combo < 2 ? "z_step" : "z_idle";
        float lean = h.combo < 2 ? 11.0f : 20.0f;
        float k = t < 0.1f ? t / 0.1f : std::max(0.0f, 1 - (t - 0.1f) / 0.3f);
        rot = lean * f * k;
        offset.x = f * 16 * k;
        break;
    }
    case HeroState::Guard:
        key = "z_hurt";
        rot = -4.0f * f;
        break;
    case HeroState::Hurt:
        key = "z_hurt";
        rot = -10.0f * f;
        break;
    case HeroState::Down:
        key = "z_hurt";
        rot = -f * 78.0f * EaseOut(t / 0.3f);
        break;
    case HeroState::GetUp:
        key = "z_hurt";
        rot = -f * 78.0f * (1 - EaseOut(t / 0.35f));
        break;
    case HeroState::BeamCharge:
    case HeroState::Beam:
        key = "z_beam";
        break;
    case HeroState::Win:
        key = "z_win";
        break;
    case HeroState::Lose:
        key = "z_hurt";
        scale.y = 1 - 0.1f * EaseOut(t / 1.5f);
        rot = -6.0f * f * EaseOut(t / 1.5f);
        break;
    }
    // Afterimages.
    for (const auto& g : ghosts)
        DrawPoseAt(g.pose, {g.x, g.y}, kHeroWpp, g.facing, g.rot, {1, 1}, WithAlpha({120, 200, 255, 255}, g.life / 0.22f * 0.45f), 0);
    Vector2 feet = {h.x + offset.x, h.y + offset.y};
    Vector2 shadow = ToScreen({h.x, 0});
    Draw::Shadow(shadow, 90 * zoom * (1 - std::min(0.6f, h.y / 600)), 14 * zoom, 0.4f);
    Color tint = WHITE;
    if (h.state == HeroState::Lose) tint = {170, 170, 190, 255};
    if (h.invulnerable > 0 && h.state == HeroState::Idle && std::fmod(sceneTime, 0.12f) < 0.06f) tint.a = 140;
    float flash = std::max(0.0f, h.flash / 0.14f) * 0.8f;
    // Charging light at the arms.
    if (h.state == HeroState::BeamCharge) {
        Vector2 hand = ToScreen({h.x + f * 40, h.y + 300});
        Draw::BlendAdd();
        Draw::Glow(hand, (60 + 120 * t) * zoom, WithAlpha({150, 230, 255, 255}, 0.4f + t));
        Draw::BlendNormal();
    }
    if (story && phase == Phase::Intro && phaseTime < 0.9f) {
        // Arriving inside a pillar of light.
        float k = phaseTime / 0.9f;
        Vector2 top = ToScreen({h.x, 1400 * (1 - k)});
        Draw::BlendAdd();
        Draw::GlowStretched({top.x, (top.y + kGround) / 2}, 90 * zoom, (kGround - top.y) / 2 + 40, 0,
                            WithAlpha({170, 230, 255, 255}, 0.8f));
        Draw::BlendNormal();
        return;
    }
    DrawPoseAt(key, feet, kHeroWpp, f, rot, scale, tint, flash);
    if (h.state == HeroState::Guard) {
        // Z's barrier: a hexagonal shimmer in front.
        Vector2 c = ToScreen({h.x + f * 95, h.y + 230});
        float r = 120 * zoom;
        std::vector<Vector2> hex;
        for (int i = 0; i < 6; ++i) hex.push_back({c.x + std::cos(i * PI / 3) * r * 0.55f, c.y + std::sin(i * PI / 3) * r});
        Draw::BlendAdd();
        Draw::Poly(hex, WithAlpha({90, 180, 255, 255}, 0.22f + 0.06f * std::sin(sceneTime * 20)));
        Draw::PolyLine(hex, 3 * zoom, WithAlpha({170, 230, 255, 255}, 0.8f));
        Draw::BlendNormal();
    }
    if (h.state == HeroState::Attack) {
        static const float reach[] = {150, 170, 210};
        float active0 = h.combo < 2 ? 0.07f : 0.12f;
        if (t >= active0 && t < active0 + 0.1f) {
            Vector2 fist = ToScreen({h.x + f * reach[h.combo] * 0.85f, h.y + 230 + h.combo * 30.0f});
            Draw::BlendAdd();
            Draw::Glow(fist, 70 * zoom, WithAlpha({200, 235, 255, 255}, 0.8f));
            if (h.combo == 2) Draw::Arc(ToScreen({h.x, h.y + 200}), 170 * zoom, 12 * zoom, f > 0 ? -1.2f : PI - 0.3f,
                                        f > 0 ? 0.3f : PI + 1.2f, WithAlpha({170, 220, 255, 255}, 0.7f));
            Draw::BlendNormal();
        }
    }
    // The colour timer on the chest.
    if (h.state != HeroState::Down && h.state != HeroState::GetUp) {
        Vector2 chest = ToScreen({h.x + offset.x, h.y + offset.y + 318 * scale.y});
        bool warn = fight.timerWarning() && std::fmod(sceneTime, 0.5f) < 0.25f;
        Color c = h.state == HeroState::Lose ? Color{60, 60, 70, 255} : fight.timerWarning() ? (warn ? Palette::red : Color{120, 20, 20, 255}) : Palette::timer;
        Draw::BlendAdd();
        Draw::Glow(chest, 26 * zoom, WithAlpha(c, 0.9f));
        Draw::BlendNormal();
    }
}

void FightScene::DrawKaiju() const {
    const Kaiju& k = fight.kaiju();
    if (exploded) return;
    float t = k.stateTime;
    int f = k.facing;
    float progress = k.stateLength > 0 ? Clamp(t / k.stateLength, 0.0f, 1.0f) : 0;
    std::string key = "rk_idle";
    float rot = 0;
    Vector2 scale = {1, 1};
    Vector2 offset = {0, 0};
    switch (k.state) {
    case KaijuState::Idle:
        scale.y = 1 + 0.01f * std::sin(sceneTime * 2.2f);
        break;
    case KaijuState::Walk:
        rot = 3 * std::sin(sceneTime * 7);
        offset.y = std::fabs(std::sin(sceneTime * 7)) * 5;
        break;
    case KaijuState::Windup:
        switch (k.move) {
        case KaijuMove::Swing: key = "rk_raise"; rot = -8.0f * f * progress; break;
        case KaijuMove::Charge: key = "rk_roar"; rot = -6.0f * f; offset.x = Rand(-3, 3); break;
        case KaijuMove::Boulder: scale.y = 1 - 0.07f * EaseOut(progress); break;
        case KaijuMove::Stomp: key = k.y > 0 ? "rk_idle" : "rk_raise"; break;
        default: break;
        }
        break;
    case KaijuState::Active:
        switch (k.move) {
        case KaijuMove::Swing: key = "rk_raise"; rot = 14.0f * f; break;
        case KaijuMove::Charge: key = "rk_raise"; rot = 16.0f * f + 2 * std::sin(sceneTime * 30); break;
        case KaijuMove::Boulder: key = "rk_raise"; break;
        case KaijuMove::Stomp: key = k.y > 0 ? "rk_idle" : "rk_raise"; if (k.y <= 0) scale.y = 0.96f; break;
        default: break;
        }
        break;
    case KaijuState::Recover:
        break;
    case KaijuState::Stagger:
        rot = -8.0f * f;
        break;
    case KaijuState::Stunned:
        rot = -10.0f * f + 3 * std::sin(sceneTime * 5);
        break;
    case KaijuState::Roar:
        key = "rk_roar";
        offset.x = Rand(-4, 4);
        break;
    case KaijuState::Dying:
        key = "rk_roar";
        rot = -f * 28.0f * EaseInOut(progress);
        offset.x = Rand(-3, 3) * progress;
        break;
    case KaijuState::Win:
        key = "rk_roar";
        break;
    }
    Vector2 shadow = ToScreen({k.x, 0});
    Draw::Shadow(shadow, 130 * zoom, 18 * zoom, 0.45f);
    if (k.enraged || k.state == KaijuState::Roar) {
        Vector2 c = ToScreen({k.x, k.y + 220});
        Draw::BlendAdd();
        Draw::GlowStretched(c, 220 * zoom, 300 * zoom, 0, WithAlpha({255, 60, 30, 255}, 0.28f + 0.1f * std::sin(sceneTime * 6)));
        Draw::BlendNormal();
    }
    Color tint = k.enraged ? Color{255, 220, 210, 255} : WHITE;
    float flash = std::max(0.0f, k.flash / 0.1f) * 0.6f;
    if (k.state == KaijuState::Dying) flash = std::max(flash, (std::fmod(sceneTime, 0.2f) < 0.1f ? 0.5f : 0.0f) * progress);
    DrawPoseAt(key, {k.x + offset.x, k.y + offset.y}, kKaijuWpp, f, rot, scale, tint, flash);
    // Swing claw trail.
    if (k.state == KaijuState::Active && k.move == KaijuMove::Swing) {
        Vector2 c = ToScreen({k.x + f * 120, 260});
        Draw::BlendAdd();
        Draw::Arc(c, 200 * zoom, 22 * zoom, f > 0 ? -1.4f : PI - 0.5f, f > 0 ? 0.5f : PI + 1.4f, WithAlpha({255, 200, 120, 255}, 0.7f));
        Draw::BlendNormal();
    }
    if (k.state == KaijuState::Windup && k.move == KaijuMove::Boulder && progress > 0.4f) {
        Vector2 hand = ToScreen({k.x + f * 60, 420 * progress});
        DrawCircleV(hand, 44 * zoom, {120, 96, 74, 255});
        DrawCircleV({hand.x - 12 * zoom, hand.y - 14 * zoom}, 16 * zoom, {150, 124, 98, 255});
    }
    if (k.state == KaijuState::Stunned) {
        Vector2 head = ToScreen({k.x, 470});
        for (int i = 0; i < 4; ++i) {
            float a = sceneTime * 4 + i * PI / 2;
            Draw::Star({head.x + std::cos(a) * 60 * zoom, head.y + std::sin(a) * 16 * zoom}, 14 * zoom, 6 * zoom, a, Palette::gold);
        }
    }
    if (k.state == KaijuState::Dying) {
        Vector2 body = ToScreen({k.x + Rand(-90, 90), Rand(80, 380)});
        if (std::fmod(sceneTime, 0.18f) < 0.03f) {
            Draw::BlendAdd();
            Draw::Glow(body, 80 * zoom, WithAlpha({255, 200, 120, 255}, 0.9f));
            Draw::BlendNormal();
        }
    }
}

void FightScene::DrawProjectiles() const {
    for (const auto& r : fight.rocks()) {
        Vector2 c = ToScreen({r.x, r.y});
        Vector2 g = ToScreen({r.x, 0});
        Draw::Shadow(g, 50 * zoom * (1 - std::min(0.7f, r.y / 900)), 8 * zoom, 0.35f);
        std::vector<Vector2> pts;
        for (int i = 0; i < 9; ++i) {
            float a = i * 2 * PI / 9 + r.spin * 4;
            float rad = 46 * zoom * (0.82f + 0.18f * std::sin(i * 2.7f));
            pts.push_back({c.x + std::cos(a) * rad, c.y + std::sin(a) * rad});
        }
        Draw::Fan(c, pts, {118, 92, 70, 255});
        Draw::PolyLine(pts, 3 * zoom, {70, 52, 40, 255});
        DrawCircleV({c.x - 12 * zoom, c.y - 14 * zoom}, 12 * zoom, {154, 126, 100, 255});
    }
    for (const auto& w : fight.waves()) {
        Vector2 c = ToScreen({w.x, 0});
        float dir = w.speed > 0 ? 1.0f : -1.0f;
        float a = std::min(1.0f, w.life);
        Draw::Ellipse({c.x, c.y - 16 * zoom}, 70 * zoom, 34 * zoom, WithAlpha({190, 160, 120, 255}, 0.55f * a));
        Draw::Tri({c.x - dir * 70 * zoom, c.y}, {c.x + dir * 50 * zoom, c.y}, {c.x + dir * 20 * zoom, c.y - 64 * zoom},
                  WithAlpha({140, 110, 80, 255}, 0.9f * a));
        Draw::BlendAdd();
        Draw::GlowStretched({c.x, c.y - 10 * zoom}, 90 * zoom, 30 * zoom, 0, WithAlpha({255, 200, 120, 255}, 0.35f * a));
        Draw::BlendNormal();
    }
}

void FightScene::DrawBeam() const {
    Vector2 from, to;
    if (!fight.BeamSegment(from, to)) return;
    Vector2 a = ToScreen(from), b = ToScreen(to);
    float t = fight.hero().stateTime;
    float grow = EaseOut(t / 0.12f);
    b = {a.x + (b.x - a.x) * grow, b.y};
    float wobble = 1 + 0.12f * std::sin(sceneTime * 60);
    float thick = 54 * zoom * wobble;
    Draw::BlendAdd();
    Vector2 mid = {(a.x + b.x) / 2, (a.y + b.y) / 2};
    float len = std::fabs(b.x - a.x);
    Draw::GlowStretched(mid, len / 2 + 40, thick * 1.8f, 0, WithAlpha({80, 170, 255, 255}, 0.55f));
    Draw::Line(a, b, thick, WithAlpha({120, 210, 255, 255}, 0.75f));
    Draw::Line(a, b, thick * 0.45f, WithAlpha(WHITE, 0.95f));
    for (int i = 0; i < 10; ++i) {
        float k = std::fmod(i * 0.1f + sceneTime * 2.4f, 1.0f);
        Vector2 p = {a.x + (b.x - a.x) * k, a.y + std::sin(k * 30 + sceneTime * 20) * thick * 0.5f};
        DrawCircleV(p, 5 * zoom, WithAlpha({255, 230, 140, 255}, 0.8f));
    }
    Draw::Glow(b, 120 * zoom * wobble, WithAlpha({200, 240, 255, 255}, 0.9f));
    Draw::Glow(a, 80 * zoom, WithAlpha({200, 240, 255, 255}, 0.9f));
    Draw::BlendNormal();
}

void FightScene::DrawWarning() const {
    float p = fight.WindupProgress();
    const Kaiju& k = fight.kaiju();
    if (p < 0 || phase != Phase::Fight) return;
    Vector2 c = ToScreen({k.x, 500});
    c.y = std::max(c.y, 150.0f);
    Color color = MoveColor(k.move);
    float pulse = 1 + 0.1f * std::sin(sceneTime * 18);
    DrawCircleV(c, 30 * pulse, WithAlpha(Palette::ink, 0.8f));
    DrawRing(c, 30, 36, -90, -90 + 360 * p, 40, color);
    Gui::Text("!", c.x, c.y - 22, 38, color, Gui::Align::Center, true);
    Gui::TextOutline(MoveName(k.move), c.x, c.y + 40, 24, color, Palette::ink, 3);
    Gui::TextOutline(MoveHint(k.move), c.x, c.y + 68, 17, Palette::text, Palette::ink, 3);
}

void FightScene::DrawBanner(const char* text, const char* sub, Color color, float t) const {
    float in = EaseOut(t / 0.3f);
    float y = 300;
    DrawRectangleRec({0, y - 60 * in, (float)kCanvasW, 140 * in}, WithAlpha(Palette::ink, 0.65f));
    DrawRectangleRec({0, y - 60 * in, (float)kCanvasW, 3}, WithAlpha(color, 0.8f));
    DrawRectangleRec({0, y + 80 * in - 3, (float)kCanvasW, 3}, WithAlpha(color, 0.8f));
    float size = 74 * (1.4f - 0.4f * in);
    Gui::TextOutline(text, 640, y - size / 2 + 4, size, MulAlpha(color, in), Palette::ink, 5);
    if (sub) Gui::Text(sub, 640, y + 46, 22, MulAlpha(Palette::text, in), Gui::Align::Center);
}

void FightScene::DrawHud() const {
    const Hero& h = fight.hero();
    const Kaiju& k = fight.kaiju();
    // Hero panel.
    Draw::RoundedPanel({16, 14, 480, 90}, 14, WithAlpha(Palette::ink, 0.7f), WithAlpha(Palette::cyan, 0.4f), 1.5f);
    if (auto* t = Avatar("z").Get()) {
        Display::ClipScope clip({24, 20, 78, 78});
        DrawTexturePro(*t, {0, 0, (float)t->width, (float)t->height}, {24, 20, 78, 78}, {0, 0}, 0, WHITE);
    }
    DrawRectangleRoundedLinesEx({24, 20, 78, 78}, 0.2f, 6, 2, Palette::cyan);
    Gui::Text("奥特曼泽塔", 116, 20, 20, Palette::text, Gui::Align::Left, true);
    float hp = h.hp / h.maxHp;
    Color hpColor = hp > 0.5f ? Palette::green : hp > 0.25f ? Palette::gold : Palette::red;
    Draw::RoundedPanel({116, 48, 364, 18}, 6, {20, 26, 44, 255}, BLANK, 0);
    Draw::RoundedPanel({116, 48, 364 * hp, 18}, 6, hpColor, BLANK, 0);
    float e = h.energy / h.maxEnergy;
    bool ready = e >= 1;
    Draw::RoundedPanel({116, 74, 300, 14}, 5, {20, 26, 44, 255}, BLANK, 0);
    Color energyColor = ready ? (std::fmod(sceneTime, 0.4f) < 0.2f ? Palette::gold : Lighten(Palette::gold, 0.4f)) : Palette::cyan;
    Draw::RoundedPanel({116, 74, 300 * e, 14}, 5, energyColor, BLANK, 0);
    for (int i = 1; i < 4; ++i) DrawRectangleRec({116 + 75.0f * i, 74, 2, 14}, WithAlpha(Palette::ink, 0.8f));
    Gui::Text(ready ? (controls.TouchVisible() ? "光线就绪！" : "光线就绪 [U]") : "能量", 426, 71, 16,
              ready ? Palette::gold : Palette::muted, Gui::Align::Left, true);

    // Kaiju panel.
    Draw::RoundedPanel({784, 14, 480, 90}, 14, WithAlpha(Palette::ink, 0.7f), WithAlpha(Palette::red, 0.4f), 1.5f);
    Gui::Text("雷德王", 1248, 20, 20, Palette::text, Gui::Align::Right, true);
    Gui::Text(k.enraged ? "狂暴" : "怪兽", 1160, 22, 16, k.enraged ? Palette::red : Palette::muted, Gui::Align::Right, true);
    float khp = k.hp / k.maxHp;
    Draw::RoundedPanel({800, 50, 448, 22}, 7, {20, 26, 44, 255}, BLANK, 0);
    Draw::RoundedPanel({800 + 448 * (1 - khp), 50, 448 * khp, 22}, 7, k.enraged ? Color{255, 80, 60, 255} : Color{255, 150, 60, 255},
                       BLANK, 0);
    DrawRectangleRec({800 + 224 - 1, 46, 2, 30}, WithAlpha(Palette::text, 0.7f));
    Gui::Text(TextFormat("%d / %d", (int)std::ceil(k.hp), (int)k.maxHp), 800, 78, 14, Palette::muted);

    // The colour timer.
    Vector2 c = {640, 56};
    bool warn = fight.timerWarning();
    bool blink = warn && std::fmod(sceneTime, 0.5f) < 0.25f;
    Color gem = warn ? (blink ? Palette::red : Color{110, 20, 24, 255}) : Palette::timer;
    DrawCircleV(c, 34, WithAlpha(Palette::ink, 0.8f));
    DrawRing(c, 30, 34, 0, 360, 40, {200, 206, 220, 255});
    Draw::BlendAdd();
    Draw::Glow(c, warn && blink ? 70 : 50, WithAlpha(gem, 0.7f));
    Draw::BlendNormal();
    DrawCircleV(c, 24, gem);
    DrawCircleV({c.x - 8, c.y - 8}, 7, WithAlpha(WHITE, 0.6f));
    int secs = (int)std::ceil(fight.remaining());
    Gui::TextOutline(TextFormat("%d:%02d", secs / 60, secs % 60), c.x, 94, 22, warn ? Palette::red : Palette::text, Palette::ink, 3);

    if (fight.comboCount() >= 3) {
        Gui::TextOutline(TextFormat("%d", fight.comboCount()), 92, 150, 56, Palette::gold, Palette::ink, 4);
        Gui::TextOutline("连击", 92, 206, 22, Palette::text, Palette::ink, 3);
    }
    if (phase == Phase::Fight && hintTime < 14 && !controls.TouchVisible()) {
        const char* hint = "A/D 移动   K 跳跃   J 攻击(连按三段)   L 防御(看准时机=完美防御)   Shift 冲刺   U 光线   Esc 暂停";
        float a = std::min(1.0f, (14 - hintTime) / 1.5f);
        Draw::RoundedPanel({140, 676, 1000, 34}, 10, WithAlpha(Palette::ink, 0.65f * a), BLANK, 0);
        Gui::Text(hint, 640, 683, 17, MulAlpha(Palette::text, a), Gui::Align::Center);
    }
}

void FightScene::Render() {
    DrawBackground();
    DrawBuildings();
    DrawGround();
    DrawKaiju();
    DrawHero();
    DrawProjectiles();
    DrawBeam();
    fx.Draw([this](Vector2 p) { return ToScreen(p); }, zoom);
    DrawWarning();
    if (phase != Phase::Result && phase != Phase::Defeat) DrawHud();
    if (phase == Phase::Fight || phase == Phase::Intro)
        controls.Draw(Controls::Layout::Fight, fight.hero().energy >= fight.hero().maxEnergy);

    if (phase == Phase::Intro) {
        if (phaseTime > 2.4f) DrawBanner("战斗开始！", nullptr, Palette::gold, phaseTime - 2.4f);
        else if (phaseTime > 1.6f) DrawBanner("雷德王", "以凶暴闻名的怪兽，从地底苏醒了", {255, 150, 70, 255}, phaseTime - 1.6f);
        else if (story && phaseTime > 0.9f) DrawBanner("奥特曼泽塔", "降临！", Palette::cyan, phaseTime - 0.9f);
    }
    if (phase == Phase::Finish && fight.result() == FightResult::Won && exploded && sceneTime - explodedAt > 1.0f)
        DrawBanner("胜利！", nullptr, Palette::gold, sceneTime - explodedAt - 1.0f);
    if (phase == Phase::Finish && fight.result() != FightResult::Won && phaseTime > 0.8f)
        DrawBanner(fight.result() == FightResult::LostTime ? "能量耗尽……" : "泽塔倒下了……",
                   fight.result() == FightResult::LostTime ? "彩色计时器熄灭了" : nullptr, Palette::red, phaseTime - 0.8f);
    if (phase == Phase::Epilogue) dialogue.Draw();
    if (phase == Phase::Result) DrawResult();
    if (phase == Phase::Defeat) DrawDefeat();
    if (paused) DrawPause();
}

void FightScene::DrawPause() {
    DrawRectangleRec({0, 0, (float)kCanvasW, (float)kCanvasH}, WithAlpha(Palette::ink, 0.7f));
    Rectangle card = {440, showHelp ? 120.0f : 190.0f, 400, showHelp ? 480.0f : 340.0f};
    Gui::Panel(card, Palette::panel, 16);
    Gui::Text("暂停", 640, card.y + 22, 34, Palette::text, Gui::Align::Center, true);
    if (showHelp) {
        const char* lines[] = {"移动：A / D 或 ← →", "跳跃：K / W / 空格", "攻击：J（连按三段，空中为飞踢）",
                               "防御：L / S（攻击到来前一瞬按下 = 完美防御）", "冲刺：Shift（短暂无敌，可穿过冲撞）",
                               "光线：U（能量满时）", "手柄：X 攻击 · A 跳跃 · B 冲刺 · LB/RB 防御 · Y 光线"};
        float y = card.y + 80;
        for (const char* l : lines) {
            Gui::Paragraph(l, card.x + 30, y, card.width - 60, 17, Palette::text, 1.35f);
            y += 46;
        }
        if (Gui::Button({card.x + 100, card.y + card.height - 66, 200, 46}, "返回", Gui::Style::Dark)) showHelp = false;
        return;
    }
    float y = card.y + 86;
    if (Gui::Button({card.x + 60, y, 280, 50}, "继续战斗", Gui::Style::Gold) || controls.Pressed(Action::Pause)) paused = false;
    if (Gui::Button({card.x + 60, y + 62, 280, 50}, "重新开始", Gui::Style::Blue)) GoToFight(false);
    if (Gui::Button({card.x + 60, y + 124, 280, 50}, "操作说明", Gui::Style::Dark)) showHelp = true;
    if (Gui::Button({card.x + 60, y + 186, 280, 50}, "返回标题", Gui::Style::Red)) GoToTitle();
}

void FightScene::DrawResult() {
    float in = EaseOut(phaseTime / 0.4f);
    DrawRectangleRec({0, 0, (float)kCanvasW, (float)kCanvasH}, WithAlpha(Palette::ink, 0.55f * in));
    Rectangle card = {330, 110 + (1 - in) * 40, 620, 500};
    Gui::Panel(card, Palette::panel, 18);
    Gui::Text("战斗结算", 640, card.y + 24, 32, Palette::gold, Gui::Align::Center, true);
    const auto& s = fight.stats();
    int secs = (int)fight.elapsed();
    struct Row { const char* label; std::string value; };
    Row rows[] = {{"用时", TextFormat("%d:%02d", secs / 60, secs % 60)},
                  {"剩余体力", TextFormat("%d%%", (int)std::round(fight.hero().hp / fight.hero().maxHp * 100))},
                  {"最大连击", TextFormat("%d", s.maxCombo)},
                  {"完美防御", TextFormat("%d", s.perfectGuards)},
                  {"击碎岩石", TextFormat("%d", s.rocksBroken)}};
    float y = card.y + 92;
    for (const auto& r : rows) {
        Gui::Text(r.label, card.x + 50, y, 21, Palette::muted);
        Gui::Text(r.value, card.x + 330, y, 21, Palette::text, Gui::Align::Right, true);
        DrawRectangleRec({card.x + 50, y + 34, 280, 1}, WithAlpha(Palette::line, 0.6f));
        y += 50;
    }
    int r = std::max(0, rank);
    float pop = EaseOut((phaseTime - 0.5f) / 0.3f);
    if (pop > 0) {
        Vector2 c = {card.x + 470, card.y + 210};
        Draw::BlendAdd();
        Draw::Glow(c, 150 * pop, WithAlpha(kRankColors[r], 0.4f));
        Draw::BlendNormal();
        Gui::Text("评价", c.x, card.y + 92, 20, Palette::muted, Gui::Align::Center);
        float size = 120 * (1.6f - 0.6f * pop);
        Gui::TextOutline(kRankNames[r], c.x, c.y - size / 2, size, kRankColors[r], Palette::ink, 6);
        if (newBest && pop >= 1)
            Gui::TextOutline("新纪录！", c.x, c.y + 80, 24, Palette::gold, Palette::ink, 3);
    }
    if (Gui::Button({card.x + 60, card.y + card.height - 80, 230, 54}, "再战一次", Gui::Style::Gold)) GoToFight(false);
    if (Gui::Button({card.x + 330, card.y + card.height - 80, 230, 54}, "返回标题", Gui::Style::Dark)) GoToTitle();
}

void FightScene::DrawDefeat() {
    float in = EaseOut(phaseTime / 0.4f);
    DrawRectangleRec({0, 0, (float)kCanvasW, (float)kCanvasH}, WithAlpha(Palette::ink, 0.6f * in));
    Rectangle card = {380, 160 + (1 - in) * 40, 520, 380};
    Gui::Panel(card, Palette::panel, 18);
    Gui::Text("挑战失败", 640, card.y + 26, 34, Palette::red, Gui::Align::Center, true);
    const char* tip = fight.result() == FightResult::LostTime
                          ? "彩色计时器只有 120 秒。攒满能量后尽早用光线收尾，别让时间耗尽。"
                          : "看到怪兽头顶的「！」再应对：重击时防御或后撤，冲撞时冲刺穿过，跺地时起跳。";
    Gui::Paragraph(tip, card.x + 50, card.y + 100, card.width - 100, 20, Palette::text, 1.55f);
    float khp = fight.kaiju().hp / fight.kaiju().maxHp;
    Gui::Text(TextFormat("雷德王剩余体力 %d%%", (int)std::round(khp * 100)), 640, card.y + 210, 20, Palette::muted,
              Gui::Align::Center);
    if (Gui::Button({card.x + 40, card.y + card.height - 84, 210, 54}, "再次挑战", Gui::Style::Gold)) GoToFight(false);
    if (Gui::Button({card.x + 270, card.y + card.height - 84, 210, 54}, "返回标题", Gui::Style::Dark)) GoToTitle();
}

} // namespace UQ
