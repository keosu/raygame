// Bullet-Hell Boss — showcases object pooling (thousands of bullets),
// parent-child transforms (boss turrets), collision via data-oriented fields,
// tweened boss movement, HUD, and camera screen-shake.
#include "RayGame.h"
#include "../common/ExampleUI.h"
#include <cmath>

using namespace RayGame;



static void DrawGlowCircle(Vector2 p, float r, Color c) {
    DrawCircleV(p, r * 2.2f, Fade(c, 0.12f));
    DrawCircleV(p, r * 1.4f, Fade(c, 0.25f));
    DrawCircleV(p, r, c);
    DrawCircleV(p, r * 0.45f, WHITE);
}

// ---- Pooled bullet field (data-oriented) ----
struct Bullet {
    Vector2 pos{0,0};
    Vector2 vel{0,0};
    float radius = 6;
    Color color = WHITE;
    bool active = false;
};

class BulletField : public Component {
public:
    ObjectPool<Bullet> pool{256};
    std::vector<Bullet*> live;

    explicit BulletField(int order = 30) { renderOrder = order; }

    void Spawn(Vector2 pos, Vector2 vel, float radius, Color color) {
        Bullet* b = pool.Acquire();
        b->pos = pos; b->vel = vel; b->radius = radius; b->color = color; b->active = true;
        live.push_back(b);
    }

    void Clear() {
        for (auto* b : live) { b->active = false; pool.Release(b); }
        live.clear();
    }

    int Count() const { return (int)live.size(); }

    void Update() override {
        float dt = GameTime::DeltaTime();
        for (size_t i = 0; i < live.size();) {
            Bullet* b = live[i];
            b->pos.x += b->vel.x * dt;
            b->pos.y += b->vel.y * dt;
            if (b->pos.x < -40 || b->pos.x > ExampleUI::Width() + 40 ||
                b->pos.y < -40 || b->pos.y > ExampleUI::Height() + 40) {
                b->active = false; pool.Release(b);
                live[i] = live.back(); live.pop_back();
            } else {
                i++;
            }
        }
    }

    void Render() override {
        for (auto* b : live) DrawGlowCircle(b->pos, b->radius, b->color);
    }

    // Returns true and removes the first bullet overlapping the circle.
    bool CheckHit(Vector2 center, float radius) {
        for (size_t i = 0; i < live.size(); i++) {
            Bullet* b = live[i];
            float dx = b->pos.x - center.x, dy = b->pos.y - center.y;
            float rr = b->radius + radius;
            if (dx*dx + dy*dy <= rr*rr) {
                b->active = false; pool.Release(b);
                live[i] = live.back(); live.pop_back();
                return true;
            }
        }
        return false;
    }
};

// ---- Turret: child of the boss, fires along its world rotation ----
class Turret : public Component {
public:
    BulletField* field = nullptr;
    float spinSpeed = 90.0f;
    float fireInterval = 0.08f;
    float timer = 0;
    Color color = {255, 90, 160, 255};

    void Update() override {
        gameObject->transform->rotation += spinSpeed * GameTime::DeltaTime();
        timer += GameTime::DeltaTime();
        if (timer >= fireInterval && field) {
            timer = 0;
            Vector2 wp = gameObject->transform->GetWorldPosition();
            float ang = gameObject->transform->GetWorldRotation() * DEG2RAD;
            float sp = 240.0f;
            field->Spawn(wp, {std::cos(ang) * sp, std::sin(ang) * sp}, 6, color);
        }
    }

    void Render() override {
        Vector2 wp = gameObject->transform->GetWorldPosition();
        DrawGlowCircle(wp, 10, color);
    }
};

class BulletHellScene;

// ---- Boss ----
class Boss : public Component {
public:
    BulletField* field = nullptr;
    BulletHellScene* game = nullptr;
    float maxHp = 1500;
    float hp = 1500;
    float patternTimer = 0;
    int phase = 0;
    float ringTimer = 0;

    Boss() { renderOrder = 20; }

    void Start() override;   // builds turret children
    void Update() override;  // movement + patterns
    void Render() override {
        Vector2 p = gameObject->transform->GetWorldPosition();
        float pulse = 1.0f + 0.08f * sinf(GameTime::GetTime() * 4);
        DrawGlowCircle(p, 44 * pulse, Color{180, 60, 255, 255});
        DrawGlowCircle(p, 22, Color{255, 120, 220, 255});
    }

    void Damage(float d);
};

// ---- Player ----
class Player : public Component {
public:
    BulletField* enemyField = nullptr;
    BulletField* playerField = nullptr;
    BulletHellScene* game = nullptr;
    float speed = 300.0f;
    float radius = 6.0f;
    float fireTimer = 0;
    float invuln = 0;
    int lives = 3;

    Player() { renderOrder = 25; }

