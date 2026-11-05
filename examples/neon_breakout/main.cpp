// Neon Breakout — showcases layered rendering, tweens, timers, collision
// callbacks, particle trails, camera screen-shake, event bus, HUD and save.
#include "RayGame.h"
#include "../common/ExampleUI.h"
#include <cmath>

using namespace RayGame;



// ---- Neon drawing helpers ----
static void DrawGlowCircle(Vector2 p, float r, Color c) {
    for (int i = 3; i >= 1; i--) {
        float rr = r * (1.0f + i * 0.6f);
        DrawCircleV(p, rr, Fade(c, 0.10f));
    }
    DrawCircleV(p, r, c);
    DrawCircleV(p, r * 0.5f, WHITE);
}

static void DrawGlowRect(Rectangle rect, Color c, float round = 0.3f) {
    Rectangle glow = {rect.x - 4, rect.y - 4, rect.width + 8, rect.height + 8};
    DrawRectangleRounded(glow, round, 6, Fade(c, 0.25f));
    DrawRectangleRounded(rect, round, 6, Fade(c, 0.85f));
    DrawRectangleRoundedLines(rect, round, 6, WHITE);
}

// ---- Camera screen-shake, driven by the "shake" event ----
class ShakeCamera : public Component {
public:
    Camera2DComponent* cam = nullptr;
    float trauma = 0.0f;
    unsigned long subscription = 0;

    void Start() override {
        cam = gameObject->GetComponent<Camera2DComponent>().get();
        subscription = EventBus::Subscribe("shake", [this](const EventData& d) {
            trauma = Clamp(trauma + (d.f > 0 ? d.f : 0.4f), 0.0f, 1.0f);
        });
    }

    void OnDestroy() override { EventBus::Unsubscribe("shake", subscription); }

    void LateUpdate() override {
        if (!cam) return;
        float shake = trauma * trauma;
        float mag = 14.0f * shake;
        cam->offset = {
            (float)GetRandomValue(-100, 100) / 100.0f * mag,
            (float)GetRandomValue(-100, 100) / 100.0f * mag
        };
        cam->GetCamera().target = {0, 0};
        cam->GetCamera().offset = cam->offset;
        cam->GetCamera().zoom = 1.0f;
        trauma = std::max(0.0f, trauma - GameTime::DeltaTime() * 1.5f);
    }
};

// ---- Background animated grid ----
class NeonGrid : public Component {
public:
    NeonGrid() { renderOrder = -100; }
    void Render() override {
        float t = GameTime::GetTime();
        Color line = {40, 60, 120, 60};
        for (int x = 0; x <= ExampleUI::Width(); x += 48) {
            float a = 30 + 20 * sinf(t + x * 0.02f);
            DrawLine(x, 0, x, ExampleUI::Height(), Fade(line, a / 255.0f));
        }
        for (int y = 0; y <= ExampleUI::Height(); y += 48) {
            float a = 30 + 20 * cosf(t + y * 0.02f);
            DrawLine(0, y, ExampleUI::Width(), y, Fade(line, a / 255.0f));
        }
    }
};

// ---- Brick ----
class Brick : public Component {
public:
    Color color = SKYBLUE;
    Vector2 size = {80, 28};
    int points = 10;

    Brick() { renderOrder = 0; }

    void Render() override {
        Vector2 p = gameObject->transform->GetRenderPosition();
        Vector2 scale = gameObject->transform->GetRenderScale();
        Vector2 extent = {size.x * scale.x, size.y * scale.y};
        Rectangle r = {p.x - extent.x/2, p.y - extent.y/2, extent.x, extent.y};
        DrawGlowRect(r, color);
    }
};

// ---- Particle burst that self-destructs ----
static void SpawnBurst(Scene* scene, Vector2 pos, Color color) {
    auto fx = scene->CreateGameObject("Burst");
    fx->transform->position = pos;
    auto p = fx->AddComponent<ParticleEmitter>();
    p->renderOrder = 50;
    p->emissionRate = 0;
    p->maxParticles = 24;
    p->shape = EmitterShape::Point;
    p->startVelocityMin = {-220, -220};
    p->startVelocityMax = {220, 220};
    p->acceleration = {0, 300};
    p->lifetimeMin = 0.25f; p->lifetimeMax = 0.6f;
    p->startSizeMin = 2; p->startSizeMax = 5;
    p->startColor = color;
    p->endColor = Fade(color, 0);
    p->loop = false; p->emitting = false;
    p->Burst(24);
    fx->AddComponent<DestroyAfter>(0.7f);
}

