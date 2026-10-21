#pragma once
#include "Model.h"
#include "../common/CampaignGame.h"

namespace Goo {
using Campaign::UI;
inline constexpr Color Mint{146, 218, 156, 255}, Dark{30, 67, 67, 255}, Gold{255, 211, 105, 255};
inline void Ball(Vector2 p, float radius, float time, Color color, Vector2 look = {1, -0.2f}) {
    float wobble = std::sin(time * 3 + p.x * 0.07f) * 0.04f;
    DrawEllipse(static_cast<int>(p.x + 2), static_cast<int>(p.y + 3), radius * 1.07f, radius,
                Fade(Dark, 0.35f));
    DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y), radius * (1 + wobble),
                radius * (1 - wobble), Dark);
    DrawEllipse(static_cast<int>(p.x - 1), static_cast<int>(p.y - 2), radius * 0.91f,
                radius * 0.87f, color);
    DrawEllipse(static_cast<int>(p.x - radius * 0.32f), static_cast<int>(p.y - radius * 0.45f),
                radius * 0.3f, radius * 0.16f, Fade(WHITE, 0.4f));
    for (float side : {-0.33f, 0.33f}) {
        Vector2 eye{p.x + radius * side, p.y - radius * 0.1f};
        DrawCircleV(eye, radius * 0.25f, {255, 249, 221, 255});
        DrawCircleV(Vector2Add(eye, Vector2Scale(look, radius * 0.09f)), radius * 0.11f, Dark);
    }
}
class Game;
class PlayScene : public PhysicsScene2D {
public:
    Model model;
    explicit PlayScene(int stage) : PhysicsScene2D("Goo Journey"), model(dynamics) {
        model.Reset(stage);
    }
};
class World : public Component {
public:
    Game* game;
    PlayScene* scene;
    World(Game* g, PlayScene* s) : game(g), scene(s) {}
    void FixedUpdate() override {
        scene->model.BeforeStep();
    }
    void PostFixedUpdate() override;
    void Render() override;
};
class Game : public Campaign::Game {
public:
    static constexpr const char* AppId = "raygame.goo_journey";
    Ref<PlayScene> arena;
    bool removeMode = false, placing = false, debugPhysics = false;
    Vector2 preview{400, 480};
    std::string feedback;
    float feedbackTime = 0;
    int heardRescue = 0, heardBroken = 0;
    explicit Game(std::filesystem::path directory)
        : Campaign::Game("goo_journey", "GOO JOURNEY", "Build a little courage. Carry the light.",
                         {"Across the Quiet Brook", "Up to the Lantern", "The Windmill Pass"},
                         std::move(directory)) {
        hint = "Build triangles. Keep the bridge steady before sending the lights.";
    }
    Model& M() {
        return arena->model;
    }
    const Model& M() const {
        return arena->model;
    }
    void StartLevel(int selected) override {
        arena = CreateRef<PlayScene>(selected);
        arena->CreateGameObject("living bridge")->AddComponent<World>(this, arena.get());
        LoadScene(arena);
        removeMode = placing = false;
        feedback.clear();
        heardRescue = heardBroken = 0;
    }
    void Notice(const std::string& message) {
        feedback = message;
        feedbackTime = 3;
    }
    void PlayClick(Vector2 p) override {
        if (removeMode) {
            if (M().Remove(M().Pick(p)))
                Sound("clear");
            else
                Notice("Fixed sockets cannot be removed.");
        } else if (M().Pick(p) < 0) {
            if (M().Place(p))
                Sound("shot");
            else
                Notice("Find two nearby balls and leave room for a triangle.");
        }
    }
    void PlayCommand(int command) override {
        if (command == 40)
            removeMode = false;
        if (command == 41 || command == 30)
            removeMode = !removeMode;
        if (command == 42) {
            if (!M().Launch())
                Notice("Connect the starting nest to a lantern socket first.");
            else {
                Sound("clear");
                Notice("The lights are on their way. Keep the path connected!");
            }
        }
        if (command == 43)
            debugPhysics = !debugPhysics;
    }
    void ExtraButtons() override {
        Add(40, {32, 640, 206, 56}, removeMode ? "BUILD" : "BUILD SELECTED");
        Add(41, {250, 640, 206, 56}, removeMode ? "RECYCLE SELECTED" : "RECYCLE");
        Add(42, {984, 640, 264, 56}, M().sending ? "LIGHTS ON THE WAY" : "SEND THE LIGHTS",
            M().waiting > 0 && !M().Route().empty());
    }
    void PointerBegin(int action, Vector2 p) override {
        if (screen != Campaign::Screen::Play || removeMode)
            return;
        preview = p;
        if (action == 40) {
            placing = true;
            return;
        }
        if (action != 100)
            return;
        int node = M().Pick(p);
        if (node >= 0)
            M().BeginDrag(node, p);
        else
            placing = true;
    }
    void PointerMove(Vector2 p) override {
        preview = p;
        if (M().dragged >= 0)
            M().MoveDrag(p);
    }
    bool PointerEnd(int, Vector2 p, bool dragged) override {
        if (M().dragged >= 0) {
            M().EndDrag();
            return true;
        }
        if (!placing)
            return false;
        placing = false;
        if (p.y >= 630)
            return false;
        if (M().Place(p))
            Sound("shot");
        else
            Notice("Find two nearby balls and leave room for a triangle.");
        (void)dragged;
        return true;
    }
    void PointerCancel() override {
        placing = false;
        if (arena)
            M().EndDrag();
    }
    void Advance(float dt) override {
        M().AfterStep(dt);
        feedbackTime = std::max(0.0f, feedbackTime - dt);
        if (M().rescued != heardRescue) {
            Sound("clear");
            heardRescue = M().rescued;
        }
        if (M().broken != heardBroken) {
            Sound("hurt");
            heardBroken = M().broken;
        }
    }
    int Outcome() const override {
        return M().outcome;
    }
    int Score() const override {
        return arena ? M().Score() : 0;
    }
    void DrawHUD() override {
        DrawRectangleRounded({28, 18, 992, 66}, 0.2f, 8, {26, 58, 62, 245});
        UI::Title(48, 28, "GOO JOURNEY", 27, Gold);
        UI::Label(340, 31, std::to_string(level + 1) + " / 3  " + Tr(levelNames[level]), 22,
                  Campaign::Cream);
        DrawRectangleRounded({30, 97, 450, 44}, 0.25f, 8, Fade(Dark, 0.85f));
        UI::Label(46, 106,
                  Tr("BALLS") + " " + std::to_string(M().reserve) + "    " + Tr("RESCUED") + " " +
                      std::to_string(M().rescued) + " / " + std::to_string(M().level.needed),
                  22, Campaign::Cream);
        DrawRectangleRounded({1000, 99, 248, 43}, 0.25f, 8, Fade(Dark, 0.8f));
        UI::LabelCentered(1124, 108,
                          Tr("TIME") + " " +
                              std::to_string(std::max(0, 240 - static_cast<int>(M().elapsed))),
                          20, Campaign::Cream);
        DrawRectangleRounded({472, 643, 497, 49}, 0.2f, 8, Fade(Dark, 0.92f));
        UI::LabelCentered(720, 657,
                          removeMode ? "Tap a green ball to recycle it."
                                     : "Drag from BUILD or tap between two balls.",
                          18, Campaign::Cream);
        if (feedbackTime > 0) {
            DrawRectangleRounded({200, 151, 880, 37}, 0.2f, 8, Fade(Dark, 0.95f));
            UI::LabelCentered(640, 159, feedback, 19, Gold);
        } else if (!M().Route().empty() && !M().sending) {
            UI::LabelCentered(710, 110, "PATH READY - SEND THE LIGHTS", 19, Dark);
        }
    }
    void DrawHelp() override {
        const char* lines[] = {"Drag from BUILD, or tap an empty spot near two balls.",
                               "Preview strands must form a triangle. Release to build.",
                               "Drag green balls gently. Golden sockets stay fixed.",
                               "Join both sides, then SEND THE LIGHTS to the lantern.",
                               "Travelers add weight. Red strands may snap under strain.",
                               "Recycle balls to rebuild. Falling lights cannot be saved.",
                               "Rescue the required lights before the four-minute dusk.",
                               "1 build   2 recycle   3 send   4 physics view"};
        for (int i = 0; i < 8; ++i)
            UI::LabelCentered(640, 264.0f + i * 38, lines[i], 19, Campaign::Cream);
    }
    int Smoke();
};
inline void World::PostFixedUpdate() {
    game->FixedStep(GameTime::FixedDeltaTime());
}
inline void World::Render() {
    const auto& model = scene->model;
    float t = model.elapsed;
    auto origin = Campaign::Origin();
    // All artwork is original procedural geometry, rendered into the adaptive canvas.
    DrawRectangleGradientV(0, 0, static_cast<int>(Display::GetWidth()),
                           static_cast<int>(Display::GetHeight()), {127, 184, 183, 255},
                           {231, 230, 181, 255});
    rlPushMatrix();
    rlTranslatef(origin.x, origin.y, 0);
    DrawCircleV({925, 245}, 83, {249, 233, 174, 255});
    DrawCircleV({925, 245}, 66, {255, 241, 193, 255});
    for (int layer = 0; layer < 3; ++layer) {
        Color colors[] = {{115, 164, 155, 255}, {92, 148, 139, 255}, {74, 127, 124, 255}};
        for (int i = -2; i < 8; ++i) {
            float x = i * 245.0f + layer * 71, y = 525 + layer * 55.0f;
            DrawEllipse(static_cast<int>(x), static_cast<int>(y), 250,
                        140 + 35 * std::sin(i * 2.0f), colors[layer]);
            DrawRectangle(static_cast<int>(x - 250), static_cast<int>(y), 500, 360, colors[layer]);
        }
    }
    for (int i = 0; i < 24; ++i) {
        Vector2 p{static_cast<float>((i * 197 + 33) % 1240),
                  210.0f + (i * 53 % 310) + std::sin(t + i) * 7};
        DrawCircleV(p, 2 + (i % 2), Fade(Gold, 0.55f));
    }
    DrawRectangleGradientV(static_cast<int>(-origin.x), 650, static_cast<int>(Display::GetWidth()),
                           static_cast<int>(std::max(70.0f, Display::GetHeight() - origin.y - 650)),
                           {76, 158, 158, 255}, {30, 94, 111, 255});
    for (int i = 0; i < 15; ++i)
        DrawLineEx({i * 94.0f + std::sin(t + i) * 8, 661.0f + (i % 4) * 11},
                   {i * 94.0f + 50, 661.0f + (i % 4) * 11}, 2, {155, 211, 187, 125});
    for (auto r : model.level.terrain) {
        DrawRectangleRounded(r, 0.08f, 6, {45, 82, 80, 255});
        DrawRectangleRounded({r.x + 8, r.y + 16, r.width - 16, r.height}, 0.08f, 6,
                             {58, 99, 90, 255});
        DrawRectangleRounded({r.x - 7, r.y - 5, r.width + 14, 22}, 0.5f, 8, {166, 197, 128, 255});
        for (int i = 0; i < static_cast<int>(r.width / 26); ++i) {
            float x = r.x + 16 + i * 26;
            DrawLineEx({x, r.y + 18}, {x + 7, r.y + 43 + (i % 4) * 13.0f}, 3, {31, 68, 63, 150});
            DrawCircleV({x, r.y - 7}, 4, {212, 218, 156, 255});
        }
    }
    Vector2 goal = model.level.exit;
    DrawLineEx({goal.x + 47, goal.y + 67}, {goal.x + 47, goal.y - 80}, 9, Dark);
    DrawLineEx({goal.x + 47, goal.y - 80}, {goal.x, goal.y - 80}, 7, Dark);
    DrawLineEx({goal.x, goal.y - 80}, {goal.x, goal.y - 42}, 3, Dark);
    DrawCircleV(goal, 43 + std::sin(t * 2) * 3, Fade(Gold, 0.22f));
    DrawRectangleRounded({goal.x - 24, goal.y - 30, 48, 60}, 0.3f, 8, Dark);
    DrawRectangleRounded({goal.x - 17, goal.y - 24, 34, 48}, 0.3f, 8, Gold);
    DrawCircleV(goal, 12 + std::sin(t * 4) * 2, {255, 246, 211, 255});
    auto nest = model.level.left;
    DrawEllipse(static_cast<int>(nest.x - 35), static_cast<int>(nest.y - 18), 39, 14, Dark);
    for (int i = 0; i < model.waiting; ++i)
        Ball({nest.x - 57 + (i % 4) * 14.0f, nest.y - 25 - (i / 4) * 12.0f}, 8, t + i, Gold);
    float alpha = scene->GetInterpolationAlpha();
    for (const auto& link : model.links) {
        auto a = model.Pos(link.a, alpha), b = model.Pos(link.b, alpha);
        float stress = Clamp(link.stress, 0.0f, 1.0f);
        Color color{static_cast<unsigned char>(136 + stress * 112),
                    static_cast<unsigned char>(205 - stress * 95),
                    static_cast<unsigned char>(146 - stress * 66), 255};
        float width =
            Clamp(11 * std::sqrt(link.rest / std::max(10.0f, Distance(a, b))), 5.0f, 15.0f);
        DrawLineEx(a, b, width + 5, Dark);
        DrawLineEx(a, b, width, color);
        DrawLineEx({a.x, a.y - 2}, {b.x, b.y - 2}, width * 0.22f, Fade(Campaign::Cream, 0.35f));
    }
    for (int i = 0; i < static_cast<int>(model.nodes.size()); ++i)
        if (model.nodes[i].alive) {
            auto p = model.Pos(i, alpha);
            if (model.nodes[i].anchor) {
                DrawCircleV(p, 18, Dark);
                DrawCircleV(p, 13, Gold);
                DrawCircleV(p, 5, Dark);
                DrawLineEx({p.x - 24, p.y + 23}, {p.x + 24, p.y + 23}, 3, Fade(Gold, 0.6f));
            } else {
                Vector2 look = Vector2Normalize(Vector2Subtract(game->aim, p));
                float grow = std::clamp(model.nodes[i].age / 0.25f, 0.0f, 1.0f) - 1; // pops in when placed
                grow = 1 + 2.70158f * grow * grow * grow + 1.70158f * grow * grow;
                Ball(p, (Radius + (model.dragged == i ? 2 : 0)) * grow, t, Mint, look);
            }
        }
    for (const auto& traveler : model.travelers)
        if (!traveler.done) {
            auto p = traveler.position;
            p.y -= 11;
            DrawCircleV(p, 17, Fade(Gold, 0.2f));
            Ball(p, 8, t, Gold);
        }
    for (const auto& b : model.bursts) {
        float k = b.age / 0.8f, fade = 1 - k;
        Vector2 c = b.position;
        if (b.kind == Effect::Snap) {
            for (int i = 0; i < 8; ++i) {
                float a = i * PI / 4 + 0.4f, d = 6 + k * 46;
                Vector2 s{c.x + std::cos(a) * d, c.y + std::sin(a) * d + k * k * 30};
                DrawLineEx(s, {s.x - std::cos(a) * 7, s.y - std::sin(a) * 7}, 3, Fade({248, 126, 84, 255}, fade));
            }
            DrawCircleV(c, 10 * fade, Fade({255, 214, 150, 255}, fade));
        } else if (b.kind == Effect::Rescue) {
            DrawCircleV(c, 30 + k * 60, Fade(Gold, 0.35f * fade));
            DrawRing(c, 26 + k * 70, 30 + k * 70, 0, 360, 64, Fade({255, 246, 211, 255}, fade));
            for (int i = 0; i < 10; ++i) {
                float a = i * PI / 5 + k * 2;
                DrawCircleV({c.x + std::cos(a) * (20 + k * 80), c.y + std::sin(a) * (20 + k * 80)}, 3 * fade + 1,
                            Fade(Gold, fade));
            }
        } else if (b.kind == Effect::Splash) {
            for (int i = 0; i < 7; ++i) {
                float spread = (i - 3) * 9.0f, up = 70 * k - 110 * k * k;
                DrawCircleV({c.x + spread * (1 + k * 2), std::min(670.0f, c.y - up * (1 - std::fabs(i - 3.0f) / 5))},
                            4 * fade + 1, Fade({196, 235, 230, 255}, fade));
            }
            DrawEllipseLines(static_cast<int>(c.x), 668, 12 + k * 40, 3 + k * 6, Fade({196, 235, 230, 255}, fade));
        } else {
            DrawRing(c, Radius * (1 - k) - 2, Radius * (1 - k) + 2, 0, 360, 48, Fade(Mint, fade));
            DrawCircleV(c, 6 * fade, Fade({255, 249, 221, 255}, fade));
        }
    }
    if (game->screen == Campaign::Screen::Play) {
        Vector2 p = game->placing ? game->preview : game->aim;
        if (!game->removeMode && model.dragged < 0 && p.y > 185 && p.y < 615 && model.Pick(p) < 0) {
            auto near = model.Candidates(p);
            Color color = model.CanPlace(p) ? Gold : Color{225, 126, 91, 255};
            for (int n : near) {
                Vector2 end = model.Pos(n, alpha);
                for (int j = 0; j < 10; j += 2)
                    DrawLineEx(RayGame::Lerp(p, end, j / 10.0f),
                               RayGame::Lerp(p, end, (j + 1) / 10.0f), 3, color);
            }
            DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y), Radius + 3, color);
            if (game->placing)
                Ball(p, Radius, t, Fade(color, 0.65f));
        }
        if (game->removeMode) {
            int selected = model.Pick(p);
            if (selected >= 0 && !model.nodes[selected].anchor)
                DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y), 24, RED);
        }
        if (game->debugPhysics)
            scene->dynamics.DebugDraw();
    }
    rlPopMatrix();
}
} // namespace Goo