    void Update() override;
    void Render() override {
        Vector2 p = gameObject->transform->position;
        Color c = invuln > 0 && ((int)(GameTime::GetTime()*20) % 2) ? Fade(SKYBLUE, 0.3f)
                                                                    : Color{120, 220, 255, 255};
        // ship triangle
        DrawTriangle({p.x, p.y-14}, {p.x-11, p.y+12}, {p.x+11, p.y+12}, c);
        DrawGlowCircle({p.x, p.y+4}, 4, Color{200, 240, 255, 255});
        // hitbox dot
        DrawCircleV(p, radius, Fade(WHITE, 0.9f));
    }
};

// ---- Scene ----
class BulletHellScene : public Scene {
public:
    Ref<GameObject> player;
    Ref<GameObject> boss;
    BulletField* enemyField = nullptr;
    BulletField* playerField = nullptr;
    int score = 0;
    bool gameOver = false;
    bool won = false;
    bool restartRequested = false;
    float shake = 0;

    BulletHellScene() : Scene("Bullet Hell") {}

    void OnViewportChanged(Vector2 previous, Vector2 current) override {
        for (const auto& object : GetAllGameObjects()) {
            if (!object->transform->GetParent())
                object->transform->position = ExampleUI::Remap(object->transform->position, previous, current);
            if (auto field = object->GetComponent<BulletField>())
                for (auto* bullet : field->live) bullet->pos = ExampleUI::Remap(bullet->pos, previous, current);
        }
        if (player) {
            auto& p = player->transform->position;
            p.x = Clamp(p.x, 12.0f, current.x - 12);
            p.y = Clamp(p.y, 122.0f, current.y - 58);
        }
    }

    void OnLoad() override {
        ExampleUI::Initialize(ExampleUI::pink);
        EventBus::Subscribe("shake", [this](const EventData& d){ shake = Clamp(shake + d.f, 0.0f, 1.0f); });

        auto camObj = CreateGameObject("Camera");
        auto cam = camObj->AddComponent<Camera2DComponent>();
        cam->GetCamera().offset = {0,0};
        cam->GetCamera().target = {0,0};
        activeCam = cam.get();
        SetActiveCamera(cam.get());

        auto ef = CreateGameObject("EnemyBullets");
        enemyField = ef->AddComponent<BulletField>(30).get();
        auto pf = CreateGameObject("PlayerBullets");
        playerField = pf->AddComponent<BulletField>(28).get();

        player = CreateGameObject("Player");
        player->transform->position = {ExampleUI::Width()/2.0f, ExampleUI::Height() - 120.0f};
        auto pl = player->AddComponent<Player>();
        pl->enemyField = enemyField; pl->playerField = playerField; pl->game = this;

        boss = CreateGameObject("Boss");
        boss->transform->position = {ExampleUI::Width()/2.0f, 150.0f};
        auto b = boss->AddComponent<Boss>();
        b->field = enemyField; b->game = this;
    }

    Camera2DComponent* activeCam = nullptr;

    void Update() override {
        if (!gameOver) Scene::Update();

        // camera shake
        if (activeCam) {
            float s = shake * shake * 16.0f;
            activeCam->GetCamera().offset = {
                (float)GetRandomValue(-100,100)/100.0f * s,
                (float)GetRandomValue(-100,100)/100.0f * s
            };
            shake = std::max(0.0f, shake - GameTime::DeltaTime() * 1.6f);
        }

        if (gameOver && (restartRequested || Input::GetKeyDown(KEY_R))) {
            Restart();
        }
    }

    void Restart() {
        Clear();
        score = 0; shake = 0; gameOver = false; won = false; restartRequested = false;
        Load();
    }

    void EndGame(bool victory) { if (!gameOver) { gameOver = true; won = victory; } }

    void Render() override {
        DrawRectangleGradientV(0, 0, ExampleUI::Width(), ExampleUI::Height(), {19, 12, 36, 255}, {6, 12, 24, 255});
        for (int y = 110; y < ExampleUI::Height() - 40; y += 48) {
            for (int x = 26; x < ExampleUI::Width(); x += 48) DrawCircle(x, y, 1, {53, 51, 82, 160});
        }
        Scene::Render();
        ExampleUI::Header(ExampleUI::Width(), "BULLET HELL", "BOSS ENCOUNTER / 01");
        if (player) ExampleUI::Lives(320, player->GetComponent<Player>()->lives);
        ExampleUI::Stat(ExampleUI::Width() - 190.0f, "SCORE", TextFormat("%06d", score), ExampleUI::gold);
        ExampleUI::Stat(ExampleUI::Width() - 32.0f, "BULLETS", TextFormat("%04d",
            (enemyField ? enemyField->Count() : 0) + (playerField ? playerField->Count() : 0)), ExampleUI::pink);
        ExampleUI::Footer(ExampleUI::Width(), ExampleUI::Height(), "WASD  MOVE    Z / SPACE  FIRE    SHIFT  FOCUS");

        if (boss && !gameOver) {
            auto enemy = boss->GetComponent<Boss>();
            UI::Label(28, 82, "THE SENTINEL", 13, ExampleUI::muted);
            UI::LabelRight(ExampleUI::Width() - 28, 82, TextFormat("%d%%", (int)(100 * enemy->hp / enemy->maxHp)), 13, ExampleUI::pink);
            UI::ProgressBar({28, 101, ExampleUI::Width() - 56.0f, 6}, enemy->hp / enemy->maxHp, ExampleUI::pink);
        }
        if (gameOver && ExampleUI::Result(ExampleUI::Width(), ExampleUI::Height(), won ? "VICTORY" : "DEFEATED", score, won)) {
            restartRequested = true;
        }
    }
};

