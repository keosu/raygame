#include "TransformScene.h"
#include "Assets.h"
#include "Characters.h"
#include "Draw.h"
#include "Game.h"
#include "Gui.h"

namespace UQ {

namespace {
constexpr float kLength = 4.2f;
struct Medal {
    const char* name;
    const char* label;
    Color face, rim;
    float at;
};
const Medal kMedals[] = {
    {"ZERO", "赛罗", {60, 120, 230, 255}, {230, 60, 60, 255}, 0.25f},
    {"SEVEN", "赛文", {210, 50, 50, 255}, {200, 200, 210, 255}, 0.7f},
    {"LEO", "雷欧", {220, 70, 50, 255}, {250, 200, 70, 255}, 1.15f},
};

void DrawRays(Vector2 c, float t, Color color, int count, float alpha) {
    Draw::BlendAdd();
    for (int i = 0; i < count; ++i) {
        float a = i * 2 * PI / count + t * 0.4f;
        Vector2 p1 = Add(c, Mul(Dir(a - 0.05f), 1600)), p2 = Add(c, Mul(Dir(a + 0.05f), 1600));
        Draw::Tri(c, p1, p2, WithAlpha(color, alpha));
    }
    Draw::BlendNormal();
}
} // namespace

void TransformScene::OnLoad() {
    Sfx::StopMusic();
    Sfx::Play("transform", 1.0f, 0);
    controls.Reset();
}

void TransformScene::Update() {
    float dt = std::min(GameTime::DeltaTime(), 0.05f);
    controls.Update(Controls::Layout::None);
    time += dt;
    if (!left && (time >= kLength || (time > 0.6f && controls.Confirm()))) {
        left = true;
        GoToFight(true);
    }
}

void TransformScene::Render() {
    float t = time;
    Draw::VerticalGradientRect({0, 0, (float)kCanvasW, (float)kCanvasH}, {6, 14, 46, 255}, {20, 60, 120, 255});
    Vector2 core = {760, 330};
    DrawRays(core, t, {120, 200, 255, 255}, 18, 0.08f + 0.06f * std::min(1.0f, t));
    // Haruki holds up the Z Riser.
    if (t < 2.6f) {
        Look look = LookFor("haruki");
        look.height = 520;
        float slide = EaseOut(t / 0.4f);
        DrawPerson(look, {320 - (1 - slide) * 200, 760}, 1, 0, 0, std::min(1.0f, t / 0.5f));
        Gui::TextOutline("宇宙拳法 · 秘传之神髓！", 640, 40, 42, WithAlpha(Palette::text, std::min(1.0f, t / 0.3f)), Palette::ink, 4);
    }
    // Medals fly in and slot into the riser's light.
    for (int i = 0; i < 3; ++i) {
        const Medal& m = kMedals[i];
        float k = (t - m.at) / 0.35f;
        if (k < 0 || t >= 2.6f) continue; // gone into the light by the burst
        float merge = Clamp((t - 1.6f) / 0.9f, 0.0f, 1.0f);
        Vector2 slot = {660 + i * 120.0f, 330};
        Vector2 from = {1400, 120 + i * 200.0f};
        Vector2 p = {from.x + (slot.x - from.x) * EaseOut(k), from.y + (slot.y - from.y) * EaseOut(k)};
        p = {p.x + (core.x - p.x) * EaseInOut(merge), p.y + (core.y - p.y) * EaseInOut(merge)};
        float r = 52 * (1 - 0.6f * merge);
        Draw::BlendAdd();
        Draw::Glow(p, r * 2.4f, WithAlpha(m.face, 0.6f));
        Draw::BlendNormal();
        DrawCircleV(p, r, m.rim);
        DrawCircleV(p, r * 0.82f, m.face);
        DrawCircleV({p.x - r * 0.3f, p.y - r * 0.3f}, r * 0.25f, WithAlpha(WHITE, 0.5f));
        if (merge < 0.5f) {
            Gui::Text(m.name, p.x, p.y - 12, 20 * (1 - merge), WHITE, Gui::Align::Center, true);
            Gui::Text(m.label, p.x, p.y + r + 8, 20, WithAlpha(Palette::text, 1 - merge * 2), Gui::Align::Center, true);
        }
    }
    if (t > 1.6f && t < 2.6f) {
        float k = (t - 1.6f) / 1.0f;
        Draw::BlendAdd();
        Draw::Glow(core, 80 + 600 * k * k, WithAlpha({200, 240, 255, 255}, 0.4f + 0.6f * k));
        Draw::BlendNormal();
        Gui::TextOutline("泽塔先生！", 640, 600, 48, Palette::gold, Palette::ink, 4);
    }
    // Ultraman Z rises out of the light.
    if (t >= 2.6f) {
        float k = (t - 2.6f) / 1.6f;
        DrawRays({640, 360}, t * 2, {255, 230, 160, 255}, 24, 0.12f);
        if (const Pose* pose = Poses::Get("z_idle")) {
            const Texture2D& tex = *pose->texture;
            float h = 560 + 140 * EaseOut(k), w = tex.width * h / tex.height;
            float y = 720 - h + (1 - EaseOut(k * 1.6f)) * 240;
            Draw::BlendAdd();
            Draw::GlowStretched({640, y + h / 2}, w, h * 0.7f, 0, WithAlpha({150, 220, 255, 255}, 0.5f));
            Draw::BlendNormal();
            DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, {640 - w / 2, y, w, h}, {0, 0}, 0, WHITE);
        }
        float title = EaseOut((t - 2.75f) / 0.3f);
        if (title > 0) {
            float size = 86 * (1.5f - 0.5f * title);
            Gui::TextOutline("奥特曼泽塔！", 640, 560 - size / 2, size, MulAlpha(Palette::cyan, title), Palette::ink, 6);
        }
        float flash = 1 - Clamp((t - 2.6f) / 0.35f, 0.0f, 1.0f);
        DrawRectangleRec({0, 0, (float)kCanvasW, (float)kCanvasH}, WithAlpha(WHITE, flash));
    }
    if (t > kLength - 0.4f) DrawRectangleRec({0, 0, (float)kCanvasW, (float)kCanvasH}, WithAlpha(WHITE, (t - (kLength - 0.4f)) / 0.4f));
    if (t > 0.6f) Gui::Text(controls.TouchVisible() ? "轻触跳过" : "按 Enter 跳过", 1250, 690, 16, WithAlpha(Palette::muted, 0.8f),
                            Gui::Align::Right);
}

