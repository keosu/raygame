#include "RayGame.h"
#include "../common/ExampleUI.h"

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
                static_cast<float>(GetRandomValue(0, (int)ExampleUI::Width())),
                static_cast<float>(GetRandomValue(0, (int)ExampleUI::Height()))
            };
            star.size = static_cast<float>(GetRandomValue(5, 18)) / 10.0f;
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
            if (star.position.y > ExampleUI::Height()) {
                star.position.y = 0;
                star.position.x = static_cast<float>(GetRandomValue(0, (int)ExampleUI::Width()));
            }
        }
    }
    
    void Render() override {
        // Ensure initialization (defensive programming)
        if (!initialized) {
            InitializeStars();
        }
        if (stars.empty()) return;
        
        DrawRectangleGradientV(0, 0, (int)std::ceil(ExampleUI::Width()), (int)std::ceil(ExampleUI::Height()), {8, 16, 32, 255}, {3, 7, 17, 255});

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
        ExampleUI::Triangle(leftWing, rightWing, nose, primaryColor);
        
        // Cockpit
        DrawCircle(pos.x, pos.y - 5, 5, accentColor);
        
        // Wings detail
        ExampleUI::Triangle(
            {pos.x - 16, pos.y + 12},
            {pos.x - 12, pos.y + 8},
            {pos.x - 10, pos.y + 12},
            accentColor
        );
        ExampleUI::Triangle(
            {pos.x + 16, pos.y + 12},
            {pos.x + 12, pos.y + 8},
            {pos.x + 10, pos.y + 12},
            accentColor
        );
        
        // Tail fins
        ExampleUI::Triangle(leftTail, {pos.x - 4, pos.y + 10}, {pos.x - 8, pos.y + 10}, DARKBLUE);
        ExampleUI::Triangle(rightTail, {pos.x + 4, pos.y + 10}, {pos.x + 8, pos.y + 10}, DARKBLUE);
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
        ExampleUI::Triangle(
            {pos.x, pos.y + 16},
            {pos.x - 18, pos.y - 10},
            {pos.x + 18, pos.y - 10},
            primaryColor
        );
        
        // Wings (pointed)
        ExampleUI::Triangle(
            {pos.x - 18, pos.y - 10},
            {pos.x - 24, pos.y - 8},
            {pos.x - 20, pos.y + 4},
            MAROON
        );
        ExampleUI::Triangle(
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
        if (gameObject->transform->position.y > ExampleUI::Height() + 100) {
            gameObject->MarkDestroyed();
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
            gameObject->transform->position.y > ExampleUI::Height() + 100) {
            gameObject->MarkDestroyed();
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
    bool restartRequested = false;
    
    SpaceShooterScene() : Scene("Space Shooter") {}

    void OnViewportChanged(Vector2 previous, Vector2 current) override {
        for (const auto& object : GetAllGameObjects()) {
            object->transform->position = ExampleUI::Remap(object->transform->position, previous, current);
            if (auto ai = object->GetComponent<EnemyAI>()) ai->startX *= current.x / previous.x;
            if (auto background = object->GetComponent<StarfieldBackground>())
                for (auto& star : background->stars) star.position = ExampleUI::Remap(star.position, previous, current);
            if (auto particles = object->GetComponent<ParticleEmitter>()) particles->Clear();
        }
        if (player && !player->IsDestroyed()) {
            auto& p = player->transform->position;
            p.x = Clamp(p.x, 20.0f, current.x - 20);
            p.y = Clamp(p.y, 92.0f, current.y - 54);
        }
    }
    
    void OnLoad() override {
        ExampleUI::Initialize();
        // Create starfield background (first, so it renders behind everything)
        auto background = CreateGameObject("Starfield");
        background->AddComponent<StarfieldBackground>();
        
        // Create player
        player = CreateGameObject("Player");
        player->tag = "Player";
        player->transform->position = {ExampleUI::Width() / 2, ExampleUI::Height() - 100};
        
        // Add custom ship renderer instead of sprite
        player->AddComponent<PlayerShipRenderer>();
        
        // Add physics
        auto rb = player->AddComponent<Rigidbody>();
        rb->drag = 0.9f;
        
        auto collider = player->AddComponent<CircleCollider>(16.0f);
        collider->isTrigger = true;
        
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
        camera->offset = {ExampleUI::Width() / 2, ExampleUI::Height() / 2};
        camera->zoom = 1.0f;
        
        // Create some initial enemies
        for (int i = 0; i < 3; i++) {
            SpawnEnemy();
        }
    }
    
    void Update() override {
        Scene::Update();
        
        if (!player || player->IsDestroyed()) {
            if (restartRequested || Input::GetKeyDown(KEY_R)) Restart();
            return;
        }

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
                if (!enemyCollider || enemy->IsDestroyed()) continue;
                
                if (bulletCollider->CheckCollision(enemyCollider.get())) {
                    // Destroy both
                    bullet->MarkDestroyed();
                    enemy->MarkDestroyed();
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
                    if (!enemyCollider || enemy->IsDestroyed()) continue;
                    
                    if (playerCollider->CheckCollision(enemyCollider.get())) {
                        // Game over effect
                        SpawnExplosion(player->transform->position);
                        player->MarkDestroyed();
                        break;
                    }
                }
            }
        }
    }
    
    void Render() override {
        Scene::Render();
        
        ExampleUI::Header(ExampleUI::Width(), "SPACE SHOOTER", "ORBITAL DEFENSE");
        ExampleUI::Stat(ExampleUI::Width() - 32, "SCORE", TextFormat("%06d", score));
        ExampleUI::Footer(ExampleUI::Width(), ExampleUI::Height(), "WASD / ARROWS  MOVE     SPACE  FIRE", "DRAG TO MOVE / AUTO FIRE");
        if (!player || !player->active) {
            if (ExampleUI::Result(ExampleUI::Width(), ExampleUI::Height(), "SECTOR LOST", score)) restartRequested = true;
        }
    }

    void Restart() {
        OnUnload();
        Clear();
        score = 0;
        enemySpawnTimer = 0;
        enemySpawnRate = 2.0f;
        restartRequested = false;
        OnLoad();
        Start();
    }

    void SpawnEnemy() {
        auto enemy = CreateGameObject("Enemy");
        enemy->tag = "Enemy";
        
        // Use GetRandomValue directly for testing
        float x = static_cast<float>(GetRandomValue(100, (int)ExampleUI::Width() - 100));
        enemy->transform->position = {x, -30};
        
        // Add custom enemy ship renderer
        enemy->AddComponent<EnemyShipRenderer>();
        
        // Add collider
        auto collider = enemy->AddComponent<CircleCollider>(18.0f);
        collider->isTrigger = true;
        
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
        
        explosion->AddComponent<DestroyAfter>(1.0f);
    }
};

