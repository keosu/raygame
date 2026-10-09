#include "RayGame.h"
#include "../common/ExampleUI.h"

using namespace RayGame;

class BouncingBallsScene : public Scene {
public:
    Rectangle arena = {24, 92, 752, 454};
    bool gravityEnabled = true;
    static constexpr int MAX_BALLS = 128;

    BouncingBallsScene() : Scene("Bouncing Balls") {}

    void OnViewportChanged(Vector2, Vector2 current) override {
        Rectangle previous = arena;
        arena = {24, 92, current.x - 48, current.y - 146};
        for (const auto& object : GetAllGameObjects()) {
            auto& p = object->transform->position;
            p = {arena.x + 16 + (p.x - previous.x - 16) * (arena.width - 32) / (previous.width - 32),
                 arena.y + 16 + (p.y - previous.y - 16) * (arena.height - 32) / (previous.height - 32)};
            p.x = Clamp(p.x, arena.x + 16, arena.x + arena.width - 16);
            p.y = Clamp(p.y, arena.y + 16, arena.y + arena.height - 16);
            if (auto particles = object->GetComponent<ParticleEmitter>()) particles->Clear();
        }
    }

    void OnLoad() override {
        arena = {24, 92, ExampleUI::Width() - 48, ExampleUI::Height() - 146};
        ExampleUI::Initialize({132, 233, 181, 255});
        for (int i = 0; i < 10; ++i) {
            AddRandomBall({Random(arena.x + 24, arena.x + arena.width - 24),
                           Random(arena.y + 24, arena.y + arena.height * 0.6f)});
        }
    }

    void Update() override {
        if (Input::GetKeyDown(KEY_R)) { Clear(); OnLoad(); Start(); }
        if (Input::GetKeyDown(KEY_G)) {
            gravityEnabled = !gravityEnabled;
            for (const auto& obj : GetAllGameObjects()) obj->GetComponent<Rigidbody>()->useGravity = gravityEnabled;
        }
        if (Input::GetKeyDown(KEY_SPACE)) AddRandomBall();
        Vector2 mouse = Input::GetMousePosition();
        if (Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT) && Input::IsMouseInCanvas()
            && CheckCollisionPointRec(mouse, arena)) AddRandomBall(mouse);
        Scene::Update();
    }

    void FixedUpdate() override {
        Scene::FixedUpdate();
        for (const auto& obj : GetAllGameObjects()) {
            auto body = obj->GetComponent<Rigidbody>();
            if (!body) continue;
            auto& pos = obj->transform->position;
            auto& vel = body->velocity;
            constexpr float radius = 16;
            if (pos.x - radius < arena.x) { pos.x = arena.x + radius; vel.x = std::fabs(vel.x) * 0.9f; }
            if (pos.x + radius > arena.x + arena.width) { pos.x = arena.x + arena.width - radius; vel.x = -std::fabs(vel.x) * 0.9f; }
            if (pos.y - radius < arena.y) { pos.y = arena.y + radius; vel.y = std::fabs(vel.y) * 0.9f; }
            if (pos.y + radius > arena.y + arena.height) { pos.y = arena.y + arena.height - radius; vel.y = -std::fabs(vel.y) * 0.85f; }
        }
    }

    void Render() override {
        DrawRectangleGradientV(0, 0, ExampleUI::Width(), ExampleUI::Height(), {10, 22, 34, 255}, {6, 11, 23, 255});
        DrawRectangleRounded(arena, 0.035f, 8, {12, 23, 38, 255});
        DrawRectangleRoundedLines(arena, 0.035f, 8, {45, 73, 92, 255});
        for (int y = 112; y < ExampleUI::Height() - 70; y += 32) {
            for (int x = 44; x < ExampleUI::Width() - 40; x += 32) DrawCircle(x, y, 1, {36, 55, 73, 255});
        }
        Scene::Render();
        ExampleUI::Header(ExampleUI::Width(), "BOUNCING BALLS", "PHYSICS PLAYGROUND");
        ExampleUI::Stat(ExampleUI::Width() - 180.0f, "BALLS", TextFormat("%03d / %d", (int)GetAllGameObjects().size(), MAX_BALLS), UI::accentColor);
        ExampleUI::Stat(ExampleUI::Width() - 32.0f, "GRAVITY", gravityEnabled ? "ON" : "OFF", ExampleUI::gold);
        ExampleUI::Footer(ExampleUI::Width(), ExampleUI::Height(), "SPACE / CLICK  ADD    G  GRAVITY    R  RESET");
    }

    void AddRandomBall(Vector2 position = {-1, -1}) {
        if (GetAllGameObjects().size() >= MAX_BALLS) return;
        auto ball = CreateGameObject("Ball");
        if (position.x < 0) position = {Random(arena.x + 24, arena.x + arena.width - 24), arena.y + 32};
        ball->transform->position = {
            Clamp(position.x, arena.x + 16, arena.x + arena.width - 16),
            Clamp(position.y, arena.y + 16, arena.y + arena.height - 16)};
        constexpr Color palette[] = {{97,218,255,255}, {255,115,173,255}, {255,207,112,255}, {132,233,181,255}, {181,157,255,255}};
        Color color = palette[GetRandomValue(0, 4)];
        auto texture = ResourceManager::CreateCircleTexture("ball", 16, WHITE);
        ball->AddComponent<SpriteRenderer>(texture)->tint = color;
        auto body = ball->AddComponent<Rigidbody>();
        body->velocity = {Random(-180.0f, 180.0f), Random(-100.0f, 50.0f)};
        body->drag = 1;
        body->useGravity = gravityEnabled;
        body->gravity = 300;
        ball->AddComponent<CircleCollider>(16.0f);
        auto particles = ball->AddComponent<ParticleEmitter>();
        particles->renderOrder = -1;
        particles->emissionRate = 16;
        particles->maxParticles = 32;
        particles->startVelocityMin = {-12, -12};
        particles->startVelocityMax = {12, 12};
        particles->acceleration = {0, 30};
        particles->lifetimeMin = 0.15f;
        particles->lifetimeMax = 0.4f;
        particles->startSizeMin = 1;
        particles->startSizeMax = 3;
        particles->startColor = color;
    }
};

class MyGame : public GameEngine {
public:
    MyGame() : GameEngine("RayGame - Bouncing Balls", 800, 600) {}
    void OnInit() override { LoadScene(CreateRef<BouncingBallsScene>()); }
};

int main() {
    return ExampleUI::Run<MyGame>("raygame.bouncing_balls");
}
