#include "RayGame.h"
#include "../common/ExampleUI.h"

using namespace RayGame;

enum class ShapeKind { Circle, Square, Bar };

// Outline is a drawing style. Both styles use the same solid circle/box collider.
class PlaygroundShape : public Component {
public:
    ShapeKind kind = ShapeKind::Circle;
    Vector2 halfSize{18, 18};
    Color color = WHITE;
    bool outline = false;

    void Start() override {
        gameObject->transform->SetInterpolationEnabled(true);
    }
    void Render() override {
        Vector2 p = gameObject->transform->GetRenderPosition();
        if (kind == ShapeKind::Circle) {
            if (outline)
                DrawRing(p, halfSize.x - 3, halfSize.x, 0, 360, 48, color);
            else {
                DrawCircleV(p, halfSize.x, color);
                DrawCircleV({p.x - 5, p.y - 6}, 5, Fade(WHITE, 0.35f));
            }
        } else {
            Rectangle rect{p.x - halfSize.x, p.y - halfSize.y, halfSize.x * 2, halfSize.y * 2};
            if (outline)
                DrawRectangleLinesEx(rect, 3, color);
            else {
                DrawRectangleRec(rect, color);
                DrawRectangleRec({rect.x + 3, rect.y + 3, rect.width - 6, 3}, Fade(WHITE, 0.35f));
            }
        }
    }
};

class BouncingBallsScene : public Scene {
public:
    Rectangle arena{24, 148, 752, 380};
    bool gravityEnabled = true;
    bool resetRequested = false, gravityRequested = false;
    int selectedShape = 0; // 0 = mix, 1 = circle, 2 = square, 3 = bar.
    int selectedStyle = 0; // 0 = mixed, 1 = solid, 2 = outline.
    static constexpr int MAX_BALLS = 128;

    BouncingBallsScene() : Scene("Shape Playground") {}

    void OnViewportChanged(Vector2, Vector2 current) override {
        Rectangle previous = arena;
        arena = {24, 148, current.x - 48, current.y - 220};
        for (const auto& object : GetAllGameObjects()) {
            auto shape = object->GetComponent<PlaygroundShape>();
            if (!shape)
                continue;
            auto& p = object->transform->position;
            p = {arena.x + (p.x - previous.x) * arena.width / previous.width,
                 arena.y + (p.y - previous.y) * arena.height / previous.height};
            p.x =
                Clamp(p.x, arena.x + shape->halfSize.x, arena.x + arena.width - shape->halfSize.x);
            p.y =
                Clamp(p.y, arena.y + shape->halfSize.y, arena.y + arena.height - shape->halfSize.y);
            if (auto particles = object->GetComponent<ParticleEmitter>())
                particles->Clear();
        }
        ResetInterpolation();
    }

    void OnLoad() override {
        arena = {24, 148, ExampleUI::Width() - 48, ExampleUI::Height() - 220};
        ExampleUI::Initialize({132, 233, 181, 255}, "bouncing_balls");
        for (int i = 0; i < 10; ++i) {
            AddShape(
                {arena.x + 60 + (i % 5) * (arena.width - 120) / 4, arena.y + 65 + (i / 5) * 90.0f},
                i % 3, i % 2 != 0);
        }
    }

