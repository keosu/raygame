#include "RayGame.h"

using namespace RayGame;

// Forward declaration
class SpaceShooterScene;

// Star for background
struct Star {
    Vector2 position;
    float size;
    float brightness;
    float twinkleSpeed;
    float twinklePhase;
};

// Starfield background component with twinkling
class StarfieldBackground : public Component {
public:
    std::vector<Star> stars;
    int numStars = 150;
    bool initialized = false;
    
    void InitializeStars() {
        if (initialized) return;
        
        stars.clear();
        stars.reserve(numStars);
        
        // Use raylib's GetRandomValue (safe and reliable)
        for (int i = 0; i < numStars; i++) {
            Star star;
            star.position = {
                static_cast<float>(GetRandomValue(0, 800)),
                static_cast<float>(GetRandomValue(0, 600))
            };
            star.size = static_cast<float>(GetRandomValue(10, 30)) / 10.0f;
            star.brightness = static_cast<float>(GetRandomValue(30, 100)) / 100.0f;
            star.twinkleSpeed = static_cast<float>(GetRandomValue(5, 30)) / 10.0f;
            star.twinklePhase = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
            stars.push_back(star);
        }
        initialized = true;
    }
    
    void Start() override {
        // Initialize in Start() - raylib is guaranteed to be initialized by then
        InitializeStars();
    }
    
    void Update() override {
        // Ensure initialization (defensive programming)
        if (!initialized) {
            InitializeStars();
        }
        if (stars.empty()) return;
        
        // Update star twinkling
        float time = GameTime::GetTime();
        for (auto& star : stars) {
            star.brightness = 0.5f + 0.5f * sinf(time * star.twinkleSpeed + star.twinklePhase);
            
            // Slowly move stars down for parallax effect
            star.position.y += 10.0f * GameTime::DeltaTime();
            if (star.position.y > 600) {
                star.position.y = 0;
                star.position.x = static_cast<float>(GetRandomValue(0, 800));
            }
        }
    }
    
    void Render() override {
        // Ensure initialization (defensive programming)
        if (!initialized) {
            InitializeStars();
        }
        if (stars.empty()) return;
        
        // Draw twinkling stars
        for (const auto& star : stars) {
            unsigned char alpha = (unsigned char)(star.brightness * 255);
            Color starColor = {255, 255, 255, alpha};
            DrawCircleV(star.position, star.size, starColor);
        }
    }
};

// Custom renderer for player ship
class PlayerShipRenderer : public Component {
public:
    Color primaryColor = SKYBLUE;
    Color accentColor = BLUE;
    
    void Render() override {
        if (!gameObject || !gameObject->transform) return;
        
        Vector2 pos = gameObject->transform->position;
        
        // Draw player ship as a triangle with wings
        Vector2 nose = {pos.x, pos.y - 20};
        Vector2 leftWing = {pos.x - 16, pos.y + 12};
        Vector2 rightWing = {pos.x + 16, pos.y + 12};
        Vector2 leftTail = {pos.x - 8, pos.y + 16};
        Vector2 rightTail = {pos.x + 8, pos.y + 16};
        
        // Main body
        DrawTriangle(leftWing, rightWing, nose, primaryColor);
        
        // Cockpit
        DrawCircle(pos.x, pos.y - 5, 5, accentColor);
        
        // Wings detail
        DrawTriangle(
            {pos.x - 16, pos.y + 12},
            {pos.x - 12, pos.y + 8},
            {pos.x - 10, pos.y + 12},
            accentColor
        );
        DrawTriangle(
            {pos.x + 16, pos.y + 12},
            {pos.x + 12, pos.y + 8},
            {pos.x + 10, pos.y + 12},
            accentColor
        );
        
        // Tail fins
        DrawTriangle(leftTail, {pos.x - 4, pos.y + 10}, {pos.x - 8, pos.y + 10}, DARKBLUE);
        DrawTriangle(rightTail, {pos.x + 4, pos.y + 10}, {pos.x + 8, pos.y + 10}, DARKBLUE);
    }
};