// ---- Paddle ----
class Paddle : public Component {
public:
    float speed = 640.0f;
    Vector2 size = {120, 18};

    Paddle() { renderOrder = 10; }
    void Start() override { gameObject->transform->SetInterpolationEnabled(true); }

    void FixedUpdate() override {
        auto& pos = gameObject->transform->position;
        float move = Input::GetAxis("Horizontal");
        pos.x += move * speed * GameTime::DeltaTime();
        // Mouse control too
        if (move == 0 && Input::IsMouseInCanvas() && std::fabs(Input::GetMouseDelta().x) > 0) {
            pos.x = Input::GetMousePosition().x;
        }
#if defined(__ANDROID__)
        if (ExampleUI::TouchInPlayfield()) pos.x = Input::GetTouchPosition(0).x;
#endif
        pos.x = Clamp(pos.x, size.x/2, ExampleUI::Width() - size.x/2);
        auto box = gameObject->GetComponent<BoxCollider>();
        if (box) box->size = size;
    }

    void Render() override {
        Vector2 p = gameObject->transform->GetRenderPosition();
        Rectangle r = {p.x - size.x/2, p.y - size.y/2, size.x, size.y};
        DrawGlowRect(r, Color{80, 220, 255, 255}, 0.6f);
    }
};

class BreakoutScene;  // fwd

// ---- Ball ----
class Ball : public Component {
public:
    Vector2 velocity = {0, 0};
    float speed = 460.0f;
    float radius = 10.0f;
    bool launched = false;
    BreakoutScene* game = nullptr;
    std::vector<Vector2> trail;

    Ball() { renderOrder = 20; }

    void Start() override { gameObject->transform->SetInterpolationEnabled(true); }
    void FixedUpdate() override;   // defined after scene
    void Render() override {
        Vector2 p = gameObject->transform->GetRenderPosition();
        for (size_t i = 0; i < trail.size(); i++) {
            float a = (float)i / trail.size();
            DrawCircleV(trail[i], radius * a, Fade(Color{120, 240, 255, 255}, a * 0.4f));
        }
        DrawGlowCircle(p, radius * gameObject->transform->GetRenderScale().x, Color{140, 240, 255, 255});
    }

    void OnTriggerEnter(Collider* other) override;  // defined after scene
};

// ---- Scene ----
class BreakoutScene : public Scene {
public:
    Ref<GameObject> paddle;
    Ref<GameObject> ball;
    int score = 0;
    int lives = 3;
    int bricksLeft = 0;
    int highscore = 0;
    bool gameOver = false;
    float overTime = 0; // real seconds since the round ended, for the result panel's entrance
    bool won = false;
    bool restartRequested = false;
    std::filesystem::path savePath;

    BreakoutScene() : Scene("Neon Breakout") {}

    void OnViewportChanged(Vector2 previous, Vector2 current) override {
        for (const auto& object : GetAllGameObjects()) {
            if (object->tag == "Brick") object->transform->position.x += (current.x - previous.x) / 2;
            if (auto particles = object->GetComponent<ParticleEmitter>()) particles->Clear();
        }
        if (paddle) {
            auto& p = paddle->transform->position;
            p = {Clamp(p.x * current.x / previous.x, 60.0f, current.x - 60), current.y - 60};
        }
        if (ball) {
            auto& p = ball->transform->position;
            p = ExampleUI::Remap(p, previous, current);
            p.x = Clamp(p.x, 10.0f, current.x - 10);
            p.y = Clamp(p.y, 10.0f, current.y - 80);
            if (!ball->GetComponent<Ball>()->launched && paddle) p = {paddle->transform->position.x, current.y - 90};
        }
    }

