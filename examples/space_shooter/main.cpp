#include "RayGame.h"

// Note: Not including <random> as it conflicts with <ctime> through raylib headers
// Use raylib's GetRandomValue() instead

// Avoid 'using namespace' to prevent potential naming conflicts
// Use explicit RayGame:: qualification where needed
namespace {
    using RayGame::Component;
    using RayGame::GameObject;
    using RayGame::Scene;
    using RayGame::GameEngine;
    using RayGame::CreateRef;
    using RayGame::Ref;
    using RayGame::Rigidbody;
    using RayGame::CircleCollider;
    using RayGame::Collider;
    using RayGame::SpriteRenderer;
    using RayGame::ParticleEmitter;
    using RayGame::EmitterShape;
    using RayGame::Camera2DComponent;
    using RayGame::ResourceManager;
    using RayGame::Input;
    using RayGame::Normalize;
    using RayGame::Random;
    using RayGame::GameTime;
}

// Forward declaration
class SpaceShooterScene;

// Player controller component
class PlayerController : public Component {
public:
    float speed = 300.0f;
    float fireRate = 0.2f;
    float fireTimer = 0.0f;
    
    void Update() override;
    void Shoot();
    
    Scene* scene = nullptr;  // Reference to scene for spawning bullets
};

// Enemy AI component
class EnemyAI : public Component {
public:
    float speed = 100.0f;
    float amplitude = 50.0f;
    float frequency = 2.0f;
    float startX = 0;
    
    void Start() override {
        startX = gameObject->transform->position.x;
    }
    
    void Update() override {
        if (!gameObject) return;
        
        // Move down
        gameObject->transform->position.y += speed * GameTime::DeltaTime();
        
        // Sine wave movement
        float offset = sin(GameTime::GetTime() * frequency) * amplitude;
        gameObject->transform->position.x = startX + offset;
        
        // Destroy if off screen
        if (gameObject->transform->position.y > 700) {
            gameObject->active = false;
        }
    }
};

// Bullet component
class Bullet : public Component {
public:
    float speed = 500.0f;
    bool isPlayerBullet = true;
    
    void Update() override {
        if (!gameObject) return;
        
        gameObject->transform->position.y += (isPlayerBullet ? -speed : speed) * GameTime::DeltaTime();
        
        // Destroy if off screen
        if (gameObject->transform->position.y < -10 || 
            gameObject->transform->position.y > 700) {
            gameObject->active = false;
        }
    }
};


// Game scene
class SpaceShooterScene : public Scene {
public:
    Ref<GameObject> player;
    float enemySpawnTimer = 0;
    float enemySpawnRate = 2.0f;
    int score = 0;
    
    SpaceShooterScene() : Scene("Space Shooter") {}
    
    void OnLoad() override {
        // Create player
        player = CreateGameObject("Player");
        player->tag = "Player";
        player->transform->position = {400, 500};
        
        // Add sprite (using colored texture)
        auto playerTex = ResourceManager::CreateColorTexture("player", 32, 32, BLUE);
        auto sprite = player->AddComponent<SpriteRenderer>(playerTex);
        sprite->tint = SKYBLUE;
        
        // Add physics
        auto rb = player->AddComponent<Rigidbody>();
        rb->drag = 0.9f;
        
        auto collider = player->AddComponent<CircleCollider>(16.0f);
        
        // Add controller
        auto controller = player->AddComponent<PlayerController>();
        controller->scene = this;
        
        // Add particle emitter for engine trail
        auto particles = player->AddComponent<ParticleEmitter>();
        particles->emissionRate = 50;
        particles->maxParticles = 100;
        particles->shape = EmitterShape::Circle;
        particles->radius = 5;
        particles->startVelocityMin = {-20, 20};
        particles->startVelocityMax = {20, 50};
        particles->acceleration = {0, -50};
        particles->lifetimeMin = 0.2f;
        particles->lifetimeMax = 0.5f;
        particles->startSizeMin = 2;
        particles->startSizeMax = 4;
        particles->startColor = {100, 200, 255, 255};
        
        // Create camera
        auto cameraObj = CreateGameObject("Camera");
        auto camera = cameraObj->AddComponent<Camera2DComponent>();
        camera->offset = {400, 300};
        camera->zoom = 1.0f;
        
        // Create some initial enemies
        for (int i = 0; i < 3; i++) {
            SpawnEnemy();
        }
    }
    