// ---- Boss impl ----
void Boss::Start() {
    // Three turrets parented to the boss (parent-child transform demo).
    for (int i = 0; i < 3; i++) {
        auto t = gameObject->scene->CreateGameObject("Turret");
        t->transform->SetParent(gameObject->transform.get(), false);
        t->transform->position = {(i - 1) * 40.0f, 10.0f};  // local offset
        auto turret = t->AddComponent<Turret>();
        turret->field = field;
        turret->spinSpeed = (i == 1) ? -70.0f : 70.0f;
        turret->color = (i == 1) ? Color{255,200,80,255} : Color{255,90,160,255};
    }
}

void Boss::Update() {
    if (game && game->gameOver) return;
    // Tweened side-to-side sweep via sine.
    float t = GameTime::GetTime();
    gameObject->transform->position.x = ExampleUI::Width()/2.0f + sinf(t * 0.8f) * 240.0f;
    gameObject->transform->rotation += 20.0f * GameTime::DeltaTime();

    // Aimed ring bursts.
    ringTimer += GameTime::DeltaTime();
    float interval = (hp < maxHp * 0.4f) ? 1.1f : 1.8f;
    if (ringTimer >= interval && field) {
        ringTimer = 0;
        Vector2 p = gameObject->transform->GetWorldPosition();
        int n = (hp < maxHp * 0.4f) ? 28 : 20;
        for (int i = 0; i < n; i++) {
            float a = (2*PI / n) * i + t;
            float sp = 180.0f;
            field->Spawn(p, {std::cos(a)*sp, std::sin(a)*sp}, 7, Color{255,120,90,255});
        }
        EventBus::Emit("shake", EventData{0, 0.15f});
    }
}

void Boss::Damage(float d) {
    if (game && game->gameOver) return;
    hp -= d;
    if (hp <= 0 && game) { hp = 0; game->EndGame(true); }
}

// ---- Player impl ----
void Player::Update() {
    if (game && game->gameOver) return;
    auto& pos = gameObject->transform->position;
    if (invuln > 0) invuln -= GameTime::DeltaTime();

    Vector2 mv = Input::GetMovementInput();
    // Focus (slow) mode with shift
    float s = (Input::GetKey(KEY_LEFT_SHIFT)) ? speed * 0.45f : speed;
    pos.x += mv.x * s * GameTime::DeltaTime();
    pos.y += mv.y * s * GameTime::DeltaTime();
    pos.x = Clamp(pos.x, 12.0f, ExampleUI::Width() - 12.0f);
    pos.y = Clamp(pos.y, 122.0f, ExampleUI::Height() - 58.0f);

    // Fire
    fireTimer -= GameTime::DeltaTime();
    if ((Input::GetKey(KEY_SPACE) || Input::GetKey(KEY_Z)) && fireTimer <= 0 && playerField) {
        fireTimer = 0.06f;
        playerField->Spawn({pos.x - 8, pos.y - 10}, {0, -680}, 5, Color{120, 240, 255, 255});
        playerField->Spawn({pos.x + 8, pos.y - 10}, {0, -680}, 5, Color{120, 240, 255, 255});
    }

    // Player bullets hit boss
    if (game && game->boss && game->boss->GetComponent<Boss>()) {
        Vector2 bp = game->boss->transform->GetWorldPosition();
        while (!game->gameOver && playerField && playerField->CheckHit(bp, 42)) {
            game->boss->GetComponent<Boss>()->Damage(12);
            game->score += 10;
        }
    }

    // Enemy bullets hit player
    if ((!game || !game->gameOver) && enemyField && invuln <= 0 && enemyField->CheckHit(pos, radius)) {
        lives--;
        invuln = 1.5f;
        EventBus::Emit("shake", EventData{0, 0.6f});
        if (lives <= 0 && game) game->EndGame(false);
    }
}

class BulletHellGame : public GameEngine {
public:
    BulletHellGame() : GameEngine("RayGame - Bullet Hell Boss", 820, 900) {
        SetClearColor(Color{10, 8, 20, 255});
    }
    void OnInit() override { LoadScene(CreateRef<BulletHellScene>()); }
};

int main() {
    return ExampleUI::Run<BulletHellGame>("raygame.bullet_hell");
}
