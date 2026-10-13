#pragma once
#include "Model.h"
#include "../common/CampaignGame.h"

namespace Marble {
using RayGame::UI;
inline constexpr Color Colors[] = {{226, 83, 72, 255},
                                   {243, 181, 61, 255},
                                   {74, 172, 123, 255},
                                   {65, 148, 207, 255},
                                   {164, 102, 196, 255}};
inline void DrawBall(Vector2 p, int color, float radius = 17, bool blast = false) {
    DrawCircleV({p.x + 2, p.y + 4}, radius + 2, {17, 28, 22, 135});
    DrawCircleV(p, radius + 1, {42, 44, 31, 255});
    DrawCircleV(p, radius, Colors[color]);
    DrawCircleV({p.x - radius * 0.3f, p.y - radius * 0.3f}, radius * 0.58f, Fade(WHITE, 0.15f));
    DrawCircleV({p.x - radius * 0.4f, p.y - radius * 0.48f}, radius * 0.18f, Fade(WHITE, 0.5f));
    // Each colour also has a distinct glyph.
    if (color == 0)
        DrawLineEx({p.x - 5, p.y + 4}, {p.x + 5, p.y + 4}, 3, Campaign::Cream);
    if (color == 1) {
        DrawLineEx({p.x, p.y - 4}, {p.x, p.y + 5}, 2, Campaign::Cream);
        DrawLineEx({p.x - 4, p.y}, {p.x + 4, p.y}, 2, Campaign::Cream);
    }
    if (color == 2)
        DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y), 5, Campaign::Cream);
    if (color == 3)
        DrawRectangleLines(static_cast<int>(p.x - 4), static_cast<int>(p.y - 4), 8, 8,
                           Campaign::Cream);
    if (color == 4)
        Campaign::Triangle({p.x, p.y - 5}, {p.x - 5, p.y + 4}, {p.x + 5, p.y + 4}, Campaign::Cream);
    if (blast)
        DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y), radius + 5, WHITE);
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
    int heardShot = 0, heardClear = 0;
    explicit Game(std::filesystem::path directory)
        : Campaign::Game(
              "marble_temple", "MARBLE TEMPLE", "Aim. Match three. Keep the ancient gate sealed.",
              {"Mossy Courtyard", "Moonlit Ruins", "The Spiral Vault"}, std::move(directory)) {
        hint = "Swap colours to reach pairs. Every ninth shot is a blast.";
    }
    void StartLevel(int selected) override {
        model.Reset(selected);
        heardShot = heardClear = 0;
        auto scene = RayGame::CreateRef<RayGame::Scene>("Marble Temple");
        scene->CreateGameObject("marble world")->AddComponent<World>(this);
        LoadScene(scene);
    }
    void PlayClick(Vector2 p) override {
        model.Fire(p);
    }
    void PlayCommand(int command) override {
        if (command == 30)
            model.Swap();
    }
    void ExtraButtons() override {
        Add(30, {526, 630, 228, 42}, "SWAP COLOURS");
    }
    void Advance(float dt) override {
        model.Update(dt);
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
    int Score() const override {
        return model.score;
    }
    void DrawHUD() override {
        using RayGame::UI;
        DrawRectangleRounded({28, 20, 1224, 62}, 0.15f, 8, {23, 39, 34, 245});
        UI::Title(48, 29, "MARBLE TEMPLE", 34, Campaign::Cream);
        UI::Label(347, 32, TextFormat("%d / 3  %s", level + 1, levelNames[level].c_str()), 22,
                  Campaign::Cream);
        UI::Label(347, 58,
                  TextFormat("TO CLEAR  %d     COMBO  x%d",
                             static_cast<int>(model.balls.size()) + model.remaining, model.combo),
                  16, {169, 203, 160, 255});
        UI::Title(850, 28, TextFormat("%05d", model.score), 36, {245, 208, 108, 255});
        if (!model.balls.empty() && model.balls.front().distance > model.path.length - 400) {
            UI::LabelCentered(640, 104, "DANGER! THE CHAIN IS NEAR THE GATE", 22,
                              {250, 153, 104, 255});
        } else
            UI::LabelCentered(640, 97,
                              "Match 3+ adjacent colours. Gaps close into chain reactions.", 17,
                              Campaign::Cream);
        UI::Label(36, 647, "NEXT", 18, Campaign::Cream);
        DrawBall({112, 654}, model.next, 17);
        UI::Label(814, 645,
                  (model.shotsFired + 1) % 9 == 0
                      ? "BLAST SHOT READY"
                      : TextFormat("BLAST IN %d SHOTS", 9 - model.shotsFired % 9),
                  20, {239, 209, 135, 255});
        UI::Label(36, 670, "Mouse: click to fire / Space to swap. Touch: drag aim, lift to fire.",
                  16, Campaign::Cream);
    }
    void DrawHelp() override {
        using RayGame::UI;
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
        BeginMode2D({{}, o, 0, 1});
        Color backgrounds[] = {{35, 65, 45, 255}, {30, 49, 75, 255}, {66, 45, 38, 255}};
        Color stone[] = {{85, 103, 64, 255}, {75, 96, 115, 255}, {123, 88, 56, 255}};
        DrawRectangleRounded({28, 90, 1224, 590}, 0.05f, 10, backgrounds[model.stage]);
        for (int y = 124; y < 625; y += 76)
            for (int x = 46; x < 1230; x += 112) {
                DrawRectangleLinesEx(
                    {static_cast<float>(x + (y % 3) * 15), static_cast<float>(y), 92, 57}, 1,
                    Fade(stone[model.stage], 0.17f));
            }
        for (int i = 0; i < 18; ++i) {
            float x = 58 + (i * 193 % 1160), y = 127 + (i * 97 % 490);
            DrawEllipse(static_cast<int>(x), static_cast<int>(y), 25, 8, {25, 46, 28, 155});
            DrawLineEx({x, y}, {x - 10, y - 16}, 3, {63, 118, 71, 120});
        }
        for (size_t i = 1; i < model.path.points.size(); ++i) {
            auto a = model.path.points[i - 1], b = model.path.points[i];
            DrawLineEx({a.x + 3, a.y + 6}, {b.x + 3, b.y + 6}, 50, {8, 21, 19, 170});
            DrawLineEx(a, b, 46, stone[model.stage]);
            DrawLineEx(a, b, 32, {38, 41, 29, 255});
            DrawLineEx(a, b, 24, {46, 47, 32, 255});
        }
        for (float s = 40; s < model.path.length; s += 64) {
            auto p = model.path.At(s), t = model.path.Tangent(s);
            DrawLineEx({p.x - t.y * 20, p.y + t.x * 20}, {p.x + t.y * 20, p.y - t.x * 20}, 2,
                       Fade(BLACK, 0.3f));
        }
        auto gate = model.path.At(model.path.length);
        DrawCircleV({gate.x, gate.y + 3}, 34, {13, 23, 21, 255});
        DrawCircleLines(static_cast<int>(gate.x), static_cast<int>(gate.y), 35,
                        {214, 167, 86, 255});
        UI::LabelCentered(gate.x, gate.y + 39, "GATE", 15, Campaign::Cream);
        for (const auto& b : model.balls)
            DrawBall(model.path.At(b.distance), b.color);
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
        DrawCircleV({p.x + 3, p.y + 8}, 58, {9, 24, 19, 155});
        DrawCircleV(p, 54, {110, 119, 67, 255});
        DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y), 49, {226, 197, 99, 255});
        DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y + 5), 40, 31, {57, 123, 73, 255});
        Vector2 nozzle{p.x + std::cos(angle) * 35, p.y + std::sin(angle) * 35};
        DrawLineEx(p, nozzle, 32, {119, 169, 82, 255});
        DrawCircleV(nozzle, 16, {35, 69, 38, 255});
        DrawBall(nozzle, model.current, 13, (model.shotsFired + 1) % 9 == 0);
        DrawCircleV({p.x - std::sin(angle) * 23, p.y + std::cos(angle) * 23}, 9, Campaign::Cream);
        DrawCircleV({p.x + std::sin(angle) * 23, p.y - std::cos(angle) * 23}, 9, Campaign::Cream);
        DrawCircleV({p.x - std::sin(angle) * 23 + 2, p.y + std::cos(angle) * 23}, 4, BLACK);
        DrawCircleV({p.x + std::sin(angle) * 23 + 2, p.y - std::cos(angle) * 23}, 4, BLACK);
        EndMode2D();
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