    void OnLoad() override {
        ExampleUI::Initialize(ExampleUI::cyan, "neon_breakout");
        SaveSystem save;
        if (savePath.empty()) {
            savePath = FileSystem::UserDataDirectory("raygame.neon_breakout") / "breakout_save.txt";
            if (!std::filesystem::exists(savePath)) {
                for (const auto& legacy : {std::filesystem::path("breakout_save.txt"),
                     std::filesystem::u8path(GetApplicationDirectory()) / "breakout_save.txt"}) {
                    if (save.Load(legacy)) { save.Save(savePath); break; }
                }
            }
        }
        if (save.Load(savePath)) highscore = save.GetInt("highscore", 0);

        EventBus::Subscribe("score", [this](const EventData& d) { score += d.i; });

        CreateGameObject("Grid")->AddComponent<NeonGrid>();

        // Camera with shake
        auto camObj = CreateGameObject("Camera");
        auto cam = camObj->AddComponent<Camera2DComponent>();
        cam->GetCamera().offset = {0, 0};
        cam->GetCamera().target = {0, 0};
        auto shake = camObj->AddComponent<ShakeCamera>();
        (void)shake;
        SetActiveCamera(cam.get());

        // Paddle
        paddle = CreateGameObject("Paddle");
        paddle->tag = "Paddle";
        paddle->transform->position = {ExampleUI::Width() / 2.0f, ExampleUI::Height() - 60.0f};
        paddle->AddComponent<Paddle>();
        auto pbox = paddle->AddComponent<BoxCollider>(Vector2{120, 18});
        pbox->isTrigger = true;

        SpawnBall();
        BuildBricks();
    }

    void BuildBricks() {
        const int cols = 10, rows = 6;
        const float bw = 80, bh = 28, gap = 8;
        float totalW = cols * bw + (cols - 1) * gap;
        float startX = (ExampleUI::Width() - totalW) / 2 + bw / 2;
        Color rowColors[] = {
            {255, 80, 160, 255}, {255, 140, 60, 255}, {255, 230, 60, 255},
            {90, 240, 140, 255}, {80, 200, 255, 255}, {170, 120, 255, 255}
        };
        bricksLeft = 0;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                auto b = CreateGameObject("Brick");
                b->tag = "Brick";
                b->transform->position = {startX + c * (bw + gap), 90.0f + r * (bh + gap)};
                auto brick = b->AddComponent<Brick>();
                brick->color = rowColors[r];
                brick->size = {bw, bh};
                brick->points = (rows - r) * 10;
                std::weak_ptr<GameObject> weakBrick = b;
                b->transform->scale = {0.2f, 0.2f};
                Tween::To(0.2f, 1.0f, 0.3f + r * 0.05f, Ease::BackOut,
                    [weakBrick](float value) {
                        auto object = weakBrick.lock();
                        if (object && !object->IsDestroyed()) object->transform->scale = {value, value};
                    });
                auto col = b->AddComponent<BoxCollider>(Vector2{bw, bh});
                col->isTrigger = true;
                bricksLeft++;
            }
        }
    }

    void SpawnBall() {
        ball = CreateGameObject("Ball");
        ball->tag = "Ball";
        ball->transform->position = {ExampleUI::Width() / 2.0f, ExampleUI::Height() - 90.0f};
        auto b = ball->AddComponent<Ball>();
        b->game = this;
        auto col = ball->AddComponent<CircleCollider>(10.0f);
        col->isTrigger = true;
        // The served ball pops onto the paddle instead of appearing at full size.
        std::weak_ptr<GameObject> weakBall = ball;
        ball->transform->scale = {0, 0};
        Tween::To(0.0f, 1.0f, 0.35f, Ease::BackOut, [weakBall](float value) {
            auto object = weakBall.lock();
            if (object && !object->IsDestroyed()) object->transform->scale = {value, value};
        });
    }

    void OnBrickDestroyed(Vector2 pos, Color color, int points) {
        SpawnBurst(this, pos, color);
        EventBus::Emit("score", EventData{points});
        EventBus::Emit("shake", EventData{0, 0.35f});
        bricksLeft--;
        if (bricksLeft <= 0 && !won) { won = true; EndGame(true); }
    }

    void LoseLife() {
        if (gameOver) return;
        lives--;
        EventBus::Emit("shake", EventData{0, 0.7f});
        if (lives <= 0) { EndGame(false); }
        else { if (ball) Destroy(ball); SpawnBall(); }
    }

    void EndGame(bool victory) {
        gameOver = true;
        won = victory;
        if (score > highscore) {
            highscore = score;
            SaveSystem save;
            save.SetInt("highscore", highscore);
            save.Save(savePath);
        }
    }

    void Restart() {
        Clear();
        score = 0; lives = 3; gameOver = false; overTime = 0; won = false; restartRequested = false;
        Load();
    }

    void Update() override {
        Scene::Update();
        if (gameOver) overTime += GameTime::UnscaledDeltaTime();
        if (gameOver && (restartRequested || Input::GetKeyDown(KEY_R))) Restart();
    }

    void Render() override {
        Scene::Render();  // world (shaken)

        ExampleUI::Header(ExampleUI::Width(), "NEON BREAKOUT", "BREAK THE PATTERN");
        ExampleUI::Lives(380, lives);
        ExampleUI::Stat(ExampleUI::Width() - 195.0f, "SCORE", TextFormat("%06d", score));
        ExampleUI::Stat(ExampleUI::Width() - 32.0f, "BEST", TextFormat("%06d", highscore), ExampleUI::gold);
        ExampleUI::Footer(ExampleUI::Width(), ExampleUI::Height(), "MOUSE / A D  MOVE     SPACE / CLICK  LAUNCH", "DRAG PADDLE / TAP TO LAUNCH");

        if (!gameOver && ball && !ball->GetComponent<Ball>()->launched) {
            UI::TitleCentered(ExampleUI::Width() / 2, ExampleUI::Height() - 180, "READY TO LAUNCH", 32);
            UI::LabelCentered(ExampleUI::Width() / 2, ExampleUI::Height() - 142, "Press Space or click to begin", 20, ExampleUI::muted);
        }
        if (gameOver && ExampleUI::Result(ExampleUI::Width(), ExampleUI::Height(), won ? "ALL CLEAR" : "GAME OVER", score, won,
                                          ExampleUI::ResultAppear(overTime, 0.6f))) {
            restartRequested = true;
        }
    }
};