// PlayerController implementations (after SpaceShooterScene is defined)
void PlayerController::Update() {
    if (!gameObject) return;
    
    // Movement
#if defined(__ANDROID__)
    bool touch = ExampleUI::MoveWithTouch(gameObject->transform->position, speed);
    if (touch) if (auto body = gameObject->GetComponent<Rigidbody>()) body->SetVelocity({0, 0});
#else
    bool touch = false;
#endif
    Vector2 input = touch ? Vector2{0, 0} : Input::GetMovementInput();
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
    
    if ((Input::GetKey(KEY_SPACE) || touch) && fireTimer <= 0) {
        Shoot();
        fireTimer = fireRate;
    }
    
    // Keep in bounds
    float margin = 20;
    if (gameObject->transform->position.x < margin) 
        gameObject->transform->position.x = margin;
    if (gameObject->transform->position.x > ExampleUI::Width() - margin)
        gameObject->transform->position.x = ExampleUI::Width() - margin;
    if (gameObject->transform->position.y < 92)
        gameObject->transform->position.y = 92;
    if (gameObject->transform->position.y > ExampleUI::Height() - 54)
        gameObject->transform->position.y = ExampleUI::Height() - 54;
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
    return ExampleUI::Run<SpaceShooterGame>("raygame.space_shooter");
}

