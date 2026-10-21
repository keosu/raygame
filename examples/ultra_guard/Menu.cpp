#include "Menu.h"
#include "Art.h"
#include "Assets.h"
#include "Battle.h"
#include "Draw.h"
#include "Gui.h"
#include "Skins.h"
#include <rlgl.h>

namespace UG {

namespace {
int heroIndexOnLoad = 0;
int prepareOnLoad = 0;
} // namespace

void PreviewHero(int hero) { heroIndexOnLoad = hero; }

void PrepareLevel(int level) { prepareOnLoad = level; }

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
    if (start == MenuPage::Heroes) heroIndex = std::clamp(heroIndexOnLoad, 0, kHeroCount - 1);

    if (start == MenuPage::Prepare) OpenPrepare(prepareOnLoad);
}

void MenuScene::OpenPrepare(int level) {
    prepareLevel = level;
    const LevelDef* def = PreparedLevel();
    if (!def) {
        page = MenuPage::Levels;
        return;
    }
    picks = Progress::Roster(*def);
    pickWarning = 0;
    SetPage(MenuPage::Prepare);
}

const LevelDef* MenuScene::PreparedLevel() const {
    if (prepareLevel == kSurvivalIndex) return SurvivalLevel();
    return prepareLevel >= 0 && prepareLevel < (int)Levels().size() ? &Levels()[prepareLevel] : nullptr;
}

bool MenuScene::LineupReady() const {
    const LevelDef* def = PreparedLevel();
    return def && picks.size() == def->heroes.size();
}