// ---------------------------------------------------------------- title

void TitleScene::OnLoad() {
    Sfx::PlayMusic("title");
    controls.Reset();
}

void TitleScene::Update() {
    float dt = std::min(GameTime::DeltaTime(), 0.05f);
    Gui::BeginFrame();
    controls.Update(Controls::Layout::None);
    time += dt;
    pageTime += dt;
    if (page != Page::Main && (Input::GetKeyDown(KEY_ESCAPE) || Input::GetKeyDown(KEY_BACKSPACE))) SetPage(Page::Main);
}

void TitleScene::DrawBackdrop() const {
    Draw::VerticalGradientRect({0, 0, (float)kCanvasW, (float)kCanvasH}, {8, 14, 40, 255}, {60, 40, 90, 255});
    Rng r(3);
    for (int i = 0; i < 120; ++i) {
        Vector2 p = {r.Range(0, kCanvasW), r.Range(0, 460)};
        float tw = 0.5f + 0.5f * std::sin(time * r.Range(1, 3) + i);
        DrawCircleV(p, r.Range(0.6f, 1.8f), WithAlpha(WHITE, 0.3f + 0.5f * tw));
    }
    // Red King's silhouette on the horizon, Z in front.
    if (const Pose* rk = Poses::Get("rk_raise")) {
        const Texture2D& t = *rk->texture;
        float h = 430, w = t.width * h / t.height;
        DrawTexturePro(t, {0, 0, (float)t.width, (float)t.height}, {640, 620 - h, w, h}, {0, 0}, 0, {40, 30, 60, 230});
    }
    Rng s(9);
    for (float x = 0; x < kCanvasW; x += s.Range(40, 90)) {
        float w = s.Range(40, 90), h = s.Range(60, 190);
        DrawRectangleRec({x, 620 - h, w, h + 100}, {18, 20, 40, 255});
        for (int y = 0; y < (int)(h / 20); ++y)
            if (s.Chance(0.5f)) DrawRectangleRec({x + 8, 630 - h + y * 20, 6, 8}, WithAlpha({255, 220, 140, 255}, 0.7f));
    }
    DrawRectangleRec({0, 620, (float)kCanvasW, 100}, {14, 16, 30, 255});
    if (const Pose* z = Poses::Get("z_win")) {
        const Texture2D& t = *z->texture;
        float h = 640, w = t.width * h / t.height;
        float x = 1000 - w / 2, y = 720 - h + 20 + std::sin(time * 1.2f) * 4;
        Draw::BlendAdd();
        Draw::GlowStretched({1000, y + h * 0.45f}, w * 0.9f, h * 0.6f, 0, WithAlpha({100, 180, 255, 255}, 0.35f));
        Draw::BlendNormal();
        DrawTexturePro(t, {0, 0, (float)t.width, (float)t.height}, {x, y, w, h}, {0, 0}, 0, WHITE);
        Draw::BlendAdd();
        Draw::Glow({1000, y + h * 0.23f}, 22 + 4 * std::sin(time * 4), WithAlpha(Palette::timer, 0.9f));
        Draw::BlendNormal();
    }
    DrawRectangleRec({0, 0, 620, (float)kCanvasH}, WithAlpha(Palette::ink, 0.35f));
}

