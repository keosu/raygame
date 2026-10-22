#pragma once
#include "Model.h"
#include "../common/CampaignGame.h"

namespace Marble {
using Campaign::UI;
inline constexpr Color Colors[] = {{226, 83, 72, 255},
                                   {243, 181, 61, 255},
                                   {74, 172, 123, 255},
                                   {65, 148, 207, 255},
                                   {164, 102, 196, 255}};
// 0 → 1 with a small overshoot, for marbles popping into view.
inline float PopIn(float t) {
    t = std::clamp(t, 0.0f, 1.0f) - 1;
    return 1 + 2.70158f * t * t * t + 1.70158f * t * t;
}
inline std::array<RayGame::TextureHandle, 5> BallTextures;
inline RayGame::TextureHandle BallShadow;
inline void DrawBall(Vector2 p, int color, float radius = 17, bool blast = false) {
    float extent = radius * 1.12f;
    if (BallShadow)
        DrawTexturePro(*BallShadow, {0, 0, 128, 128},
                       {p.x - radius * 1.4f, p.y - radius * 0.6f, radius * 2.8f, radius * 1.8f}, {},
                       0, WHITE);
    if (BallTextures[color])
        DrawTexturePro(*BallTextures[color], {0, 0, 256, 256},
                       {p.x - extent, p.y - extent, extent * 2, extent * 2}, {}, 0, WHITE);
    else
        DrawCircleV(p, radius, Colors[color]);
    if (blast)
        DrawRing(p, radius + 3, radius + 4.5f, 0, 360, 96, Fade(WHITE, 0.9f));
}
class Game;
class World : public RayGame::Component {
    Game* game;

public:
    explicit World(Game* owner) : game(owner) {}
    void FixedUpdate() override;
    void Render() override;
};
class Game : public Campaign::Game {
public:
    static constexpr const char* AppId = "raygame.marble_temple";
    Model model;
    std::array<RayGame::TextureHandle, 3> backdrops;
    RayGame::TextureHandle launcherTexture;
    int heardShot = 0, heardClear = 0;
    float ammoChange = 1; // 0..1 after a swap or a new next marble, for the pop-in
    int shownNext = -1;
    explicit Game(std::filesystem::path directory)
        : Campaign::Game(
              "marble_temple", "MARBLE TEMPLE", "Aim. Match three. Keep the ancient gate sealed.",
              {"Mossy Courtyard", "Moonlit Ruins", "The Spiral Vault"}, std::move(directory)) {
        hint = "Swap colours to reach pairs. Every ninth shot is a blast.";
        accent = {232, 186, 92, 255};
    }
    void DrawEmblem(Vector2 c, float time) override {
        DrawRing(c, 50, 54, 0, 360, 96, Fade(accent, 0.6f));
        int colors[] = {0, 3, 2};
        for (int i = 0; i < 3; ++i) {
            float a = -PI / 2 + i * 2 * PI / 3 + std::sin(time * 0.8f) * 0.08f;
            DrawBall({c.x + std::cos(a) * 27, c.y + std::sin(a) * 27}, colors[i], 22);
        }
    }
    void DrawLevelArt(int stage, Rectangle area, float time) override {
        if (backdrops[stage]) { // centre crop of the stage's own temple backdrop
            float w = 1440 * area.width / area.height;
            DrawTexturePro(*backdrops[stage], {(2560 - w) / 2, 0, w, 1440}, area, {}, 0, WHITE);
        }
        for (int i = 0; i < 6; ++i) {
            float t = i / 5.0f;
            Vector2 p{area.x + 30 + t * (area.width - 60), area.y + area.height - 46 - std::sin(t * PI) * 40};
            DrawBall(p, (i + stage) % (stage == 2 ? 5 : 4), 15);
        }
        (void)time;
    }
    void OnInit() override {
        Campaign::Game::OnInit();
        for (int i = 0; i < 5; ++i) {
            BallTextures[i] = RayGame::ResourceManager::LoadTexture(
                "marble-ball-" + std::to_string(i), Asset("ball-" + std::to_string(i) + ".png"));
            if (!BallTextures[i])
                throw std::runtime_error("Marble texture is missing");
            SetTextureFilter(*BallTextures[i], TEXTURE_FILTER_BILINEAR);
        }
        for (int i = 0; i < 3; ++i) {
            backdrops[i] = RayGame::ResourceManager::LoadTexture(
                "temple-" + std::to_string(i), Asset("temple-" + std::to_string(i) + ".png"));
            if (!backdrops[i])
                throw std::runtime_error("Temple backdrop is missing");
            SetTextureFilter(*backdrops[i], TEXTURE_FILTER_BILINEAR);
        }
        launcherTexture =
            RayGame::ResourceManager::LoadTexture("temple-launcher", Asset("launcher.png"));
        if (!launcherTexture)
            throw std::runtime_error("Temple launcher is missing");
        SetTextureFilter(*launcherTexture, TEXTURE_FILTER_BILINEAR);
        BallShadow = RayGame::ResourceManager::LoadTexture("marble-shadow", Asset("shadow.png"));
        if (BallShadow)
            SetTextureFilter(*BallShadow, TEXTURE_FILTER_BILINEAR);
    }
    void StartLevel(int selected) override {
        model.Reset(selected);
        heardShot = heardClear = 0;
        ammoChange = 1;
        shownNext = model.next;
        auto scene = RayGame::CreateRef<RayGame::Scene>("Marble Temple");
        scene->CreateGameObject("marble world")->AddComponent<World>(this);
        LoadScene(scene);
    }
    void PlayClick(Vector2 p) override {
        model.Fire(p);
    }
    void PlayCommand(int command) override {
        if (command == 30) {
            model.Swap();
            ammoChange = 0;
        }
    }
    void ExtraButtons() override {
        Add(30, {526, 630, 228, 42}, "SWAP COLOURS");
    }
    void Advance(float dt) override {
        model.Update(dt);
        if (model.next != shownNext) {
            shownNext = model.next;
            ammoChange = 0;
        }
        ammoChange = std::min(1.0f, ammoChange + dt / 0.18f);
        if (model.soundShot != heardShot) {
            heardShot = model.soundShot;
            Sound("shot");
        }
        if (model.soundClear != heardClear) {
            heardClear = model.soundClear;
            Sound("clear");
        }
    }
    int Outcome() const override {
        return model.outcome;
    }
    void OnRender() override {
        Campaign::Game::OnRender();
        if (screen != Campaign::Screen::Play)
            return;
        // Points rise above the launcher and the HUD rather than slipping behind them.
        auto o = Campaign::Origin();
        rlPushMatrix();
        rlTranslatef(o.x, o.y, 0);
        for (const auto& p : model.popups) {
            float fade = std::clamp((1 - p.age) / 0.4f, 0.0f, 1.0f), rise = 50 * (1 - (1 - p.age) * (1 - p.age));
            UI::TitleCentered(p.position.x, p.position.y - 30 - rise, TextFormat("+%d", p.points),
                              30 + 4 * std::min(p.combo, 4), Fade({255, 226, 140, 255}, fade));
            if (p.combo > 1)
                UI::LabelCentered(p.position.x, p.position.y + 6 - rise, TextFormat("COMBO x%d", p.combo), 18,
                                  Fade(Campaign::Cream, fade));
        }
        rlPopMatrix();
    }
    int Score() const override {
        return model.score;
    }
    void DrawHUD() override {
        using Campaign::UI;
        DrawRectangleRounded({28, 20, 1224, 62}, 0.15f, 8, {23, 39, 34, 245});
        UI::Title(48, 29, "MARBLE TEMPLE", 34, Campaign::Cream);
        UI::Label(347, 32, TextFormat("%d / 3  %s", level + 1, Tr(levelNames[level]).c_str()), 22,
                  Campaign::Cream);
        UI::Label(347, 58,
                  TextFormat((Tr("TO CLEAR") + "  %d     " + Tr("COMBO") + "  x%d").c_str(),
                             static_cast<int>(model.balls.size()) + model.remaining, model.combo),
                  16, {169, 203, 160, 255});
        UI::Title(850, 28, TextFormat("%05d", model.score), 36, {245, 208, 108, 255});
        if (!model.balls.empty() && model.balls.front().distance > model.path.length - 400) {
            UI::LabelCentered(640, 104, "DANGER! THE CHAIN IS NEAR THE GATE", 22,
                              {250, 153, 104, 255});
        }
        UI::Label(36, 647, "NEXT", 18, Campaign::Cream);
        DrawBall({112, 654}, model.next, 17 * PopIn(ammoChange));
        UI::Label(814, 645,
                  (model.shotsFired + 1) % 9 == 0
                      ? "BLAST SHOT READY"
                      : TextFormat(Tr("BLAST IN %d SHOTS").c_str(), 9 - model.shotsFired % 9),
                  20, {239, 209, 135, 255});
    }
    void DrawHelp() override {
        using Campaign::UI;
        const char* lines[] = {"1. Aim at the moving chain, then click to launch a marble.",
                               "2. Touch: drag to aim; lift your finger to shoot.",
                               "3. Three or more adjacent colours disappear together.",
                               "4. Matching ends close the gap and create chain reactions.",
                               "5. Space / right click / SWAP exchanges current and next.",
                               "6. Every ninth shot blasts nearby marbles of any colour.",
                               "7. Clear all marbles before the leading one enters the gate."};
        for (int i = 0; i < 7; ++i)
            UI::Label(364, 265 + i * 41, lines[i], 20, Campaign::Cream);
        for (int i = 0; i < 5; ++i)
            DrawBall({546.0f + i * 47, 570}, i);
    }
    void DrawWorld() {
        auto o = Campaign::Origin();
        rlPushMatrix();
        rlTranslatef(o.x, o.y, 0);
        if (backdrops[model.stage])
            DrawTexturePro(*backdrops[model.stage], {0, 0, 2560, 1440}, {0, 0, 1280, 720}, {}, 0,
                           WHITE);
        auto gate = model.path.At(model.path.length);
        DrawCircleSector({gate.x, gate.y + 3}, 34, 0, 360, 96, {13, 23, 21, 255});
        DrawRing(gate, 33, 35, 0, 360, 128, {214, 167, 86, 255});
        UI::LabelCentered(gate.x, gate.y + 39, "GATE", 15, Campaign::Cream);
        // New marbles grow out of a well at the start of the track.
        auto well = model.path.At(0);
        DrawCircleSector({well.x, well.y + 2}, 22, 0, 360, 64, {13, 23, 21, 230});
        DrawRing(well, 21, 23, 0, 360, 96, Fade({214, 167, 86, 255}, 0.7f));
        for (const auto& b : model.balls) {
            float emerge = std::clamp((b.distance - b.lag) / Spacing, 0.0f, 1.0f);
            DrawBall(model.Visible(b), b.color, 17 * (0.35f + 0.65f * emerge));
        }
        for (const auto& s : model.shots) {
            DrawLineEx({s.position.x - s.velocity.x * 0.025f, s.position.y - s.velocity.y * 0.025f},
                       s.position, 8, Fade(Colors[s.color], 0.28f));
            DrawBall(s.position, s.color, 13, s.blast);
        }
        for (const auto& s : model.sparks)
            for (int i = 0; i < 7; ++i) {
                float a = i * 2 * PI / 7, distance = s.age * 110;
                DrawCircleV(
                    {s.position.x + std::cos(a) * distance, s.position.y + std::sin(a) * distance},
                    4 * (1 - s.age / 0.7f), Fade(Colors[s.color], 1 - s.age / 0.7f));
            }
        auto p = model.launcher;
        float angle = std::atan2(aim.y - p.y, aim.x - p.x);
        if (screen == Campaign::Screen::Play) {
            for (int i = 1; i < 20; ++i)
                DrawCircleV({p.x + std::cos(angle) * i * 16, p.y + std::sin(angle) * i * 16}, 2,
                            Fade(Campaign::Cream, 0.28f));
        }
        DrawTexturePro(*launcherTexture, {0, 0, 256, 256}, {p.x, p.y, 128, 128}, {64, 64},
                       angle * RAD2DEG, WHITE);
        Vector2 nozzle{p.x + std::cos(angle) * 35, p.y + std::sin(angle) * 35};
        float reload = PopIn(1 - model.cooldown / 0.2f) * PopIn(ammoChange);
        DrawBall(nozzle, model.current, 13 * reload, (model.shotsFired + 1) % 9 == 0);
        rlPopMatrix();
    }
    int Smoke();
};
inline void World::FixedUpdate() {
    game->FixedStep(RayGame::GameTime::DeltaTime());
}
inline void World::Render() {
    game->DrawWorld();
}
} // namespace Marble