void MenuScene::Launch() {
    if (!LineupReady()) return;
    Progress::SetRoster(*PreparedLevel(), picks);
    GoToBattle(prepareLevel);
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
    if (!survivalPreview.id && SurvivalLevel()) {
        survivalMap.Build(*SurvivalLevel());
        survivalPreview = RenderMap(survivalMap, 0.5f);
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
    if (survivalPreview.id) UnloadRenderTexture(survivalPreview);
    survivalPreview = {};
    DropPreview();
}

// The database page runs one demo battle for the selected hero. It is created and stepped
// from Update because loading bakes a map texture, which cannot happen mid-draw.
void MenuScene::SyncPreview(float dt) {
    if (page != MenuPage::Heroes) return DropPreview();
    if (!preview || previewHero != heroIndex) {
        DropPreview();
        preview = CreateRef<BattleScene>((HeroId)heroIndex);
        preview->embedded = true;
        preview->Load();
        previewHero = heroIndex;
        Sfx::SetEffectScale(0.35f);
    }
    preview->TickEmbedded(dt);
}

void MenuScene::DropPreview() {
    if (!preview) return;
    preview->ReleaseEmbedded();
    preview.reset();
    previewHero = -1;
    Sfx::SetEffectScale(1);
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
    pickWarning = std::max(0.0f, pickWarning - dt);
    bool confirm = Input::GetKeyDown(KEY_ENTER) || Input::GetKeyDown(KEY_SPACE);
    if (Input::GetKeyDown(KEY_ESCAPE) || Input::GetKeyDown(KEY_BACK)) {
        if (page == MenuPage::Title) QuitGame();
        else if (page == MenuPage::Prepare && prepareLevel != kSurvivalIndex) SetPage(MenuPage::Levels);
        else SetPage(MenuPage::Title);
    } else if (page == MenuPage::Title && confirm) {
        SetPage(MenuPage::Levels);
    } else if (page == MenuPage::Prepare && confirm) {
        Launch();
    }
    if (page == MenuPage::Heroes) {
        if (Input::GetKeyDown(KEY_DOWN) || Input::GetKeyDown(KEY_S)) heroIndex = (heroIndex + 1) % kHeroCount;
        if (Input::GetKeyDown(KEY_UP) || Input::GetKeyDown(KEY_W)) heroIndex = (heroIndex + kHeroCount - 1) % kHeroCount;
    }
    SyncPreview(dt);
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
    case MenuPage::Prepare:
        DrawPrepare();
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

    // Hero lineup: the 2026 popularity top ten, in ranking order.
    static const HeroId kTopTen[] = {HeroId::Tiga,  HeroId::Mebius, HeroId::Leo,  HeroId::Cosmos, HeroId::Seven,
                                     HeroId::Ginga, HeroId::Gaia,   HeroId::Zero, HeroId::Nexus,  HeroId::Ultraman};
    constexpr int kShown = (int)std::size(kTopTen);
    float gap = 12;
    float size = std::min(150.0f, (W - 70 - (kShown - 1) * gap) / kShown);
    float total = kShown * size + (kShown - 1) * gap;
    for (int i = 0; i < kShown; ++i) {
        HeroId id = kTopTen[i];
        const HeroDef& def = HeroInfo(id);
        float appear = EaseOut((pageTime - 0.15f - i * 0.06f) / 0.5f);
        if (appear <= 0) continue;
        float arc = std::fabs(i - (kShown - 1) * 0.5f) * 5.0f / (kShown - 1);
        float x = W / 2 - total / 2 + i * (size + gap);
        float y = 226 + arc * arc * 5 + std::sin(t * 1.6f + i) * 5 + (1 - appear) * 60;
        Vector2 c{x + size / 2, y + size * 0.62f};
        Draw::BlendAdd();
        Draw::Glow(c, size * 0.7f, WithAlpha(def.primary, 0.45f * appear));
        Draw::Glow(c, size * 0.38f, WithAlpha(def.beam, 0.25f * appear));
        Draw::BlendNormal();
        Draw::GlowStretched({c.x, y + size + 4}, size * 0.5f, 12, 0, WithAlpha(BLACK, 0.5f * appear));
        Art::DrawPortrait(id, {x, y, size, size}, WithAlpha(WHITE, appear));
        std::string name = def.name;
        const std::string suffix = "奥特曼";
        if (name.size() > suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
            name.resize(name.size() - suffix.size());
        Gui::TextShadow(name, c.x, y + size + 6, 15, WithAlpha(Palette::text, appear), Gui::Align::Center);
        Gui::Text(TextFormat("%d", i + 1), x + 8, y + 4, 14, WithAlpha(i < 3 ? Palette::gold : Palette::muted, appear), Gui::Align::Left, true);
    }
    Gui::Text("2026 奥特曼人气榜 TOP 10", W / 2, 208, 13, WithAlpha(Palette::muted, a), Gui::Align::Center);

    float by = 470 + (1 - a) * 30;
    if (Gui::Button({W / 2 - 300, by, 290, 60}, "开始作战", Gui::Style::Gold, true, 26)) SetPage(MenuPage::Levels);
    if (Gui::Button({W / 2 + 10, by, 290, 60}, "生存模式", Gui::Style::Red, SurvivalLevel() != nullptr, 26)) OpenPrepare(kSurvivalIndex);
    if (Gui::Button({W / 2 - 300, by + 76, 190, 46}, "奥特数据库", Gui::Style::Blue, true, 19)) SetPage(MenuPage::Heroes);
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
    float best = Progress::SurvivalBestTime();
    Gui::Text(TextFormat("已获得星星 %d / %d     累计击败怪兽 %d     生存最佳 %02d:%02d", stars, (int)Levels().size() * 3, Progress::TotalKills(),
                         (int)best / 60, (int)best % 60),
              W / 2, Hh - 50, 15, Palette::muted, Gui::Align::Center);
    Gui::Text("Enter 开始   F11 全屏   Esc 退出", W / 2, Hh - 28, 12, WithAlpha(Palette::muted, 0.7f), Gui::Align::Center);
}

void MenuScene::DrawLevels() {
    DrawBack();
    DrawPageTitle("选择关卡", "点选缩略图，挑好出战阵容后出击");
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
    constexpr float kCardW = 390, kCardH = 240, kGapX = 18, kGapY = 34;
    float gridW = kColsOnPage * kCardW + (kColsOnPage - 1) * kGapX;
    float x0 = (W - gridW) / 2;
    float y0 = 140;
    for (int i = first; i < first + shown; ++i) {
        const LevelDef& def = levels[i];
        int slot = i - first;
        int col = slot % kColsOnPage, row = slot / kColsOnPage;
        if (!def.arc.empty() && (col == 0 || levels[i - 1].arc != def.arc)) {
            // Arc header over the row (or from the card where a new arc starts mid-row).
            float hx = x0 + col * (kCardW + kGapX), hy = y0 + row * (kCardH + kGapY) - 26;
            Draw::Diamond({hx + 6, hy + 10}, 5, 7, Palette::gold);
            Gui::Text(def.arc, hx + 18, hy, 18, Palette::gold, Gui::Align::Left, true);
            DrawRectangleRec({hx + 120, hy + 10, x0 + gridW - hx - 120, 1.5f}, WithAlpha(Palette::gold, 0.35f));
        }
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
        Rectangle badge{pv.x + 6, pv.y + 6, 52, 26};
        Draw::RoundedPanel(badge, 7, {8, 10, 24, 220}, WithAlpha(unlocked ? accent : Palette::muted, 0.9f), 1.5f);
        Gui::Text(def.tag, badge.x + badge.width / 2, badge.y + 3, 16, unlocked ? Palette::text : Palette::muted, Gui::Align::Center, true);
        if (def.assault > 0) {
            Rectangle warn{pv.x + pv.width - 112, pv.y + 6, 106, 26};
            Draw::RoundedPanel(warn, 7, {70, 10, 18, 220}, WithAlpha(Palette::red, 0.9f), 1.5f);
            Gui::Text("怪兽会反击", warn.x + warn.width / 2, warn.y + 4, 14, {255, 200, 200, 255}, Gui::Align::Center, true);
        }
        Gui::TextShadow(def.name, r.x + r.width / 2, r.y + 186, 26, unlocked ? Palette::text : Palette::muted, Gui::Align::Center);
        if (unlocked) {
            int stars = Progress::Stars(i);
            for (int s = 0; s < 3; ++s) Gui::StarIcon({r.x + r.width / 2 - 36 + s * 36.0f, r.y + 220}, 12, s < stars);
            if (Gui::Clicked(r)) OpenPrepare(i);
        } else {
            Gui::LockIcon({pv.x + pv.width / 2, pv.y + pv.height / 2}, 26, {170, 176, 188, 255});
        }
    }
}

void MenuScene::DrawPrepare() {
    const LevelDef* def = PreparedLevel();
    if (!def) return;
    bool survival = prepareLevel == kSurvivalIndex;
    int need = (int)def->heroes.size();
    if (Gui::Button({30, 28, 120, 44}, "返回", Gui::Style::Dark, true, 18)) SetPage(survival ? MenuPage::Title : MenuPage::Levels);
    DrawPageTitle("出战准备", TextFormat("选择 %d 位光之巨人，卡片顺序就是战斗中的 1-%d 号键", need, need));

    // Mission briefing
    Rectangle info{40, 128, 420, 562};
    Gui::Panel(info, {12, 18, 40, 240}, 16);
    Color accent = ThemeAccent(def->theme);
    Rectangle pv{info.x + 14, info.y + 14, info.width - 28, (info.width - 28) * 0.6f};
    DrawMapTexture(survival ? survivalPreview : previews[prepareLevel], pv);
    DrawRectangleLinesEx(pv, 2, WithAlpha(accent, 0.85f));
    float y = pv.y + pv.height + 12;
    Gui::Text(survival ? "生存模式" : def->tag + "  " + def->arc, info.x + 18, y, 15, accent, Gui::Align::Left, true);
    Gui::TextShadow(def->name, info.x + 18, y + 22, 30, Palette::text);
    y += 66;
    y += Gui::Paragraph(def->desc, info.x + 18, y, info.width - 36, 14, Palette::muted, 1.55f);
    y += 10;
    struct Row {
        const char* label;
        std::string value;
        Color tone;
    };
    std::vector<Row> rows;
    if (survival) {
        float best = Progress::SurvivalBestTime();
        rows.push_back({"波次", "无尽，越来越强", Palette::text});
        if (def->siege > 0) rows.push_back({"灯塔耐久", TextFormat("%.0f · 每波修复 5%%", def->siege), Palette::cyan});
        rows.push_back({"最佳纪录", TextFormat("%02d:%02d · 第 %d 波", (int)best / 60, (int)best % 60, Progress::SurvivalBestWave()), Palette::gold});
        rows.push_back({"怪兽反击", "第 3 波起，逐波增强", Palette::red});
    } else {
        rows.push_back({"波次", TextFormat("%d 波", (int)def->waves.size()), Palette::text});
        rows.push_back({"初始金币", TextFormat("%d", def->gold), Palette::gold});
        rows.push_back({"怪兽反击", def->assault > 0 ? TextFormat("强度 %.0f%%", def->assault * 100) : "无", def->assault > 0 ? Palette::red : Palette::muted});
    }
    for (const auto& row : rows) {
        Gui::Text(row.label, info.x + 18, y, 15, Palette::muted);
        Gui::Text(row.value, info.x + info.width - 18, y, 15, row.tone, Gui::Align::Right, true);
        y += 26;
    }
    if (def->assault > 0)
        Gui::Paragraph("怪兽会用光线、岩石和火球攻击附近的英雄。能量耗尽的英雄倒下 8 秒，脱离攻击后能量会慢慢恢复，升级可以立刻补满。",
                       info.x + 18, std::min(y + 4, info.y + info.height - 64), info.width - 36, 12.5f, {255, 190, 180, 255}, 1.5f);

    // Hero pool: five compact cards per row (room for twenty), picked heroes show their card number.
    float t = RealTime();
    constexpr int kPoolCols = 5;
    constexpr float kPoolW = 755, kPoolH = 418, kGap = 7;
    const int poolRows = (kHeroCount + kPoolCols - 1) / kPoolCols;
    const float kCardW = (kPoolW - (kPoolCols - 1) * kGap) / kPoolCols;
    const float kCardH = std::min(132.0f, (kPoolH - (poolRows - 1) * kGap) / poolRows);
    float gx = 485, gy = 128;
    for (int i = 0; i < kHeroCount; ++i) {
        HeroId id = (HeroId)i;
        const HeroDef& hero = HeroInfo(id);
        Rectangle r{gx + (i % kPoolCols) * (kCardW + kGap), gy + (i / kPoolCols) * (kCardH + kGap), kCardW, kCardH};
        auto at = std::find(picks.begin(), picks.end(), id);
        bool picked = at != picks.end();
        bool hover = Gui::Hover(r);
        bool recommended = std::find(def->heroes.begin(), def->heroes.end(), id) != def->heroes.end();
        Color fill = picked ? Mix(hero.primary, {10, 14, 30, 255}, 0.55f) : Color{14, 20, 44, 230};
        Color border = picked ? Palette::gold : hover ? Lighten(hero.primary, 0.4f) : Palette::line;
        if (picked) {
            Draw::BlendAdd();
            Draw::GlowStretched({r.x + r.width / 2, r.y + r.height / 2}, 140, 80, 0, WithAlpha(hero.beam, 0.12f + 0.05f * std::sin(t * 4 + i)));
            Draw::BlendNormal();
        }
        Draw::RoundedPanel(r, 10, fill, border, picked ? 2.4f : 1.2f);
        float face = kCardH - 36;
        Art::DrawPortrait(id, {r.x + r.width / 2 - face / 2, r.y + 2, face, face}, picked || hover ? WHITE : Color{200, 204, 216, 255});
        Gui::Text(hero.name, r.x + r.width / 2, r.y + kCardH - 36, 13, Palette::text, Gui::Align::Center, true);
        Gui::Text(hero.role, r.x + 8, r.y + kCardH - 17, 11, hero.beam, Gui::Align::Left, true);
        Gui::Coin({r.x + r.width - 38, r.y + kCardH - 10}, 5);
        Gui::Text(TextFormat("%d", hero.cost), r.x + r.width - 8, r.y + kCardH - 18, 12, Palette::gold, Gui::Align::Right, true);
        if (recommended) Gui::Text("推荐", r.x + 8, r.y + 6, 11, Palette::cyan, Gui::Align::Left, true);
        if (picked) {
            Vector2 c{r.x + r.width - 16, r.y + 16};
            DrawCircleV(c, 11, Palette::gold);
            Gui::Text(TextFormat("%d", (int)(at - picks.begin()) + 1), c.x, c.y - 8, 14, {50, 24, 0, 255}, Gui::Align::Center, true);
        }
        if (hover)
            Gui::Text(TextFormat("%s · %s", hero.name, hero.move), gx, gy + kPoolH + 3, 13, Palette::gold, Gui::Align::Left, true);
        if (Gui::Clicked(r)) {
            if (picked) {
                picks.erase(at);
                Sfx::Play("sell", 0.4f);
            } else if ((int)picks.size() < need) {
                picks.push_back(id);
                Sfx::Play("build", 0.4f);
            } else {
                pickWarning = 1.6f;
                Sfx::Play("error", 0.5f);
            }
        }
    }

    // Lineup strip in card order; clicking a slot removes that hero.
    Rectangle strip{gx, 568, kPoolW, 122};
    Gui::Panel(strip, {12, 18, 40, 240}, 14);
    Gui::Text(TextFormat("出战阵容  %d / %d", (int)picks.size(), need), strip.x + 16, strip.y + 10, 16,
              LineupReady() ? Palette::gold : Palette::text, Gui::Align::Left, true);
    if (pickWarning > 0)
        Gui::Text("阵容已满，先点掉一位再换", strip.x + 200, strip.y + 11, 14, WithAlpha(Palette::red, std::min(1.0f, pickWarning)));
    const float slot = 62, slotGap = 10;
    for (int i = 0; i < need; ++i) {
        Rectangle s{strip.x + 16 + i * (slot + slotGap), strip.y + 40, slot, slot};
        bool filled = i < (int)picks.size();
        Draw::RoundedPanel(s, 10, filled ? Mix(HeroInfo(picks[i]).primary, {10, 14, 30, 255}, 0.5f) : Color{8, 10, 22, 220},
                           filled ? WithAlpha(HeroInfo(picks[i]).beam, 0.9f) : Palette::line, 1.5f);
        Gui::Text(TextFormat("%d", i + 1), s.x + 9, s.y + 3, 12, Palette::muted, Gui::Align::Center, true);
        if (!filled) continue;
        Art::DrawPortrait(picks[i], {s.x - 4, s.y - 6, slot + 8, slot + 8});
        if (Gui::Clicked(s)) {
            picks.erase(picks.begin() + i);
            Sfx::Play("sell", 0.4f);
            break;
        }
    }
    float bx = strip.x + strip.width - 214;
    if (Gui::Button({bx, strip.y + 14, 196, 40}, "推荐阵容", Gui::Style::Dark, picks != def->heroes, 16)) picks = def->heroes;
    if (Gui::Button({bx, strip.y + 64, 196, 46}, survival ? "开始生存" : "出击", Gui::Style::Gold, LineupReady(), 20)) Launch();
}

void MenuScene::DrawHeroes() {
    DrawBack();
    DrawPageTitle("奥特曼数据库", "可上阵的光之巨人，每关阵容不同");
    // Two columns so the roster can grow to twenty.
    const int perColumn = (kHeroCount + 1) / 2;
    float rowH = std::min(92.0f, 566.0f / perColumn);
    for (int i = 0; i < kHeroCount; ++i) {
        const HeroDef& def = HeroInfo((HeroId)i);
        Rectangle r{30 + (i / perColumn) * 180.0f, 128 + (i % perColumn) * rowH, 174, rowH - 5};
        bool active = heroIndex == i;
        if (Gui::Clicked(r)) heroIndex = i;
        bool hover = Gui::Hover(r);
        Draw::RoundedPanel(r, 10, active ? Mix(def.primary, {10, 14, 30, 255}, 0.55f) : Color{14, 20, 44, 230},
                           active  ? Palette::gold
                           : hover ? Lighten(def.primary, 0.3f)
                                   : Palette::line,
                           active ? 2.5f : 1.3f);
        float s = r.height + 2;
        Art::DrawPortrait((HeroId)i, {r.x + 2, r.y - 2, s, s});
        Gui::Text(def.name, r.x + s + 6, r.y + r.height * 0.5f - 17, 14, Palette::text, Gui::Align::Left, true);
        Gui::Text(def.role, r.x + s + 6, r.y + r.height * 0.5f + 2, 11.5f, def.beam);
    }
    const HeroDef& def = HeroInfo((HeroId)heroIndex);
    Rectangle panel{396, 128, 856, 562};
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
    // The hero's real attack effects play over the portrait, fired at invisible passing kaiju.
    if (preview && previewHero == heroIndex)
        preview->DrawEmbedded({panel.x + 6, panel.y + 6, 428, panel.height - 12}, {c.x - 20, c.y - 10});
    if (Art::HasCustomAvatar((HeroId)heroIndex)) Gui::Text("自定义头像", panel.x + 20, panel.y + 16, 13, Palette::muted);

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
    if (def.kind == AttackKind::Field) dps *= 3.5f; // the field ticks its damage every second for 3.5 s
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

void MenuScene::DrawKaiju() {
    DrawBack();
    DrawPageTitle("怪兽图鉴", "了解敌人的特性，才能制定最佳防守策略");
    float t = RealTime();
    float gap = 10;
    float cw = std::min(150.0f, (W - 60 - (kEnemyCount - 1) * gap) / kEnemyCount);
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
        pose.scale = 1.7f / std::max(1.0f, def.size * 0.85f) * std::min(1.0f, cw / 120);
        pose.time = t + i;
        Art::DrawKaiju((EnemyId)i, pose);
        Gui::Text(def.name, r.x + cw / 2, r.y + 124, cw < 120 ? 13.0f : 16.0f, Palette::text, Gui::Align::Center, true);
        if (def.boss) Gui::Text("BOSS", r.x + cw - 8, r.y + 8, 11, Palette::red, Gui::Align::Right, true);
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
    Rectangle panel{90, 116, W - 180, 584};
    Gui::Panel(panel, {12, 18, 40, 240}, 18);
    struct Section {
        const char* title;
        const char* body;
    };
    Section sections[] = {
        {"基本玩法", "怪兽沿道路冲向基地，每有一只抵达就会损失生命。在道路两侧的空地上放置奥特曼英雄消灭它们，"
                     "撑过所有波次即可获胜，剩余生命越多星星越多。出击前可以在「出战准备」里挑选本关的阵容。"},
        {"放置与升级", "点击右侧英雄卡片或按数字键选择英雄，再点击空地放置，按住 Shift 可连续放置。点击已放置的英雄可升级"
                       "（最高三级，三级解锁特殊能力）、出售（返还 70%）或切换攻击目标：最前 / 最强 / 最近。"},
        {"波次与反击", "按空格或「开始进攻」召唤第一波，之后自动倒计时，提前召唤有额外金币。后期关卡的怪兽会攻击附近英雄，"
                       "能量耗尽的英雄倒下 8 秒，升级可以立刻补满能量。"},
        {"必杀技", "Q：奥特兄弟合体光线，全场伤害。W：喷气甲虫号空中支援，选区轰炸并眩晕。E：究极合体，"
                   "阵容全部上场后蓄能约 25 秒即可发动，冷却 75 秒。"},
        {"生存模式 · 守城", "光之城堡四面各有一座城门，怪兽从四个方向攻进庭院，围攻正中央的守护灯塔。英雄可以站上城墙，"
                            "灯塔会放光束反击，每波修复少许，耐久归零即结束，看你能守多久。"},
        {"技能演示", "在「奥特数据库」里选中一位奥特曼，头像上会循环播放他的招式粒子效果。"},
        {"快捷键", "F / Tab 倍速    P / Esc 暂停    U 升级    T 切换目标    Delete 出售    右键 取消    F11 全屏"},
    };
    float y = panel.y + 22;
    for (const auto& s : sections) {
        Draw::Diamond({panel.x + 40, y + 10}, 5, 7, Palette::gold);
        Gui::Text(s.title, panel.x + 58, y, 18, Palette::gold, Gui::Align::Left, true);
        y += 26;
        y += Gui::Paragraph(s.body, panel.x + 58, y, panel.width - 110, 14, Palette::text, 1.45f);
        y += 9;
    }
    std::string keys;
    for (int i = 0; i < kHeroCount; ++i) keys += std::string(i ? "、" : "") + HeroInfo((HeroId)i).key;
    Gui::Paragraph("自定义头像：将正方形 PNG 图片放入游戏目录 assets/avatars/ 下，文件名为 " + keys +
                       "（.png），重启游戏即可替换对应英雄的头像。",
                   panel.x + 58, panel.y + panel.height - 50, panel.width - 110, 12.5f, Palette::muted, 1.45f);
}

} // namespace UG
