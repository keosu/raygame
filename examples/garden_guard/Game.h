#pragma once
#include "Model.h"
#include "../common/CampaignGame.h"

namespace Garden {
using Campaign::UI;
inline constexpr Color Leaf{90, 163, 66, 255};
inline void DrawPlant(Vector2 p, Kind kind, float time = 0, bool small = false) {
    float bounce = small ? 0 : std::sin(time * 3 + p.x) * 2;
    p.y += bounce;
    float scale = small ? 0.67f : 1;
    auto ellipse = [&](float x, float y, float w, float h, Color color) {
        DrawEllipse(static_cast<int>(p.x + x * scale), static_cast<int>(p.y + y * scale), w * scale,
                    h * scale, color);
    };
    auto circle = [&](float x, float y, float r, Color color) {
        DrawCircleV({p.x + x * scale, p.y + y * scale}, r * scale, color);
    };
    ellipse(2, 28, 32, 8, {32, 60, 23, 75});
    if (kind == Kind::Wallnut) {
        ellipse(0, 0, 28, 35, {104, 69, 34, 255});
        ellipse(-3, -3, 24, 31, {182, 130, 65, 255});
        ellipse(-11, -14, 7, 10, {207, 157, 87, 255});
        for (int i = 0; i < 3; ++i)
            DrawLineEx({p.x - 16 * scale, p.y + (i * 10 - 1) * scale},
                       {p.x - 12 * scale, p.y + (i * 10 + 4) * scale}, 2, {139, 93, 43, 255});
        circle(-9, -9, 6, Campaign::Cream);
        circle(9, -9, 6, Campaign::Cream);
        circle(-8, -8, 2.5f, BLACK);
        circle(10, -8, 2.5f, BLACK);
        DrawLineEx({p.x - 5 * scale, p.y + 11 * scale}, {p.x + 6 * scale, p.y + 11 * scale}, 2,
                   {83, 53, 27, 255});
        return;
    }
    ellipse(-17, 22, 19, 9, Leaf);
    ellipse(18, 21, 20, 9, {70, 139, 62, 255});
    DrawLineEx({p.x, p.y + 23 * scale}, {p.x, p.y - 10 * scale}, 7 * scale, {67, 132, 59, 255});
    if (kind == Kind::Sunflower) {
        for (int i = 0; i < 10; ++i) {
            float a = i * PI / 5;
            circle(std::cos(a) * 23, std::sin(a) * 23 - 10, 10, {242, 182, 52, 255});
        }
        circle(0, -10, 19, {132, 82, 39, 255});
        circle(-4, -14, 15, {169, 114, 51, 255});
        circle(-7, -14, 3, BLACK);
        circle(7, -14, 3, BLACK);
        DrawLineEx({p.x - 7 * scale, p.y}, {p.x + 6 * scale, p.y + 3 * scale}, 2,
                   {65, 43, 23, 255});
    } else if (kind == Kind::Cherry) {
        DrawLineEx({p.x - 12 * scale, p.y}, {p.x, p.y - 28 * scale}, 4 * scale, Leaf);
        DrawLineEx({p.x + 12 * scale, p.y}, {p.x, p.y - 28 * scale}, 4 * scale, Leaf);
        circle(-13, 0, 17, {212, 57, 59, 255});
        circle(13, 1, 18, {230, 66, 62, 255});
        circle(-17, -5, 5, {246, 137, 106, 255});
        circle(10, -5, 5, {246, 137, 106, 255});
    } else {
        Color body = kind == Kind::Iceflower  ? Color{109, 190, 214, 255}
                     : kind == Kind::Repeater ? Color{55, 134, 68, 255}
                                              : Color{120, 188, 74, 255};
        circle(-6, -12, 25, body);
        ellipse(16, -9, 23, 13, body);
        circle(34, -9, 13,
               kind == Kind::Iceflower ? Color{54, 106, 150, 255} : Color{49, 101, 44, 255});
        ellipse(34, -9, 6, 9, {28, 54, 34, 255});
        circle(-13, -19, 5, Campaign::Cream);
        circle(-12, -19, 2.5f, BLACK);
        if (kind == Kind::Repeater) {
            circle(17, -25, 10, body);
            circle(27, -25, 8, {36, 90, 44, 255});
        }
        if (kind == Kind::Iceflower)
            for (int i = 0; i < 3; ++i)
                Campaign::Triangle({p.x + (i * 10 - 22) * scale, p.y - 28 * scale},
                                   {p.x + (i * 10 - 15) * scale, p.y - 44 * scale},
                                   {p.x + (i * 10 - 9) * scale, p.y - 27 * scale},
                                   {170, 235, 240, 255});
    }
}
inline void DrawZombie(const Zombie& z, float time) {
    float y = Center(z.row, 0).y, x = z.x, sway = std::sin(time * 5 + z.id) * 5;
    Color skin = z.slow > 0 ? Color{130, 187, 204, 255} : Color{145, 163, 114, 255};
    Color coat = z.type == 2 ? Color{161, 82, 66, 255} : Color{103, 89, 76, 255};
    DrawEllipse(static_cast<int>(x + 2), static_cast<int>(y + 32), 25, 8, {27, 49, 27, 75});
    DrawLineEx({x - 9, y + 10}, {x - 12 + sway, y + 29}, 9, {67, 74, 75, 255});
    DrawLineEx({x + 9, y + 10}, {x + 9 - sway, y + 29}, 9, {67, 74, 75, 255});
    DrawEllipse(static_cast<int>(x - 17 + sway), static_cast<int>(y + 31), 11, 5,
                {46, 46, 39, 255});
    DrawEllipse(static_cast<int>(x + 4 - sway), static_cast<int>(y + 31), 11, 5, {46, 46, 39, 255});
    DrawRectangleRounded({x - 17, y - 23, 33, 39}, 0.15f, 6, coat);
    DrawRectangle(x - 4, y - 18, 9, 25, {218, 210, 184, 255});
    Campaign::Triangle({x, y - 18}, {x - 5, y}, {x + 6, y + 2}, {141, 60, 54, 255});
    DrawLineEx({x - 12, y - 14}, {x - 35, y - 8 + (z.bite > 0 ? sway : 0)}, 8, coat);
    DrawCircleV({x - 36, y - 8}, 7, skin);
    DrawCircleV({x - 5, y - 38}, 22, skin);
    DrawCircleV({x - 11, y - 44}, 9, {205, 217, 185, 255});
    DrawCircleV({x + 3, y - 44}, 8, {205, 217, 185, 255});
    DrawCircleV({x - 14, y - 44}, 3, BLACK);
    DrawCircleV({x, y - 44}, 3, BLACK);
    DrawRectangle(x - 18, y - 30, 18, 6, {67, 67, 45, 255});
    DrawRectangle(x - 14, y - 30, 5, 5, Campaign::Cream);
    if (z.type == 1) {
        Campaign::Triangle({x - 26, y - 55}, {x - 5, y - 92}, {x + 17, y - 55},
                           {224, 131, 59, 255});
        DrawLineEx({x - 24, y - 62}, {x + 14, y - 62}, 6, {248, 208, 122, 255});
    }
    if (z.type == 2)
        DrawRectangleRounded({x - 25, y - 60, 40, 10}, 0.3f, 5, {189, 56, 47, 255});
    if (z.flash > 0)
        DrawCircleV({x - 6, y - 30}, 30, Fade(WHITE, 0.22f));
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
    static constexpr const char* AppId = "raygame.garden_guard";
    Model model;
    int heardShot = 0, heardHit = 0, heardPlant = 0;
    std::string feedback;
    float feedbackTime = 0;
    int dragSeed = -1;
    bool dragActive = false;
    Vector2 dragPoint{};
    void PointerBegin(int action, Vector2 p) override {
        if (screen != Campaign::Screen::Play)
            return;
        if (action >= 40 && action <= 46) {
            dragSeed = action - 40;
            dragPoint = p;
        }
    }
    void PointerMove(Vector2 p) override {
        if (dragSeed < 0)
            return;
        dragPoint = p;
        dragActive = pointer.dragging;
    }
    bool PointerEnd(int, Vector2 p, bool dragged) override {
        if (dragSeed < 0)
            return false;
        int seed = dragSeed;
        PointerCancel();
        if (!dragged)
            return false;
        int row = static_cast<int>(std::floor((p.y - Top) / CellH));
        int col = static_cast<int>(std::floor((p.x - Left) / CellW));
        if (row < 0 || row >= Rows || col < 0 || col >= Columns)
            return true;
        model.selected = seed;
        bool placed = seed == 6 ? model.Shovel(row, col) : model.PlantSeed(row, col, seed);
        if (!placed) {
            feedback = "Need sun, an empty tile and a ready seed packet.";
            feedbackTime = 2;
        }
        return true;
    }
    void PointerCancel() override {
        dragSeed = -1;
        dragActive = false;
    }
    explicit Game(std::filesystem::path directory)
        : Campaign::Game(
              "garden_guard", "GARDEN GUARD", "Grow a defence. Protect the little house.",
              {"Sunny Front Yard", "Armoured Visitors", "The Last Stand"}, std::move(directory)) {
        hint = "Grow sunflowers early. Cover every row before the waves arrive.";
    }
    void StartLevel(int selected) override {
        model.Reset(selected);
        heardShot = heardHit = heardPlant = 0;
        feedback.clear();
        auto scene = RayGame::CreateRef<RayGame::Scene>("Garden Guard");
        scene->CreateGameObject("garden world")->AddComponent<World>(this);
        LoadScene(scene);
    }
    void PlayClick(Vector2 p) override {
        if (!model.Click(p)) {
            feedback = "Need sun, an empty tile and a ready seed packet.";
            feedbackTime = 2;
        } else if (model.selected == 6) {
            feedback = "Plant removed. Choose a seed packet to replant.";
            feedbackTime = 2;
        }
    }
    void PlayCommand(int command) override {
        if (command >= 40 && command <= 46) {
            model.selected = command - 40;
            feedback.clear();
        }
        if (command == 30)
            model.selected = 6;
    }
    void ExtraButtons() override {
        for (int i = 0; i < 6; ++i) {
            Add(40 + i, {240.0f + i * 144, 100, 134, 86}, "");
        }
        Add(46, {1120, 100, 124, 86}, "SHOVEL / 7");
    }
    int Outcome() const override {
        return model.outcome;
    }
    int Score() const override {
        return model.score;
    }
    void Advance(float dt) override {
        model.Update(dt);
        feedbackTime -= dt;
        if (model.soundShot != heardShot) {
            heardShot = model.soundShot;
            Sound("shot");
        }
        if (model.soundHit != heardHit) {
            heardHit = model.soundHit;
            Sound("hurt");
        }
        if (model.soundPlant != heardPlant) {
            heardPlant = model.soundPlant;
            Sound("clear");
        }
    }
    void DrawHUD() override {
        using Campaign::UI;
        DrawRectangleRounded({28, 20, 1224, 62}, 0.15f, 8, {39, 65, 43, 245});
        UI::Title(47, 29, "GARDEN GUARD", 34, Campaign::Cream);
        UI::Label(331, 31, TextFormat("%d / 3  %s", level + 1, Tr(levelNames[level]).c_str()), 22,
                  Campaign::Cream);
        UI::Label(331, 57,
                  TextFormat((Tr("WAVE") + " %d / 3   " + Tr("DEFEATED") + "  %d / %d").c_str(),
                             model.elapsed < 60   ? 1
                             : model.elapsed < 98 ? 2
                                                  : 3,
                             model.defeated, static_cast<int>(model.schedule.size())),
                  16, {187, 212, 164, 255});
        UI::Title(867, 28, TextFormat("%05d", model.score), 36, {251, 211, 111, 255});
        DrawRectangleRounded({40, 99, 175, 86}, 0.16f, 8, {80, 98, 52, 255});
        DrawCircleV({74, 140}, 18, {247, 199, 72, 255});
        UI::Title(102, 120, std::to_string(model.sun), 36, Campaign::Cream);
        UI::Label(57, 164, "SUN TO SPEND", 15, Campaign::Cream);
        if (feedbackTime > 0 && !feedback.empty())
            UI::LabelCentered(640, 685, feedback, 16, {224, 228, 190, 255});
    }
    void OnRender() override {
        Campaign::Game::OnRender();
        if (screen != Campaign::Screen::Play)
            return;
        auto o = Campaign::Origin();
        rlPushMatrix();
        rlTranslatef(o.x, o.y, 0);
        for (int i = 0; i < 6; ++i) {
            Rectangle r{240.0f + i * 144, 100, 134, 86};
            if (model.selected == i)
                DrawRectangleRoundedLinesEx(r, 0.18f, 24, 2, {255, 217, 102, 255});
            DrawPlant({r.x + 33, r.y + 47}, static_cast<Kind>(i), 0, true);
            UI::Label(r.x + 61, r.y + 17, TextFormat("%d  / %d", i + 1, Costs[i]), 18,
                      Campaign::Cream);
            const char* labels[] = {"SUN", "PEA", "WALL", "ICE", "BOMB", "DOUBLE"};
            UI::Label(r.x + 61, r.y + 42, labels[i], 16, Campaign::Cream);
            if (model.cooldown[i] > 0 || model.sun < Costs[i]) {
                float fraction = model.cooldown[i] / Recharges[i];
                DrawRectangleRec(
                    {r.x, r.y, r.width, model.cooldown[i] > 0 ? r.height * fraction : r.height},
                    {22, 32, 30, 110});
                if (model.cooldown[i] > 0)
                    UI::Label(r.x + 91, r.y + 64, TextFormat("%.0fs", std::ceil(model.cooldown[i])),
                              15, Campaign::Cream);
            }
        }
        if (model.selected == 6)
            DrawRectangleLinesEx({1120, 100, 124, 86}, 3, {255, 217, 102, 255});
        auto point = dragActive ? dragPoint : aim;
        int row = static_cast<int>(std::floor((point.y - Top) / CellH)),
            col = static_cast<int>(std::floor((point.x - Left) / CellW));
        if (row >= 0 && col >= 0 && row < Rows && col < Columns) {
            int seed = dragActive ? dragSeed : model.selected;
            bool valid = seed == 6 ? model.PlantAt(row, col) >= 0
                                   : model.PlantAt(row, col) < 0 && model.cooldown[seed] <= 0 &&
                                         model.sun >= Costs[seed];
            Rectangle cell{Left + col * CellW, Top + row * CellH, CellW, CellH};
            Color glow = valid ? Color{215, 246, 142, 220} : Color{246, 116, 94, 220};
            DrawRectangleRec(cell, Fade(glow, 0.14f));
            DrawRectangleLinesEx(cell, 3, glow);
            if (dragActive && seed < 6) {
                DrawPlant(Center(row, col), static_cast<Kind>(seed));
                DrawRectangleRec(cell, Fade(glow, 0.1f));
            }
        }
        if (dragActive) {
            Vector2 ghost{point.x, point.y - (pointer.IsTouch() ? 60 : 18)};
            DrawCircleV({ghost.x, ghost.y + 12}, 42, {20, 41, 28, 110});
            if (dragSeed < 6)
                DrawPlant(ghost, static_cast<Kind>(dragSeed));
            else
                RayGame::ButtonPainter::Icon(RayGame::ButtonIcon::Drop, ghost, 44, Campaign::Cream);
        }
        if (model.elapsed < 22)
            UI::TitleCentered(715, 300,
                              Tr("PLANT YOUR DEFENCE") +
                                  TextFormat("  /  %.0fs", std::ceil(22 - model.elapsed)),
                              28, {249, 234, 178, 255});
        if (model.waveFlash > 0 && model.spawned > 0)
            UI::LabelCentered(715, 198, "VISITORS APPROACH!", 20, {255, 219, 119, 255});
        rlPopMatrix();
    }
    void DrawHelp() override {
        using Campaign::UI;
        const char* lines[] = {"Collect glowing sun tokens to pay for plants.",
                               "Select a seed packet (1 - 6), then click an empty tile.",
                               "Sunflowers produce sun. Peashooters attack their own row.",
                               "Wallnuts block; ice flowers slow; repeaters fire twice.",
                               "Cherry bombs clear a 3 x 3 area after a short fuse.",
                               "Shovel (7) removes a plant. Seed packets need to recharge.",
                               "Each row has one mower. A second breach loses the level.",
                               "Survive all three waves. Cover all five rows!"};
        for (int i = 0; i < 8; ++i)
            UI::Label(359, 256 + i * 39, lines[i], 20, Campaign::Cream);
    }
    void DrawWorld() {
        auto o = Campaign::Origin();
        rlPushMatrix();
        rlTranslatef(o.x, o.y, 0);
        Color sky[] = {{122, 169, 150, 255}, {128, 150, 172, 255}, {180, 139, 116, 255}};
        DrawRectangleRounded({28, 90, 1224, 590}, 0.05f, 10, sky[model.stage]);
        for (int i = 0; i < 10; ++i) {
            DrawCircleV({75.0f + i * 126, 115}, 67, {86, 125, 68, 255});
            DrawCircleV({93.0f + i * 126, 104}, 45, {107, 147, 83, 255});
        }
        // Cottage, picket fence and garden beds are code-native artwork.
        DrawRectangle(52, 249, 112, 340, {224, 203, 159, 255});
        Campaign::Triangle({36, 262}, {109, 188}, {179, 262}, {142, 70, 51, 255});
        DrawRectangle(72, 446, 55, 143, {115, 91, 64, 255});
        DrawCircle(117, 521, 4, {235, 191, 87, 255});
        for (int row = 0; row < Rows; ++row)
            for (int col = 0; col < Columns; ++col) {
                Rectangle tile{Left + col * CellW, Top + row * CellH, CellW, CellH};
                bool alternate = (row + col) % 2;
                DrawRectangleRec(tile,
                                 alternate ? Color{102, 147, 70, 255} : Color{113, 159, 76, 255});
                DrawRectangleLinesEx(tile, 1, {83, 136, 63, 95});
                for (int i = 0; i < 3; ++i) {
                    float x = tile.x + 17 + i * 31, y = tile.y + 67 + (col * 7 + row * 3) % 9;
                    DrawLineEx({x, y}, {x - 3, y - 6}, 1, {76, 126, 53, 170});
                }
            }
        for (int row = 0; row < Rows; ++row) {
            float x = model.mowerX[row] >= 0 ? model.mowerX[row] : Left - 45,
                  y = Center(row, 0).y + 6;
            if (model.mowerReady[row] || model.mowerX[row] >= 0) {
                DrawCircleV({x - 12, y + 15}, 8, {41, 48, 42, 255});
                DrawCircleV({x + 14, y + 15}, 8, {41, 48, 42, 255});
                DrawRectangleRounded({x - 23, y - 7, 45, 23}, 0.2f, 6, {176, 64, 46, 255});
                DrawLineEx({x + 12, y - 5}, {x + 28, y - 30}, 4, {60, 75, 60, 255});
            }
        }
        for (const auto& p : model.plants) {
            auto center = Center(p.row, p.col);
            DrawPlant(center, p.kind, model.elapsed);
            float maxHp = p.kind == Kind::Wallnut ? 520 : 110;
            if (p.hp < maxHp) {
                DrawRectangle(static_cast<int>(center.x - 25), static_cast<int>(center.y + 34), 50,
                              4, {65, 63, 42, 255});
                DrawRectangle(static_cast<int>(center.x - 25), static_cast<int>(center.y + 34),
                              static_cast<int>(50 * p.hp / maxHp), 4, {211, 205, 92, 255});
            }
        }
        for (const auto& z : model.zombies)
            DrawZombie(z, model.elapsed);
        for (const auto& pea : model.peas) {
            auto center = Center(pea.row, 0);
            DrawCircleV({pea.x + 1, center.y - 5}, 9, {44, 77, 32, 80});
            DrawCircleV({pea.x, center.y - 10}, 8,
                        pea.ice ? Color{159, 233, 247, 255} : Color{193, 223, 103, 255});
        }
        for (const auto& sun : model.suns) {
            auto p = sun.position;
            DrawCircleV(p, 31,
                        Fade({249, 217, 90, 255}, 0.2f + 0.06f * std::sin(model.elapsed * 4)));
            for (int i = 0; i < 8; ++i) {
                float a = i * PI / 4 + model.elapsed * 0.5f;
                Campaign::Triangle({p.x + std::cos(a) * 28, p.y + std::sin(a) * 28},
                                   {p.x + std::cos(a + 0.18f) * 18, p.y + std::sin(a + 0.18f) * 18},
                                   {p.x + std::cos(a - 0.18f) * 18, p.y + std::sin(a - 0.18f) * 18},
                                   {246, 199, 61, 255});
            }
            DrawCircleV(p, 18, {251, 211, 93, 255});
            DrawCircleV({p.x - 4, p.y - 5}, 9, {255, 236, 157, 255});
        }
        for (const auto& b : model.blasts) {
            DrawCircleV(b.position, 115 * b.age / 0.6f + 15,
                        Fade({248, 184, 66, 255}, 1 - b.age / 0.6f));
            DrawCircleLines(static_cast<int>(b.position.x), static_cast<int>(b.position.y),
                            155 * b.age / 0.6f, {249, 203, 87, 255});
        }
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
} // namespace Garden
