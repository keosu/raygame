#include "RayGame.h"

// Avoid 'using namespace' to prevent Time class conflict with <ctime>
namespace {
    using RayGame::Component;
    using RayGame::GameObject;
    using RayGame::Scene;
    using RayGame::GameEngine;
    using RayGame::CreateRef;
    using RayGame::Ref;
    using RayGame::Rigidbody;
    using RayGame::CircleCollider;
    using RayGame::SpriteRenderer;
    using RayGame::ParticleEmitter;
    using RayGame::EmitterShape;
    using RayGame::ResourceManager;
    using RayGame::Input;
    using RayGame::Random;
    using RayGame::GameTime;
}

// Simple bouncing balls demo
class BouncingBallsScene : public Scene {
public:
    BouncingBallsScene() : Scene("Bouncing Balls Demo") {}
    
    void OnLoad() override {
        // Create some bouncing balls
        for (int i = 0; i < 10; i++) {
            auto ball = CreateGameObject("Ball");
            ball->transform->position = {
                Random(100.0f, 700.0f),
                Random(100.0f, 500.0f)
            };
            
            // Add sprite
            auto tex = ResourceManager::CreateCircleTexture(
                "ball" + std::to_string(i), 16, 
                Color{
                    (unsigned char)GetRandomValue(100, 255),
                    (unsigned char)GetRandomValue(100, 255),
                    (unsigned char)GetRandomValue(100, 255),
                    255
                }
            );
            auto sprite = ball->AddComponent<SpriteRenderer>(tex);
            
            // Add physics
            auto rb = ball->AddComponent<Rigidbody>();
            rb->velocity = {Random(-200.0f, 200.0f), Random(-200.0f, 200.0f)};
            rb->drag = 1.0f;  // No drag
            rb->useGravity = true;
            rb->gravity = 300.0f;
            
            auto collider = ball->AddComponent<CircleCollider>(16.0f);
            
            // Add particle trail
            auto particles = ball->AddComponent<ParticleEmitter>();
            particles->emissionRate = 20;
            particles->maxParticles = 50;
            particles->shape = EmitterShape::Point;
            particles->startVelocityMin = {-20, -20};
            particles->startVelocityMax = {20, 20};
            particles->acceleration = {0, 50};
            particles->lifetimeMin = 0.3f;
            particles->lifetimeMax = 0.6f;
            particles->startSizeMin = 2;
            particles->startSizeMax = 4;
            particles->startColor = sprite->tint;
        }
    }
    
    void Update() override {
        Scene::Update();
        
        // Keep balls in bounds (bounce off walls)
        for (auto& obj : GetAllGameObjects()) {
            auto rb = obj->GetComponent<Rigidbody>();
            if (!rb) continue;
            
            Vector2& pos = obj->transform->position;
            Vector2& vel = rb->velocity;
            
            float radius = 16;
            if (pos.x - radius < 0) {
                pos.x = radius;
                vel.x = -vel.x * 0.8f;
            }
            if (pos.x + radius > 800) {
                pos.x = 800 - radius;
                vel.x = -vel.x * 0.8f;
            }
            if (pos.y - radius < 0) {
                pos.y = radius;
                vel.y = -vel.y * 0.8f;
            }
            if (pos.y + radius > 600) {
                pos.y = 600 - radius;
                vel.y = -vel.y * 0.8f;
            }
        }
    }
    
    void Render() override {
        Scene::Render();
        
        // Draw UI
        DrawText("RAYGAME ENGINE - Bouncing Balls Demo", 10, 10, 20, WHITE);
        DrawText(TextFormat("Objects: %d", GetAllGameObjects().size()), 10, 35, 20, YELLOW);
        DrawText("F1: Toggle Debug", 10, 60, 15, GRAY);
        DrawText("ESC: Quit", 10, 75, 15, GRAY);
        DrawText("SPACE: Add Ball", 10, 90, 15, GRAY);
        
        if (Input::GetKeyDown(KEY_SPACE)) {
            AddRandomBall();
        }
    }
    
    void AddRandomBall() {
        auto ball = CreateGameObject("Ball");
        ball->transform->position = {Random(100.0f, 700.0f), 50.0f};
        
        int ballIndex = GetAllGameObjects().size();
        auto tex = ResourceManager::CreateCircleTexture(
            "ball_new_" + std::to_string(ballIndex), 16,
            Color{
                (unsigned char)GetRandomValue(100, 255),
                (unsigned char)GetRandomValue(100, 255),
                (unsigned char)GetRandomValue(100, 255),
                255
            }
        );
        auto sprite = ball->AddComponent<SpriteRenderer>(tex);
        
        auto rb = ball->AddComponent<Rigidbody>();
        rb->velocity = {Random(-100.0f, 100.0f), Random(-50.0f, 50.0f)};
        rb->drag = 1.0f;
        rb->useGravity = true;
        rb->gravity = 300.0f;
        
        auto collider = ball->AddComponent<CircleCollider>(16.0f);
        
        auto particles = ball->AddComponent<ParticleEmitter>();
        particles->emissionRate = 20;
        particles->maxParticles = 50;
        particles->shape = EmitterShape::Point;
        particles->startVelocityMin = {-20, -20};
        particles->startVelocityMax = {20, 20};
        particles->acceleration = {0, 50};
        particles->lifetimeMin = 0.3f;
        particles->lifetimeMax = 0.6f;
        particles->startSizeMin = 2;
        particles->startSizeMax = 4;
        particles->startColor = sprite->tint;
    }
};

class MyGame : public GameEngine {
public:
    MyGame() : GameEngine("RayGame Engine Demo", 800, 600) {}
    
    void OnInit() override {
        auto scene = CreateRef<BouncingBallsScene>();
        LoadScene(scene);
    }
};

int main() {
    MyGame game;
    game.Run();
    return 0;
}

