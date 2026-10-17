#include "Menu.h"
#include "Art.h"
#include "Assets.h"
#include "Draw.h"
#include "Gui.h"
#include "Skins.h"
#include <rlgl.h>

namespace UG {

namespace {
bool openCodexOnLoad = false;
} // namespace

void PreviewCodex() { openCodexOnLoad = true; }

namespace {
constexpr float W = (float)kCanvasW, Hh = (float)kCanvasH;

float EaseOut(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return 1 - (1 - t) * (1 - t) * (1 - t);
}

void Skyline(Rng& rng, float baseY, float minH, float maxH, Color body, Color window, float windowChance) {
    float x = -10;
    while (x < W + 10) {
        float w = rng.Range(26, 70), h = rng.Range(minH, maxH);
        DrawRectangleRec({x, baseY - h, w, h + 200}, body);
        if (rng.Float() < 0.3f) DrawRectangleRec({x + w * 0.3f, baseY - h - 10, w * 0.4f, 10}, body);
        for (float wy = baseY - h + 8; wy < baseY - 6; wy += 9)
            for (float wx = x + 5; wx < x + w - 6; wx += 8)
                if (rng.Float() < windowChance) DrawRectangleRec({wx, wy, 3, 4}, window);
        x += w + rng.Range(-4, 6);
    }
}

void TokyoTower(Vector2 base, float h, Color c) {
    Draw::Tri({base.x, base.y - h}, {base.x - h * 0.22f, base.y}, {base.x - h * 0.15f, base.y}, c);
    Draw::Tri({base.x, base.y - h}, {base.x + h * 0.15f, base.y}, {base.x + h * 0.22f, base.y}, c);
    Draw::Quad({base.x - h * 0.03f, base.y - h}, {base.x + h * 0.03f, base.y - h}, {base.x + h * 0.06f, base.y - h * 0.4f},
               {base.x - h * 0.06f, base.y - h * 0.4f}, c);
    DrawRectangleRec({base.x - h * 0.12f, base.y - h * 0.38f, h * 0.24f, h * 0.04f}, c);
    DrawRectangleRec({base.x - h * 0.07f, base.y - h * 0.7f, h * 0.14f, h * 0.03f}, c);
    Draw::Line({base.x, base.y - h}, {base.x, base.y - h * 1.12f}, 2, c);
}
} // namespace

MenuScene::MenuScene(MenuPage start) : Scene("Menu"), page(start) {
    if (start == MenuPage::Heroes && openCodexOnLoad) {
        codex = true;
        openCodexOnLoad = false;
    }
}

void MenuScene::OnLoad() {
    if (!sky.id) {
        sky = LoadRenderTexture(kCanvasW, kCanvasH);
        SetTextureFilter(sky.texture, TEXTURE_FILTER_BILINEAR);
        BeginTextureMode(sky);
        ClearBackground(BLACK);
        Draw::VerticalGradientRect({0, 0, W, Hh * 0.6f}, {6, 8, 28, 255}, {34, 22, 72, 255});
        Draw::VerticalGradientRect({0, Hh * 0.6f - 1, W, Hh * 0.4f + 1}, {34, 22, 72, 255}, {120, 50, 90, 255});
        Rng rng(78);
        Draw::BlendAdd();
        Draw::Glow({W * 0.78f, Hh * 0.22f}, 380, WithAlpha({255, 60, 90, 255}, 0.22f));
        Draw::Glow({W * 0.84f, Hh * 0.16f}, 240, WithAlpha({110, 120, 255, 255}, 0.25f));
        Draw::Glow({W * 0.8f, Hh * 0.2f}, 90, WithAlpha({255, 220, 240, 255}, 0.35f));
        Draw::Glow({W * 0.2f, Hh * 0.9f}, 520, WithAlpha({255, 120, 60, 255}, 0.18f));
        for (int i = 0; i < 520; ++i) {
            Vector2 p{rng.Float() * W, rng.Float() * Hh * 0.75f};
            float r = rng.Range(0.6f, 2.2f);
            Draw::Glow(p, r * 2.2f, WithAlpha({220, 230, 255, 255}, rng.Range(0.3f, 1.0f)));
        }
        Draw::BlendNormal();
        // Distant Earth
        Vector2 earth{150, 120};
        Draw::BlendAdd();
        Draw::Glow(earth, 120, WithAlpha({80, 160, 255, 255}, 0.35f));
        Draw::BlendNormal();
        Draw::GradientEllipse(earth, 62, 62, {16, 50, 120, 255}, {90, 170, 255, 255}, {earth.x - 20, earth.y - 22}, 14);
        Draw::Ellipse({earth.x - 12, earth.y - 8}, 22, 12, {70, 150, 90, 220}, 0.4f);
        Draw::Ellipse({earth.x + 20, earth.y + 18}, 16, 9, {70, 150, 90, 220}, -0.3f);
        Draw::Ellipse({earth.x + 6, earth.y - 30}, 26, 5, WithAlpha(WHITE, 0.4f), 0.1f);
        for (int i = 0; i < 10; ++i)
            Draw::Ellipse({earth.x + i * 3.0f, earth.y + i * 2.4f}, 62 - i * 3.9f, 62 - i * 3.9f, WithAlpha({4, 6, 22, 255}, 0.08f));
        // City skyline layers
        Skyline(rng, Hh - 120, 40, 130, {44, 30, 66, 255}, {130, 90, 150, 255}, 0.08f);
        TokyoTower({W * 0.66f, Hh - 100}, 240, {60, 30, 70, 255});
        Skyline(rng, Hh - 50, 30, 120, {22, 16, 38, 255}, {255, 200, 110, 255}, 0.16f);
        DrawRectangleGradientV(0, (int)(Hh - 160), (int)W, 160, {0, 0, 0, 0}, {6, 4, 14, 220});
        EndTextureMode();
    }
    if (previews.empty()) {
        for (const auto& def : Levels()) {
            MapData m;
            m.Build(def);
            maps.push_back(m);
            previews.push_back(RenderMap(maps.back(), 1.0f / 3.0f));
        }
    }
    pageTime = 0;
    Sfx::PlayMusic("menu");
}

void MenuScene::OnUnload() {
    if (sky.id) UnloadRenderTexture(sky);
    sky = {};
    for (auto& p : previews) UnloadRenderTexture(p);
    previews.clear();
    maps.clear();
}

void MenuScene::SetPage(MenuPage next) {
    page = next;
    pageTime = 0;
    if (next == MenuPage::Levels) {
        int newest = 0;
        for (int i = 0; i < (int)Levels().size(); ++i)
            if (Progress::Unlocked(i)) newest = i;
        levelPage = newest / kLevelsPerPage;
    }
}

void MenuScene::Update() {
    Scene::Update();
    float dt = GameTime::UnscaledDeltaTime();
    pageTime += dt;
    if (GetRandomValue(0, 180) == 0) meteors.push_back({{Rand(200, W + 200), Rand(-20, 160)}, {Rand(-520, -380), Rand(160, 240)}, 1.2f});
    for (auto& m : meteors) {
        m.p = Add(m.p, Mul(m.v, dt));
        m.life -= dt;
    }
    meteors.erase(std::remove_if(meteors.begin(), meteors.end(), [](const Meteor& m) { return m.life <= 0; }), meteors.end());
    if (Input::GetKeyDown(KEY_ESCAPE) || Input::GetKeyDown(KEY_BACK)) {
        if (page == MenuPage::Title) QuitGame();
        else SetPage(MenuPage::Title);
    }
    if (page == MenuPage::Title && (Input::GetKeyDown(KEY_ENTER) || Input::GetKeyDown(KEY_SPACE))) SetPage(MenuPage::Levels);
    if (page == MenuPage::Heroes) {
        if (Input::GetKeyDown(KEY_C)) codex = !codex;
        if (codex) {
            int count = std::max(1, (int)UltraCodex().size());
            if (Input::GetKeyDown(KEY_DOWN) || Input::GetKeyDown(KEY_S)) codexIndex = (codexIndex + 1) % count;
            if (Input::GetKeyDown(KEY_UP) || Input::GetKeyDown(KEY_W)) codexIndex = (codexIndex + count - 1) % count;
        } else {
            if (Input::GetKeyDown(KEY_DOWN) || Input::GetKeyDown(KEY_S)) heroIndex = (heroIndex + 1) % kHeroCount;
            if (Input::GetKeyDown(KEY_UP) || Input::GetKeyDown(KEY_W)) heroIndex = (heroIndex + kHeroCount - 1) % kHeroCount;
        }
    }
    if (page == MenuPage::Levels) {
        int pages = std::max(1, ((int)Levels().size() + kLevelsPerPage - 1) / kLevelsPerPage);
        if (Input::GetKeyDown(KEY_RIGHT) || Input::GetKeyDown(KEY_D)) levelPage = std::min(levelPage + 1, pages - 1);
        if (Input::GetKeyDown(KEY_LEFT) || Input::GetKeyDown(KEY_A)) levelPage = std::max(levelPage - 1, 0);
    }
    if (page == MenuPage::Kaiju) {
        if (Input::GetKeyDown(KEY_RIGHT) || Input::GetKeyDown(KEY_D)) kaijuIndex = (kaijuIndex + 1) % kEnemyCount;
        if (Input::GetKeyDown(KEY_LEFT) || Input::GetKeyDown(KEY_A)) kaijuIndex = (kaijuIndex + kEnemyCount - 1) % kEnemyCount;
    }
}

void MenuScene::Render() {
    Gui::BeginFrame();
    DrawBackground();
    switch (page) {
    case MenuPage::Title:
        DrawTitle();
        break;
    case MenuPage::Levels:
        DrawLevels();
        break;
    case MenuPage::Heroes:
        DrawHeroes();
        break;
    case MenuPage::Kaiju:
        DrawKaiju();
        break;
    case MenuPage::Help:
        DrawHelp();
        break;
    }
}

void MenuScene::DrawBackground() {
    Display::PresentOpaque(sky.texture, {0, 0, W, Hh});
    float t = RealTime();
    Draw::BlendAdd();
    Rng rng(9);
    for (int i = 0; i < 60; ++i) {
        Vector2 p{rng.Float() * W, rng.Float() * Hh * 0.7f};
        float tw = 0.5f + 0.5f * std::sin(t * rng.Range(1, 4) + i);
        Draw::Glow(p, rng.Range(2, 5), WithAlpha({200, 220, 255, 255}, 0.6f * tw));
    }
    for (const auto& m : meteors) {
        Vector2 tail = Sub(m.p, Mul(m.v, 0.18f));
        Draw::Line(tail, m.p, 2, WithAlpha({200, 230, 255, 255}, 0.5f * m.life));
        Draw::Glow(m.p, 8, WithAlpha(WHITE, 0.8f * m.life));
    }
    // Rising light motes
    for (int i = 0; i < 40; ++i) {
        float speed = rng.Range(14, 40);
        float y = Hh - std::fmod(t * speed + rng.Range(0, Hh), Hh + 40);
        float x = rng.Float() * W + std::sin(t * 0.7f + i) * 14;
        Draw::Glow({x, y}, rng.Range(3, 7), WithAlpha({255, 200, 140, 255}, 0.35f));
    }
    Draw::BlendNormal();
}

void MenuScene::DrawPageTitle(const char* title, const char* sub) {
    float a = EaseOut(pageTime / 0.4f);
    Gui::TextOutline(title, W / 2, 34 - (1 - a) * 20, 44, WithAlpha(Palette::gold, a), WithAlpha({30, 10, 0, 255}, a), 2.2f);
    Gui::Text(sub, W / 2, 90, 16, WithAlpha(Palette::muted, a), Gui::Align::Center);
}

void MenuScene::DrawBack() {
    if (Gui::Button({30, 28, 120, 44}, "返回", Gui::Style::Dark, true, 18)) SetPage(MenuPage::Title);
}

void MenuScene::DrawTitle() {
    float t = RealTime();
    float a = EaseOut(pageTime / 0.6f);
    // Light burst behind the title
    Draw::BlendAdd();
    for (int i = 0; i < 14; ++i) {
        float ang = t * 0.08f + i * PI / 7;
        Vector2 c{W / 2, 130};
        Draw::Tri(c, Add(c, Mul(Dir(ang - 0.05f), 700)), Add(c, Mul(Dir(ang + 0.05f), 700)), WithAlpha({255, 220, 160, 255}, 0.035f * a));
    }
    Draw::Glow({W / 2, 130}, 320, WithAlpha({255, 200, 120, 255}, 0.18f * a));
    Draw::BlendNormal();

    Gui::Text("ULTRAMAN  TOWER  DEFENSE", W / 2, 40, 16, WithAlpha(Palette::cyan, a), Gui::Align::Center, true);
    float bounce = (1 - EaseOut(pageTime / 0.7f)) * -40;
    Gui::TextOutline("光之守护者", W / 2 + 3, 64 + bounce + 5, 92, WithAlpha({120, 20, 20, 255}, a), WithAlpha({120, 20, 20, 255}, a), 3);
    Gui::TextOutline("光之守护者", W / 2, 64 + bounce, 92, WithAlpha({255, 236, 180, 255}, a), WithAlpha({170, 40, 30, 255}, a), 3.5f);
    Gui::Text("奥特曼塔防  ·  集结光之巨人，守护地球", W / 2, 174, 21, WithAlpha(Palette::text, a), Gui::Align::Center);

    // Hero lineup
    float size = 150, gap = 22;
    float total = kHeroCount * size + (kHeroCount - 1) * gap;
    for (int i = 0; i < kHeroCount; ++i) {
        const HeroDef& def = HeroInfo((HeroId)i);
        float appear = EaseOut((pageTime - 0.15f - i * 0.08f) / 0.5f);
        if (appear <= 0) continue;
        float arc = std::fabs(i - 2.5f);
        float x = W / 2 - total / 2 + i * (size + gap);
        float y = 222 + arc * arc * 6 + std::sin(t * 1.6f + i) * 5 + (1 - appear) * 60;
        Vector2 c{x + size / 2, y + size * 0.62f};
        Draw::BlendAdd();
        Draw::Glow(c, size * 0.7f, WithAlpha(def.primary, 0.45f * appear));
        Draw::Glow(c, size * 0.38f, WithAlpha(def.beam, 0.25f * appear));
        Draw::BlendNormal();
        Draw::GlowStretched({c.x, y + size + 4}, size * 0.5f, 12, 0, WithAlpha(BLACK, 0.5f * appear));
        Art::DrawPortrait((HeroId)i, {x, y, size, size}, WithAlpha(WHITE, appear));
        Gui::TextShadow(def.name, c.x, y + size + 6, 17, WithAlpha(Palette::text, appear), Gui::Align::Center);
    }

    float by = 470 + (1 - a) * 30;
    if (Gui::Button({W / 2 - 160, by, 320, 60}, "开始作战", Gui::Style::Gold, true, 26)) SetPage(MenuPage::Levels);
    if (Gui::Button({W / 2 - 300, by + 76, 190, 46}, "奥特数据库", Gui::Style::Blue, true, 19)) {
        codex = true;
        SetPage(MenuPage::Heroes);
    }
    if (Gui::Button({W / 2 - 95, by + 76, 190, 46}, "怪兽图鉴", Gui::Style::Red, true, 19)) SetPage(MenuPage::Kaiju);
    if (Gui::Button({W / 2 + 110, by + 76, 190, 46}, "操作说明", Gui::Style::Dark, true, 19)) SetPage(MenuPage::Help);

    auto& opt = Progress::Options();
    Rectangle sfxBtn{W - 160, Hh - 64, 56, 44}, musicBtn{W - 96, Hh - 64, 56, 44};
    if (Gui::Button(sfxBtn, "", Gui::Style::Dark)) {
        opt.sfx = !opt.sfx;
        Sfx::ApplySettings();
        Progress::Save();
    }
    Gui::SoundIcon({sfxBtn.x + 28, sfxBtn.y + 21}, 11, opt.sfx ? WHITE : Palette::muted, opt.sfx);
    if (Gui::Button(musicBtn, "", Gui::Style::Dark)) {
        opt.music = !opt.music;
        Sfx::ApplySettings();
        Progress::Save();
    }
    Gui::NoteIcon({musicBtn.x + 28, musicBtn.y + 21}, 11, opt.music ? WHITE : Palette::muted, opt.music);
    if (Gui::Button({40, Hh - 64, 130, 44}, "退出游戏", Gui::Style::Dark, true, 17)) QuitGame();
    const auto& skins = Skins::All();
    if (skins.size() > 1 && Gui::Button({186, Hh - 64, 210, 44}, "皮肤：" + skins[Skins::Current()].name, Gui::Style::Dark, true, 17)) {
        Skins::Select((Skins::Current() + 1) % (int)skins.size());
        GoToMenu(MenuPage::Title); // rebuilds the baked map previews
    }
    if (!Skins::Errors().empty()) Gui::Text("皮肤文件有误：" + Skins::Errors()[0], 40, Hh - 86, 12, Palette::red);

    int stars = 0;
    for (int i = 0; i < (int)Levels().size(); ++i) stars += Progress::Stars(i);
    Gui::Text(TextFormat("已获得星星 %d / %d     累计击败怪兽 %d", stars, (int)Levels().size() * 3, Progress::TotalKills()), W / 2, Hh - 50,
              15, Palette::muted, Gui::Align::Center);
    Gui::Text("Enter 开始   F11 全屏   Esc 退出", W / 2, Hh - 28, 12, WithAlpha(Palette::muted, 0.7f), Gui::Align::Center);
}

void MenuScene::DrawLevels() {
    DrawBack();
    DrawPageTitle("选择关卡", "点选缩略图出击");
    const auto& levels = Levels();
    const auto& errors = LevelErrors();
    if (!errors.empty()) {
        Rectangle strip{50, levels.empty() ? 300.0f : 686.0f, W - 100, levels.empty() ? 120.0f : 28.0f};
        Draw::RoundedPanel(strip, 8, {60, 12, 18, 230}, WithAlpha(Palette::red, 0.8f), 1.5f);
        std::string message = "关卡文件有误：" + errors[0];
        if (errors.size() > 1) message += TextFormat("（共 %d 条，详见日志）", (int)errors.size());
        Gui::Text(message, W / 2, strip.y + strip.height / 2 - 8, 14, {255, 200, 200, 255}, Gui::Align::Center);
    }
    int pages = std::max(1, ((int)levels.size() + kLevelsPerPage - 1) / kLevelsPerPage);
    levelPage = std::clamp(levelPage, 0, pages - 1);
    if (pages > 1) {
        Gui::Text(TextFormat("%d / %d", levelPage + 1, pages), W - 160, 104, 15, Palette::muted, Gui::Align::Center);
        if (Gui::Button({W - 290, 28, 120, 44}, "上一页", Gui::Style::Dark, levelPage > 0, 18)) --levelPage;
        if (Gui::Button({W - 150, 28, 120, 44}, "下一页", Gui::Style::Dark, levelPage + 1 < pages, 18)) ++levelPage;
    }
    int first = levelPage * kLevelsPerPage;
    int shown = std::min(kLevelsPerPage, (int)levels.size() - first);
    constexpr int kColsOnPage = 3;
    constexpr float kCardW = 390, kCardH = 248, kGapX = 18, kGapY = 14;
    float gridW = kColsOnPage * kCardW + (kColsOnPage - 1) * kGapX;
    float x0 = (W - gridW) / 2;
    float y0 = 124;
    for (int i = first; i < first + shown; ++i) {
        const LevelDef& def = levels[i];
        int slot = i - first;
        int col = slot % kColsOnPage, row = slot / kColsOnPage;
        float appear = EaseOut((pageTime - slot * 0.06f) / 0.4f);
        Rectangle r{x0 + col * (kCardW + kGapX), y0 + row * (kCardH + kGapY) + (1 - appear) * 24, kCardW, kCardH};
        bool unlocked = Progress::Unlocked(i);
        bool hover = unlocked && Gui::Hover(r);
        Color accent = ThemeAccent(def.theme);
        if (hover) {
            Draw::BlendAdd();
            Draw::GlowStretched({r.x + r.width / 2, r.y + r.height / 2}, 220, 140, 0, WithAlpha(accent, 0.2f));
            Draw::BlendNormal();
        }
        Color panel = unlocked ? Color{14, 20, 44, (unsigned char)(235 * appear)} : Color{28, 30, 38, (unsigned char)(230 * appear)};
        Gui::Panel(r, panel, 14);
        Rectangle pv{r.x + 10, r.y + 10, r.width - 20, 168};
        DrawMapTexture(previews[i], pv);
        if (!unlocked) DrawRectangleRec(pv, {18, 20, 28, 150});
        DrawRectangleLinesEx(pv, 2, WithAlpha(unlocked ? accent : Palette::muted, 0.85f));
        Gui::TextShadow(def.name, r.x + r.width / 2, r.y + 186, 26, unlocked ? Palette::text : Palette::muted, Gui::Align::Center);
        if (unlocked) {
            int stars = Progress::Stars(i);
            for (int s = 0; s < 3; ++s) Gui::StarIcon({r.x + r.width / 2 - 36 + s * 36.0f, r.y + 224}, 12, s < stars);
            if (Gui::Clicked(r)) GoToBattle(i);
        } else {
            Gui::LockIcon({pv.x + pv.width / 2, pv.y + pv.height / 2}, 26, {170, 176, 188, 255});
        }
    }
}

void MenuScene::DrawHeroes() {
    DrawBack();
    if (Gui::Button({W - 250, 28, 210, 44}, codex ? "出战英雄" : "光之档案", Gui::Style::Blue, true, 18)) codex = !codex;
    if (codex) {
        DrawCodex();
        return;
    }
    DrawPageTitle("出战英雄", "当前可以放置的六位光之巨人");
    for (int i = 0; i < kHeroCount; ++i) {
        const HeroDef& def = HeroInfo((HeroId)i);
        Rectangle r{60, 134 + i * 92.0f, 270, 82};
        bool active = heroIndex == i;
        if (Gui::Clicked(r)) heroIndex = i;
        bool hover = Gui::Hover(r);
        Draw::RoundedPanel(r, 12, active ? Mix(def.primary, {10, 14, 30, 255}, 0.55f) : Color{14, 20, 44, 230},
                           active  ? Palette::gold
                           : hover ? Lighten(def.primary, 0.3f)
                                   : Palette::line,
                           active ? 2.5f : 1.5f);
        Art::DrawPortrait((HeroId)i, {r.x + 4, r.y - 2, 84, 84});
        Gui::Text(def.name, r.x + 96, r.y + 16, 20, Palette::text, Gui::Align::Left, true);
        Gui::Text(def.role, r.x + 96, r.y + 46, 14, def.beam);
        Gui::Coin({r.x + r.width - 52, r.y + 26}, 7);
        Gui::Text(TextFormat("%d", def.cost), r.x + r.width - 40, r.y + 17, 16, Palette::gold, Gui::Align::Left, true);
    }
    const HeroDef& def = HeroInfo((HeroId)heroIndex);
    Rectangle panel{360, 130, 870, 560};
    Gui::Panel(panel, {12, 18, 40, 240}, 18);
    float t = RealTime();
    Vector2 c{panel.x + 210, panel.y + 250};
    Draw::BlendAdd();
    Draw::Glow(c, 230, WithAlpha(def.primary, 0.45f));
    Draw::Glow(c, 130, WithAlpha(def.beam, 0.3f));
    for (int i = 0; i < 10; ++i) {
        float a = t * 0.3f + i * PI / 5;
        Draw::Tri(c, Add(c, Mul(Dir(a - 0.04f), 260)), Add(c, Mul(Dir(a + 0.04f), 260)), WithAlpha(def.beam, 0.05f));
    }
    Draw::BlendNormal();
    DrawRing(c, 150, 154, 0, 360, 64, WithAlpha(def.beam, 0.5f));
    float s = 340;
    Art::DrawPortrait((HeroId)heroIndex, {c.x - s / 2, c.y - s / 2 - 10 + std::sin(t * 1.5f) * 4, s, s});
    if (Art::HasCustomAvatar((HeroId)heroIndex))
        Gui::Text("自定义头像", c.x, panel.y + panel.height - 40, 13, Palette::muted, Gui::Align::Center);

    float x = panel.x + 440, y = panel.y + 36, w = panel.width - 470;
    Gui::Text(def.role, x, y, 16, def.beam, Gui::Align::Left, true);
    Gui::TextShadow(def.name, x, y + 24, 46, Palette::text);
    Gui::Text(TextFormat("必杀技 · %s", def.move), x, y + 86, 20, Palette::gold, Gui::Align::Left, true);
    float py = y + 126;
    py += Gui::Paragraph(def.desc, x, py, w, 16, Palette::muted, 1.6f);
    py += 14;
    struct Stat {
        const char* label;
        float value, max;
        std::string text;
    };
    float dps = HeroDamage((HeroId)heroIndex, 0) / def.cooldown;
    Stat stats[] = {
        {"单次伤害", def.damage, 120, TextFormat("%.0f", def.damage)},
        {"每秒伤害", dps, 80, TextFormat("%.0f", dps)},
        {"射程", def.range, 4.6f, TextFormat("%.1f 格", def.range)},
        {"费用", (float)def.cost, 240, TextFormat("%d", def.cost)},
    };
    for (const auto& st : stats) {
        Gui::Text(st.label, x, py, 15, Palette::text);
        Rectangle bar{x + 90, py + 4, w - 150, 10};
        DrawRectangleRounded(bar, 0.5f, 6, {30, 38, 66, 255});
        DrawRectangleRounded({bar.x, bar.y, bar.width * Clamp(st.value / st.max, 0.05f, 1.0f), bar.height}, 0.5f, 6, def.beam);
        Gui::Text(st.text, x + w, py, 15, Palette::text, Gui::Align::Right, true);
        py += 30;
    }
    py += 8;
    Draw::RoundedPanel({x - 10, py, w + 20, 48}, 10, {40, 30, 10, 200}, WithAlpha(Palette::gold, 0.6f), 1.5f);
    Gui::Paragraph(def.perk, x + 4, py + 14, w - 8, 15, Palette::gold);
}

void MenuScene::DrawCodex() {
    DrawPageTitle("奥特曼数据库", "招式名按中文常用译名，游戏里的攻击方式是改编");
    const auto& entries = UltraCodex();
    const auto& errors = UltraErrors();
    if (!errors.empty()) Gui::Text("档案有误：" + errors[0], W / 2, 112, 14, Palette::red, Gui::Align::Center);
    if (entries.empty()) {
        Gui::Text("还没有可读的档案", W / 2, 360, 22, Palette::muted, Gui::Align::Center);
        return;
    }
    codexIndex = std::clamp(codexIndex, 0, (int)entries.size() - 1);
    constexpr int kVisible = 8;
    int page = codexIndex / kVisible;
    int first = page * kVisible;
    for (int i = first; i < std::min(first + kVisible, (int)entries.size()); ++i) {
        const UltraEntry& entry = entries[i];
        Rectangle r{48, 132 + (i - first) * 68.0f, 360, 62};
        bool active = codexIndex == i;
        if (Gui::Clicked(r)) codexIndex = i;
        Draw::RoundedPanel(r, 10, active ? Color{46, 28, 48, 235} : Color{14, 20, 44, 220},
                           active          ? Palette::gold
                           : Gui::Hover(r) ? Palette::muted
                                           : Palette::line,
                           active ? 2.2f : 1.4f);
        Gui::Text(entry.name, r.x + 14, r.y + 8, 18, Palette::text, Gui::Align::Left, true);
        Gui::Text(entry.move, r.x + 14, r.y + 34, 14, Palette::gold);
    }
    int pages = ((int)entries.size() + kVisible - 1) / kVisible;
    if (pages > 1) {
        if (Gui::Button({48, 682, 110, 30}, "上一页", Gui::Style::Dark, page > 0, 15)) codexIndex = (page - 1) * kVisible;
        if (Gui::Button({168, 682, 110, 30}, "下一页", Gui::Style::Dark, page + 1 < pages, 15))
            codexIndex = std::min((page + 1) * kVisible, (int)entries.size() - 1);
    }
    const UltraEntry& entry = entries[codexIndex];
    Rectangle panel{430, 132, 800, 540};
    Gui::Panel(panel, {12, 18, 40, 240}, 18);
    HeroId hero = HeroId::Count;
    for (int i = 0; i < kHeroCount; ++i)
        if (entry.id == HeroInfo((HeroId)i).key) hero = (HeroId)i;
    if (hero != HeroId::Count) {
        const HeroDef& def = HeroInfo(hero);
        Draw::BlendAdd();
        Draw::Glow({panel.x + 150, panel.y + 210}, 180, WithAlpha(def.primary, 0.4f));
        Draw::BlendNormal();
        Art::DrawPortrait(hero, {panel.x + 30, panel.y + 70, 250, 250});
    }
    float x = panel.x + (hero != HeroId::Count ? 300 : 36);
    float w = panel.width - (x - panel.x) - 28;
    Gui::Text(entry.title, x, panel.y + 28, 16, Palette::cyan, Gui::Align::Left, true);
    Gui::TextShadow(entry.name, x, panel.y + 52, 40, Palette::text);
    Gui::Text(TextFormat("必杀 · %s", entry.move.c_str()), x, panel.y + 112, 20, Palette::gold, Gui::Align::Left, true);
    float py = panel.y + 156;
    py += Gui::Paragraph(entry.desc, x, py, w, 17, Palette::muted, 1.6f);
    py += 18;
    Draw::RoundedPanel({x - 8, py, w + 16, 64}, 10, {36, 28, 12, 210}, WithAlpha(Palette::gold, 0.55f), 1.4f);
    Gui::Paragraph(entry.note, x, py + 12, w, 15, Palette::gold, 1.45f);
}

void MenuScene::DrawKaiju() {
    DrawBack();
    DrawPageTitle("怪兽图鉴", "了解敌人的特性，才能制定最佳防守策略");
    float t = RealTime();
    float cw = 150, gap = 12;
    float start = W / 2 - (kEnemyCount * cw + (kEnemyCount - 1) * gap) / 2;
    for (int i = 0; i < kEnemyCount; ++i) {
        const EnemyDef& def = EnemyInfo((EnemyId)i);
        Rectangle r{start + i * (cw + gap), 128, cw, 160};
        bool active = kaijuIndex == i;
        if (Gui::Clicked(r)) kaijuIndex = i;
        Draw::RoundedPanel(r, 12, active ? Color{50, 24, 40, 240} : Color{14, 20, 44, 230},
                           active          ? Palette::red
                           : Gui::Hover(r) ? Palette::muted
                                           : Palette::line,
                           active ? 2.5f : 1.5f);
        Draw::GlowStretched({r.x + cw / 2, r.y + 112}, 46, 10, 0, WithAlpha(BLACK, 0.5f));
        Art::KaijuPose pose;
        pose.feet = {r.x + cw / 2, r.y + 112};
        pose.scale = 1.7f / std::max(1.0f, def.size * 0.85f);
        pose.time = t + i;
        Art::DrawKaiju((EnemyId)i, pose);
        Gui::Text(def.name, r.x + cw / 2, r.y + 124, 16, Palette::text, Gui::Align::Center, true);
        if (def.boss) Gui::Text("BOSS", r.x + cw - 10, r.y + 8, 12, Palette::red, Gui::Align::Right, true);
    }
    const EnemyDef& def = EnemyInfo((EnemyId)kaijuIndex);
    Rectangle panel{50, 306, W - 100, 384};
    Gui::Panel(panel, {12, 18, 40, 240}, 18);
    Vector2 feet{panel.x + 240, panel.y + 330};
    Draw::BlendAdd();
    Draw::Glow({feet.x, feet.y - 120}, 220, WithAlpha(def.boss ? Color{255, 60, 80, 255} : Color{255, 160, 80, 255}, 0.25f));
    Draw::BlendNormal();
    Draw::GlowStretched(feet, 110, 22, 0, WithAlpha(BLACK, 0.55f));
    Art::KaijuPose pose;
    pose.feet = feet;
    pose.scale = 4.6f / std::max(1.0f, def.size * 0.75f);
    pose.time = t;
    pose.shielded = kaijuIndex == (int)EnemyId::Zetton && std::fmod(t, 6.0f) > 4;
    pose.enraged = kaijuIndex == (int)EnemyId::Belial && std::fmod(t, 6.0f) > 4;
    Art::DrawKaiju((EnemyId)kaijuIndex, pose);

    float x = panel.x + 500, y = panel.y + 30, w = panel.width - 540;
    Gui::Text(def.title, x, y, 17, Palette::red, Gui::Align::Left, true);
    Gui::TextShadow(def.name, x, y + 24, 46, Palette::text);
    float py = y + 96;
    py += Gui::Paragraph(def.desc, x, py, w, 17, Palette::muted, 1.6f);
    py += 18;
    const char* speed = def.speed >= 1.7f ? "极快" : def.speed >= 1.3f ? "快" : def.speed >= 0.9f ? "中等" : "缓慢";
    struct Row {
        const char* label;
        std::string value;
    };
    Row rows[] = {
        {"基础生命", TextFormat("%.0f", def.hp)},      {"移动速度", speed},
        {"护甲", TextFormat("%.0f", def.armor)},       {"击败奖励", TextFormat("%d 金币", def.reward)},
        {"突破伤害", TextFormat("%d 生命", def.leak)}, {"减速抗性", TextFormat("%.0f%%", def.slowResist * 100)},
    };
    for (int i = 0; i < 6; ++i) {
        float rx = x + (i % 3) * (w / 3), ry = py + (i / 3) * 62;
        Draw::RoundedPanel({rx, ry, w / 3 - 12, 52}, 10, {22, 30, 60, 230}, Palette::line, 1);
        Gui::Text(rows[i].label, rx + 12, ry + 6, 13, Palette::muted);
        Gui::Text(rows[i].value, rx + 12, ry + 24, 19, Palette::text, Gui::Align::Left, true);
    }
}

void MenuScene::DrawHelp() {
    DrawBack();
    DrawPageTitle("操作说明", "成为优秀的防卫队指挥官");
    Rectangle panel{90, 130, W - 180, 560};
    Gui::Panel(panel, {12, 18, 40, 240}, 18);
    struct Section {
        const char* title;
        const char* body;
    };
    Section sections[] = {
        {"基本玩法", "怪兽会沿着道路冲向基地，每有一只怪兽抵达基地就会损失生命。在道路两侧的空地上"
                     "放置奥特曼英雄来消灭它们，"
                     "撑过所有波次即可获胜。剩余生命越多，获得的星星越多。"},
        {"放置与升级", "点击右侧英雄卡片或按数字键 1-6 选择英雄，再点击地图空地放置；按住 Shift "
                       "可连续放置。点击已放置的英雄"
                       "可升级（最高三级，三级解锁特殊能力）、出售（返还 70%）或切换攻击目标：最前 "
                       "/ 最强 / 最近。"},
        {"波次", "按空格或点击「开始进攻」召唤第一波。之后每波结束会自动倒计时，提前召唤可获得额外金币，"
                 "每开启新一波还会获得波次奖励。"},
        {"必杀技", "Q：奥特兄弟合体光线，对全场造成伤害。W：喷气甲虫号空中支援，选区轰炸并眩晕。E：究极合体。"
                   "六位奥特曼全部上场后，右侧「究极合体」才会点亮，伤害极高。"},
        {"快捷键", "F / Tab 切换 1x-3x 倍速    P / Esc 暂停    U 升级    T 切换目标    Delete 出售 "
                   "   右键 取消    F11 全屏"},
    };
    float y = panel.y + 28;
    for (const auto& s : sections) {
        Draw::Diamond({panel.x + 40, y + 11}, 6, 8, Palette::gold);
        Gui::Text(s.title, panel.x + 58, y, 21, Palette::gold, Gui::Align::Left, true);
        y += 32;
        y += Gui::Paragraph(s.body, panel.x + 58, y, panel.width - 110, 16, Palette::text, 1.55f);
        y += 14;
    }
    std::string keys;
    for (int i = 0; i < kHeroCount; ++i) keys += std::string(i ? "、" : "") + HeroInfo((HeroId)i).key;
    Gui::Paragraph("自定义头像：将正方形 PNG 图片放入游戏目录 assets/avatars/ 下，文件名为 " + keys +
                       "（.png），重启游戏即可替换对应英雄的头像。",
                   panel.x + 58, panel.y + panel.height - 58, panel.width - 110, 13, Palette::muted, 1.5f);
}

} // namespace UG