// Custom renderer for enemy ship
class EnemyShipRenderer : public Component {
public:
    Color primaryColor = RED;
    Color accentColor = DARKPURPLE;
    
    void Render() override {
        if (!gameObject || !gameObject->transform) return;
        
        Vector2 pos = gameObject->transform->position;
        
        // Draw enemy ship as alien-looking craft
        // Main body (wider, more menacing)
        DrawTriangle(
            {pos.x, pos.y + 16},
            {pos.x - 18, pos.y - 10},
            {pos.x + 18, pos.y - 10},
            primaryColor
        );
        
        // Wings (pointed)
        DrawTriangle(
            {pos.x - 18, pos.y - 10},
            {pos.x - 24, pos.y - 8},
            {pos.x - 20, pos.y + 4},
            MAROON
        );
        DrawTriangle(
            {pos.x + 18, pos.y - 10},
            {pos.x + 24, pos.y - 8},
            {pos.x + 20, pos.y + 4},
            MAROON
        );
        
        // Cockpit/core
        DrawCircle(pos.x, pos.y, 8, accentColor);
        DrawCircle(pos.x, pos.y, 5, {255, 100, 150, 255});
        
        // Engine glow
        DrawCircle(pos.x, pos.y + 14, 4, {255, 100, 0, 200});
    }
};

// Custom bullet renderer
class BulletRenderer : public Component {
public:
    bool isPlayerBullet = true;
    
    void Render() override {
        if (!gameObject || !gameObject->transform) return;
        
        Vector2 pos = gameObject->transform->position;
        
        if (isPlayerBullet) {
            // Player bullet - blue energy bolt
            DrawCircle(pos.x, pos.y, 5, {150, 200, 255, 200});
            DrawCircle(pos.x, pos.y, 3, {200, 230, 255, 255});
            DrawRectangle(pos.x - 2, pos.y - 8, 4, 16, {100, 180, 255, 180});
            DrawCircle(pos.x, pos.y, 2, WHITE);
        } else {
            // Enemy bullet - red energy
            DrawCircle(pos.x, pos.y, 4, {255, 100, 100, 200});
            DrawCircle(pos.x, pos.y, 2, {255, 150, 150, 255});
            DrawRectangle(pos.x - 1, pos.y - 6, 2, 12, {255, 80, 80, 180});
        }
    }
};

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
        // Create starfield background (first, so it renders behind everything)
        auto background = CreateGameObject("Starfield");
        background->AddComponent<StarfieldBackground>();
        
        // Create player
        player = CreateGameObject("Player");
        player->tag = "Player";
        player->transform->position = {400, 500};
        
        // Add custom ship renderer instead of sprite
        player->AddComponent<PlayerShipRenderer>();
        
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
        
        // Use GetRandomValue directly for testing
        float x = static_cast<float>(GetRandomValue(50, 750));
        enemy->transform->position = {x, -30};
        
        // Add custom enemy ship renderer
        enemy->AddComponent<EnemyShipRenderer>();
        
        // Add collider
        auto collider = enemy->AddComponent<CircleCollider>(18.0f);
        
        // Add AI
        auto ai = enemy->AddComponent<EnemyAI>();
        ai->speed = static_cast<float>(GetRandomValue(50, 150));
        ai->amplitude = static_cast<float>(GetRandomValue(30, 100));
        ai->frequency = static_cast<float>(GetRandomValue(1, 3));
    }
    
    void SpawnBullet(Vector2 position, bool isPlayerBullet = true) {
        auto bullet = CreateGameObject("Bullet");
        bullet->tag = "Bullet";
        bullet->transform->position = position;
        
        // Add custom bullet renderer
        auto renderer = bullet->AddComponent<BulletRenderer>();
        renderer->isPlayerBullet = isPlayerBullet;
        
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