    void Update() override {
        Scene::Update();
        
        // Spawn enemies
        enemySpawnTimer += GameTime::DeltaTime();
        if (enemySpawnTimer >= enemySpawnRate) {
            SpawnEnemy();
            enemySpawnTimer = 0;
            
            // Increase difficulty
            if (enemySpawnRate > 0.5f) {
                enemySpawnRate -= 0.05f;
            }
        }
        
        // Check bullet-enemy collisions (simplified)
        auto bullets = FindGameObjectsByTag("Bullet");
        auto enemies = FindGameObjectsByTag("Enemy");
        
        for (auto& bullet : bullets) {
            auto bulletCollider = bullet->GetComponent<Collider>();
            if (!bulletCollider) continue;
            
            for (auto& enemy : enemies) {
                auto enemyCollider = enemy->GetComponent<Collider>();
                if (!enemyCollider) continue;
                
                if (bulletCollider->CheckCollision(enemyCollider.get())) {
                    // Destroy both
                    bullet->active = false;
                    enemy->active = false;
                    score += 10;
                    
                    // Spawn explosion particles
                    SpawnExplosion(enemy->transform->position);
                    
                    break;
                }
            }
        }
        
        // Check player-enemy collisions
        if (player && player->active) {
            auto playerCollider = player->GetComponent<Collider>();
            if (playerCollider) {
                for (auto& enemy : enemies) {
                    auto enemyCollider = enemy->GetComponent<Collider>();
                    if (!enemyCollider) continue;
                    
                    if (playerCollider->CheckCollision(enemyCollider.get())) {
                        // Game over effect
                        SpawnExplosion(player->transform->position);
                        player->active = false;
                    }
                }
            }
        }
    }
    
    void Render() override {
        Scene::Render();
        
        // Draw UI
        DrawText("SPACE SHOOTER DEMO", 10, 10, 20, WHITE);
        DrawText(TextFormat("Score: %d", score), 10, 35, 20, YELLOW);
        DrawText("WASD/Arrows: Move", 10, 60, 15, GRAY);
        DrawText("SPACE: Shoot", 10, 75, 15, GRAY);
        DrawText("F1: Toggle Debug", 10, 90, 15, GRAY);
        DrawText("ESC: Quit", 10, 105, 15, GRAY);
        
        if (!player || !player->active) {
            DrawText("GAME OVER!", 300, 250, 40, RED);
            DrawText("Press R to Restart", 290, 300, 25, WHITE);
            
            if (Input::GetKeyDown(KEY_R)) {
                // Reload scene
                OnUnload();
                gameObjects.clear();
                score = 0;
                enemySpawnRate = 2.0f;
                OnLoad();
                Start();
            }
        }
    }
    
    void SpawnEnemy() {
        auto enemy = CreateGameObject("Enemy");
        enemy->tag = "Enemy";
        enemy->transform->position = {Random(50, 750), -30};
        
        // Add sprite
        auto enemyTex = ResourceManager::CreateColorTexture("enemy", 32, 32, RED);
        auto sprite = enemy->AddComponent<SpriteRenderer>(enemyTex);
        sprite->tint = {255, 100, 100, 255};
        
        // Add collider
        auto collider = enemy->AddComponent<CircleCollider>(16.0f);
        
        // Add AI
        auto ai = enemy->AddComponent<EnemyAI>();
        ai->speed = Random(50, 150);
        ai->amplitude = Random(30, 100);
        ai->frequency = Random(1, 3);
    }
    