// ---- Ball methods (need full scene) ----
void Ball::FixedUpdate() {
    if (game && game->gameOver) return;
    auto& pos = gameObject->transform->position;

    if (!launched) {
        if (game && game->paddle) pos = {game->paddle->transform->position.x, ExampleUI::Height() - 90.0f};
        if (Input::GetKeyDown(KEY_SPACE) || (Input::IsMouseInCanvas() && Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT))) {
            launched = true;
            velocity = Normalize({(float)GetRandomValue(-40, 40), -100});
            velocity = {velocity.x * speed, velocity.y * speed};
        }
        return;
    }

    pos.x += velocity.x * GameTime::DeltaTime();
    pos.y += velocity.y * GameTime::DeltaTime();

    // Wall bounces
    if (pos.x - radius < 0) { pos.x = radius; velocity.x = std::fabs(velocity.x); }
    if (pos.x + radius > ExampleUI::Width()) { pos.x = ExampleUI::Width() - radius; velocity.x = -std::fabs(velocity.x); }
    if (pos.y - radius < 0) { pos.y = radius; velocity.y = std::fabs(velocity.y); }

    // Trail
    trail.push_back(pos);
    if (trail.size() > 12) trail.erase(trail.begin());

    // Fell out
    if (pos.y - radius > ExampleUI::Height()) {
        if (game) game->LoseLife();
    }
}

void Ball::OnTriggerEnter(Collider* other) {
    if (!launched || (game && game->gameOver) || !other || !other->GetGameObject()) return;
    auto* obj = other->GetGameObject();
    if (obj->IsDestroyed()) return;
    Vector2& pos = gameObject->transform->position;

    if (obj->tag == "Paddle") {
        Vector2 pp = obj->transform->position;
        float offset = Clamp((pos.x - pp.x) / 60.0f, -1.0f, 1.0f);
        Vector2 dir = Normalize({offset, -1.0f});
        velocity = {dir.x * speed, dir.y * speed};
        auto box = obj->GetComponent<BoxCollider>();
        pos.y = pp.y - (box ? box->size.y / 2 : 9) - radius;
    } else if (obj->tag == "Brick") {
        auto brick = obj->GetComponent<Brick>();
        Vector2 bp = obj->transform->GetWorldPosition();
        // Decide reflection axis by overlap region.
        float dx = (pos.x - bp.x) / (brick->size.x / 2);
        float dy = (pos.y - bp.y) / (brick->size.y / 2);
        if (std::fabs(dx) > std::fabs(dy)) velocity.x = -velocity.x;
        else velocity.y = -velocity.y;

        if (game) game->OnBrickDestroyed(bp, brick->color, brick->points);
        obj->MarkDestroyed();
    }
}

class NeonBreakoutGame : public GameEngine {
public:
    NeonBreakoutGame() : GameEngine("RayGame - Neon Breakout", 960, 720) {
        SetClearColor(Color{8, 10, 24, 255});
    }
    void OnInit() override { LoadScene(CreateRef<BreakoutScene>()); }
};

int main() {
    return ExampleUI::Run<NeonBreakoutGame>("raygame.neon_breakout");
}