    void Update() override {
        if (resetRequested || Input::GetKeyDown(KEY_R)) {
            resetRequested = false;
            Clear();
            Load();
        }
        if (gravityRequested || Input::GetKeyDown(KEY_G)) {
            gravityRequested = false;
            gravityEnabled = !gravityEnabled;
            for (const auto& obj : GetAllGameObjects())
                if (auto body = obj->GetComponent<Rigidbody>())
                    body->useGravity = gravityEnabled;
        }
        for (int i = 0; i < 4; ++i)
            if (Input::GetKeyDown(KEY_ONE + i))
                selectedShape = i;
        if (Input::GetKeyDown(KEY_F))
            selectedStyle = (selectedStyle + 1) % 3;
        if (Input::GetKeyDown(KEY_SPACE))
            AddShape();
        Vector2 mouse = Input::GetMousePosition();
        if (Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT) && Input::IsMouseInCanvas() &&
            CheckCollisionPointRec(mouse, arena))
            AddShape(mouse);
        Scene::Update();
    }

    void PostFixedUpdate() override {
        Scene::PostFixedUpdate();
        for (const auto& obj : GetAllGameObjects()) {
            auto body = obj->GetComponent<Rigidbody>();
            auto shape = obj->GetComponent<PlaygroundShape>();
            if (!body || !shape)
                continue;
            auto& pos = obj->transform->position;
            auto& vel = body->velocity;
            auto half = shape->halfSize;
            if (pos.x - half.x < arena.x) {
                pos.x = arena.x + half.x;
                vel.x = std::fabs(vel.x) * 0.9f;
            }
            if (pos.x + half.x > arena.x + arena.width) {
                pos.x = arena.x + arena.width - half.x;
                vel.x = -std::fabs(vel.x) * 0.9f;
            }
            if (pos.y - half.y < arena.y) {
                pos.y = arena.y + half.y;
                vel.y = std::fabs(vel.y) * 0.9f;
            }
            if (pos.y + half.y > arena.y + arena.height) {
                pos.y = arena.y + arena.height - half.y;
                vel.y = -std::fabs(vel.y) * 0.85f;
            }
        }
    }

    void Render() override {
        DrawRectangleGradientV(0, 0, ExampleUI::Width(), ExampleUI::Height(), {10, 22, 34, 255},
                               {6, 11, 23, 255});
        DrawRectangleRounded(arena, 0.035f, 8, {12, 23, 38, 255});
        DrawRectangleRoundedLines(arena, 0.035f, 8, {45, 73, 92, 255});
        for (float y = arena.y + 16; y < arena.y + arena.height - 12; y += 32)
            for (float x = arena.x + 16; x < arena.x + arena.width - 12; x += 32)
                DrawCircleV({x, y}, 1, {36, 55, 73, 255});
        Scene::Render();
        ExampleUI::Header(ExampleUI::Width(), "SHAPE LAB", "BOUNCING BALLS / PHYSICS PLAYGROUND");
        ExampleUI::Stat(ExampleUI::Width() - 172, "SHAPES",
                        TextFormat("%03d / %d", (int)GetAllGameObjects().size(), MAX_BALLS),
                        UI::accentColor);
        ExampleUI::Stat(ExampleUI::Width() - 32, "GRAVITY", gravityEnabled ? "ON" : "OFF",
                        ExampleUI::gold);
        const char* names[] = {"MIX", "CIRCLE", "SQUARE", "BAR"};
        for (int i = 0; i < 4; ++i) {
            Rectangle button{24 + i * 106.0f, 90, 98, 42};
            if (UI::Button(button, names[i], 18))
                selectedShape = i;
            if (selectedShape == i)
                DrawRectangleRec({button.x + 8, button.y + 38, button.width - 16, 3},
                                 UI::accentColor);
        }
        const char* styles[] = {"MIXED STYLE", "SOLID", "OUTLINE"};
        if (UI::Button({458, 90, 132, 42}, styles[selectedStyle], 16))
            selectedStyle = (selectedStyle + 1) % 3;
        if (UI::Button({602, 90, 78, 42}, "RESET", 16))
            resetRequested = true;
        if (UI::Button({692, 90, 84, 42}, "GRAVITY", 14))
            gravityRequested = true;
        UI::Label(28, ExampleUI::Height() - 62, "CHOOSE A SHAPE ABOVE, THEN TAP / CLICK THE ARENA",
                  16, ExampleUI::muted);
        ExampleUI::Footer(ExampleUI::Width(), ExampleUI::Height(),
                          "SPACE ADD   1-4 SHAPE   F STYLE",
                          "TAP TO ADD / CHOOSE SHAPE AND STYLE ABOVE");
    }

    void AddShape(Vector2 position = {-1, -1}, int kind = -1, int style = -1) {
        if (GetAllGameObjects().size() >= MAX_BALLS)
            return;
        if (kind < 0)
            kind = selectedShape == 0 ? GetRandomValue(0, 2) : selectedShape - 1;
        auto object = CreateGameObject("Shape");
        auto shape = object->AddComponent<PlaygroundShape>();
        shape->kind = static_cast<ShapeKind>(kind);
        shape->halfSize = kind == 0   ? Vector2{18, 18}
                          : kind == 1 ? Vector2{20, 20}
                                      : Vector2{30, 12};
        shape->outline = style >= 0           ? style != 0
                         : selectedStyle == 0 ? GetRandomValue(0, 1) != 0
                                              : selectedStyle == 2;
        if (position.x < 0)
            position = {Random(arena.x + 40, arena.x + arena.width - 40), arena.y + 38};
        object->transform->position = {Clamp(position.x, arena.x + shape->halfSize.x,
                                             arena.x + arena.width - shape->halfSize.x),
                                       Clamp(position.y, arena.y + shape->halfSize.y,
                                             arena.y + arena.height - shape->halfSize.y)};
        constexpr Color palette[] = {{97, 218, 255, 255},
                                     {255, 115, 173, 255},
                                     {255, 207, 112, 255},
                                     {132, 233, 181, 255},
                                     {181, 157, 255, 255}};
        shape->color = palette[GetRandomValue(0, 4)];
        auto body = object->AddComponent<Rigidbody>();
        body->velocity = {Random(-180.0f, 180.0f), Random(-100.0f, 50.0f)};
        body->drag = 1;
        body->useGravity = gravityEnabled;
        body->gravity = 300;
        if (kind == 0)
            object->AddComponent<CircleCollider>(shape->halfSize.x);
        else
            object->AddComponent<BoxCollider>(
                Vector2{shape->halfSize.x * 2, shape->halfSize.y * 2});
        auto particles = object->AddComponent<ParticleEmitter>();
        particles->renderOrder = -1;
        particles->emissionRate = 10;
        particles->maxParticles = 24;
        particles->startVelocityMin = {-12, -12};
        particles->startVelocityMax = {12, 12};
        particles->acceleration = {0, 30};
        particles->lifetimeMin = 0.15f;
        particles->lifetimeMax = 0.3f;
        particles->startSizeMin = 1;
        particles->startSizeMax = 2;
        particles->startColor = shape->color;
        particles->endColor = Fade(shape->color, 0);
    }
};

class MyGame : public GameEngine {
public:
    MyGame() : GameEngine("RayGame - Shape Lab", 800, 600) {}
    void OnInit() override {
        LoadScene(CreateRef<BouncingBallsScene>());
    }
};

int main() {
    return ExampleUI::Run<MyGame>("raygame.bouncing_balls");
}