void TitleScene::DrawMain() {
    float in = EaseOut(pageTime / 0.5f);
    float x = 70 - (1 - in) * 40;
    Gui::Text("ULTRAMAN Z · FAN ADVENTURE", x, 70, 16, WithAlpha(Palette::cyan, in), Gui::Align::Left, true);
    Draw::RoundedPanel({x + 300, 66, 64, 26}, 8, WithAlpha(Palette::gold, in), BLANK, 0);
    Gui::Text(kVersion, x + 332, 70, 16, Palette::ink, Gui::Align::Center, true);
    Gui::TextShadow("泽塔传说", x - 4, 100, 96, MulAlpha(Palette::text, in), Gui::Align::Left, true, 4);
    Gui::Text("第一章 · 地底的咆哮", x, 214, 28, MulAlpha(Palette::gold, in), Gui::Align::Left, true);
    Gui::Text("调查小镇的连续地震，变身奥特曼泽塔，迎战雷德王", x, 258, 18, MulAlpha(Palette::muted, in));

    float y = 320;
    bool explored = Progress::SkipIntro();
    if (Gui::Button({x, y, 360, 58}, explored ? "重新开始冒险" : "开始冒险", Gui::Style::Gold, true, 24)) GoToExplore();
    y += 70;
    if (Gui::Button({x, y, 360, 52}, explored ? "直接挑战雷德王" : "直接挑战（调查后解锁）", Gui::Style::Red, explored, 20))
        GoToFight(false);
    y += 64;
    if (Gui::Button({x, y, 174, 48}, "操作说明", Gui::Style::Blue)) SetPage(Page::Help);
    if (Gui::Button({x + 186, y, 174, 48}, "设置", Gui::Style::Dark)) SetPage(Page::Settings);
    y += 60;
    if (Gui::Button({x, y, 360, 46}, "退出游戏", Gui::Style::Dark)) QuitGame();

    int best = Progress::BestRank();
    if (best >= 0) {
        static const char* names[] = {"C", "B", "A", "S", "SS"};
        int secs = (int)Progress::BestTime();
        Gui::Text(TextFormat("最佳评价  %s   ·   最快 %d:%02d", names[best], secs / 60, secs % 60), x, 640, 18, Palette::gold,
                  Gui::Align::Left, true);
    }
    Gui::Text("非官方同人作品 · 角色版权归圆谷制作所有", x, 680, 14, WithAlpha(Palette::muted, 0.8f));
}

void TitleScene::DrawHelp() {
    Rectangle card = {60, 70, 560, 590};
    Gui::Panel(card, Palette::panel, 16);
    Gui::Text("操作说明", card.x + 30, card.y + 22, 30, Palette::gold, Gui::Align::Left, true);
    const char* lines[] = {
        "【调查】← → / A D 移动，靠近「！」「？」按 E / J 调查。收集 3 条线索后前往工地。",
        "【战斗】A D 移动，K 跳跃，J 攻击（连按三段，空中为飞踢）。",
        "L 防御：在怪兽出手前一瞬按下是「完美防御」，不受伤害并回复能量。",
        "Shift 冲刺：短暂无敌，可以穿过雷德王的冲撞。",
        "攻击命中会积攒能量，满格后按 U 发射泽斯提姆光线。",
        "雷德王头顶出现「！」时就是出招预警，下方会提示应对方法。",
        "彩色计时器只有 120 秒，变红闪烁说明时间或体力告急。",
        "手柄：X 攻击 · A 跳跃 · B 冲刺 · LB/RB 防御 · Y 光线；触屏显示虚拟按键。",
    };
    float y = card.y + 82;
    for (const char* l : lines) y += Gui::Paragraph(l, card.x + 30, y, card.width - 60, 18, Palette::text, 1.45f) + 10;
    if (Gui::Button({card.x + 30, card.y + card.height - 70, 200, 48}, "返回", Gui::Style::Dark)) SetPage(Page::Main);
}

void TitleScene::DrawSettings() {
    Rectangle card = {60, 110, 520, 500};
    Gui::Panel(card, Palette::panel, 16);
    Gui::Text("设置", card.x + 30, card.y + 22, 30, Palette::gold, Gui::Align::Left, true);
    Settings& s = Progress::Options();
    struct Row { const char* label; bool* value; };
    std::vector<Row> rows = {{"音效", &s.sfx}, {"音乐", &s.music}, {"镜头震动", &s.shake}};
#if !defined(__ANDROID__)
    rows.push_back({"显示触屏按键", &s.touchControls});
    rows.push_back({"全屏（F11）", &s.fullscreen});
#endif
    float y = card.y + 90;
    for (auto& r : rows) {
        Gui::Text(r.label, card.x + 40, y + 12, 22, Palette::text);
        if (Gui::Button({card.x + 330, y, 140, 46}, *r.value ? "开" : "关", *r.value ? Gui::Style::Green : Gui::Style::Dark)) {
            if (r.value == &s.fullscreen) {
                ToggleFullscreenMode();
            } else {
                *r.value = !*r.value;
                Sfx::ApplySettings();
                Progress::Save();
            }
        }
        y += 62;
    }
    if (Gui::Button({card.x + 30, card.y + card.height - 70, 200, 48}, "返回", Gui::Style::Dark)) SetPage(Page::Main);
}

void TitleScene::Render() {
    DrawBackdrop();
    switch (page) {
    case Page::Main: DrawMain(); break;
    case Page::Help: DrawHelp(); break;
    case Page::Settings: DrawSettings(); break;
    }
}

} // namespace UQ