    void SpawnBullet(Vector2 position, bool isPlayerBullet = true) {
        auto bullet = CreateGameObject("Bullet");
        bullet->tag = "Bullet";
        bullet->transform->position = position;
        
        // Add sprite
        auto bulletTex = ResourceManager::CreateColorTexture("bullet", 8, 16, YELLOW);
        auto sprite = bullet->AddComponent<SpriteRenderer>(bulletTex);
        sprite->tint = isPlayerBullet ? YELLOW : RED;
        
        // Add collider
        auto collider = bullet->AddComponent<CircleCollider>(4.0f);
        collider->isTrigger = true;
        
        // Add bullet component
        auto bulletComp = bullet->AddComponent<Bullet>();
        bulletComp->isPlayerBullet = isPlayerBullet;
    }
    
    void SpawnExplosion(Vector2 position) {
        auto explosion = CreateGameObject("Explosion");
        explosion->transform->position = position;
        
        auto particles = explosion->AddComponent<ParticleEmitter>();
        particles->emissionRate = 0;  // Burst only
        particles->maxParticles = 50;
        particles->shape = EmitterShape::Circle;
        particles->radius = 10;
        particles->startVelocityMin = {-150, -150};
        particles->startVelocityMax = {150, 150};
        particles->acceleration = {0, 100};
        particles->lifetimeMin = 0.3f;
        particles->lifetimeMax = 0.8f;
        particles->startSizeMin = 3;
        particles->startSizeMax = 8;
        particles->startColor = {255, 150, 0, 255};
        particles->emitting = false;
        particles->loop = false;
        
        particles->Burst(50);
        
        // Auto-destroy after 2 seconds
        // In a real engine, you'd have a component for this
    }
};

// PlayerController implementations (after SpaceShooterScene is defined)
void PlayerController::Update() {
    if (!gameObject) return;
    
    // Movement
    Vector2 input = Input::GetMovementInput();
    if (input.x != 0 || input.y != 0) {
        Vector2 normalized = Normalize(input);
        
        auto rb = gameObject->GetComponent<Rigidbody>();
        if (rb) {
            rb->SetVelocity({
                normalized.x * speed,
                normalized.y * speed
            });
        }
    }
    
    // Shooting
    fireTimer -= GameTime::DeltaTime();
    
    if (Input::GetKey(KEY_SPACE) && fireTimer <= 0) {
        Shoot();
        fireTimer = fireRate;
    }
    
    // Keep in bounds
    float margin = 20;
    if (gameObject->transform->position.x < margin) 
        gameObject->transform->position.x = margin;
    if (gameObject->transform->position.x > 800 - margin) 
        gameObject->transform->position.x = 800 - margin;
    if (gameObject->transform->position.y < margin) 
        gameObject->transform->position.y = margin;
    if (gameObject->transform->position.y > 600 - margin) 
        gameObject->transform->position.y = 600 - margin;
}

void PlayerController::Shoot() {
    if (!gameObject || !scene) return;
    
    Vector2 pos = gameObject->transform->position;
    pos.y -= 20;
    
    // Cast to SpaceShooterScene to spawn bullet
    SpaceShooterScene* gameScene = dynamic_cast<SpaceShooterScene*>(scene);
    if (gameScene) {
        gameScene->SpawnBullet(pos, true);
    }
}

// Main game engine class
class SpaceShooterGame : public GameEngine {
public:
    SpaceShooterGame() : GameEngine("RayGame Engine - Space Shooter Demo", 800, 600) {}
    
    void OnInit() override {
        // Seed random
        srand(static_cast<unsigned int>(GetRandomValue(0, 100000)));
        
        // Load scene
        auto scene = CreateRef<SpaceShooterScene>();
        
        // Store reference for player shooting
        gameScene = scene;
        
        LoadScene(scene);
    }
    
    Ref<SpaceShooterScene> gameScene;
};

int main() {
    SpaceShooterGame game;
    game.Run();
    return 0;
}

