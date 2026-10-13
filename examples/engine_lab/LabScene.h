#pragma once
#include "RayGame.h"

namespace Lab {
using namespace RayGame;
inline constexpr Color Cyan{86, 215, 218, 255}, Gold{255, 192, 101, 255}, Pink{238, 121, 168, 255};
inline const char* Names[] = {"Objects", "Physics", "Sprites", "Effects", "Tilemap", "Services"};
inline const char* Descriptions[] = {
    "Parent / child transforms, component lifecycle and deferred destruction.",
    "Rigid bodies, circle / box contacts, triggers and camera follow.",
    "Shared texture handles, live replacement and frame animation.",
    "Particles, repeating timers, scoped events, tweens and object pools.",
    "A real Tilemap component. Click a tile to paint and query solidity.",
    "System speech, input, versioned saves and per-user file storage."};

struct Metrics {
    int awake = 0, starts = 0, destroyed = 0, contacts = 0, triggers = 0, events = 0;
};
class Shape : public Component {
public:
    Vector2 size{40, 40};
    Color color = Cyan;
    bool circle = false;
    Shape(Vector2 dimensions, Color tint, bool round = false)
        : size(dimensions), color(tint), circle(round) {}
    void Render() override {
        auto p = gameObject->transform->GetRenderPosition();
        auto scale = gameObject->transform->GetRenderScale();
        if (circle)
            DrawCircleV(p, size.x * std::fabs(scale.x) / 2, color);
        else
            DrawRectanglePro({p.x, p.y, size.x * scale.x, size.y * scale.y},
                             {size.x * scale.x / 2, size.y * scale.y / 2},
                             gameObject->transform->GetRenderRotation(), color);
    }
};
class Probe : public Component {
    Ref<Metrics> metrics;

public:
    explicit Probe(Ref<Metrics> counters) : metrics(std::move(counters)) {}
    void Awake() override {
        ++metrics->awake;
    }
    void Start() override {
        ++metrics->starts;
    }
    void OnDestroy() override {
        ++metrics->destroyed;
    }
    void OnCollisionEnter(Collider*) override {
        ++metrics->contacts;
    }
    void OnTriggerEnter(Collider*) override {
        ++metrics->triggers;
    }
};
class Spin : public Component {
public:
    void FixedUpdate() override {
        gameObject->transform->rotation += 45 * GameTime::DeltaTime();
    }
    void Start() override {
        gameObject->transform->SetInterpolationEnabled(true);
    }
};
struct Pulse {
    float age = 0;
    Vector2 position{};
};

class LabScene : public Scene {
public:
    int page;
    bool ready = false, alternate = false;
    Ref<Metrics> metrics = CreateRef<Metrics>();
    Ref<GameObject> subject, satellite, temporary;
    Ref<Spin> spin;
    Ref<Camera2DComponent> camera;
    Ref<SpriteRenderer> sprite;
    Ref<SpriteAnimation> animation;
    Ref<ParticleEmitter> emitter;
    Ref<TimerComponent> timer;
    Ref<Tilemap> map;
    TextureHandle sheet;
    ObjectPool<Pulse> pool{16};
    std::vector<Pulse*> pulses;
    Vector2 selectedTile{0, 0};
    std::string assetRoot;
    Rectangle viewport{262, 112, 990, 412};
    explicit LabScene(int selected, std::string assets)
        : Scene(Names[selected]), page(selected), assetRoot(std::move(assets)) {}

    std::string Asset(const std::string& name) const {
        // AssetManager requires APK-relative names without a leading "./".
        return assetRoot.empty() ? name : assetRoot + "/" + name;
    }

    Ref<GameObject> Object(const std::string& name, Vector2 position, Vector2 size, Color color,
                           bool circle = false) {
        auto object = CreateGameObject(name);
        object->transform->position = position;
        object->AddComponent<Shape>(size, color, circle);
        object->AddComponent<Probe>(metrics);
        return object;
    }
    void SpawnBody() {
        if (FindGameObjectsByTag("body").size() >= 12)
            return;
        int n = static_cast<int>(FindGameObjectsByTag("body").size());
        auto object = Object("body", {-220.0f + (n % 4) * 85, -135}, {40, 40}, n % 2 ? Gold : Cyan,
                             n % 2 == 0);
        object->tag = "body";
        auto body = object->AddComponent<Rigidbody>();
        body->useGravity = true;
        body->gravity = 400;
        body->drag = 1;
        body->restitution = 0.75f;
        body->velocity = {115, 0};
        if (n % 2)
            object->AddComponent<BoxCollider>(Vector2{40, 40});
        else
            object->AddComponent<CircleCollider>(20.0f);
        if (!subject)
            subject = object;
    }
    void Burst() {
        EventBus::Emit("lab.pulse");
    }
    void OnLoad() override {
        camera = CreateGameObject("camera")->AddComponent<Camera2DComponent>();
        SetActiveCamera(camera.get());
        if (page == 0) {
            subject = Object("parent", {0, 0}, {72, 72}, Cyan);
            spin = subject->AddComponent<Spin>();
            satellite = Object("child", {170, 0}, {42, 42}, Gold, true);
            satellite->transform->SetParent(subject->transform.get(), false);
            satellite->transform->SetInterpolationEnabled(true);
        } else if (page == 1) {
            for (auto r : {Rectangle{0, 185, 740, 26}, {-370, 0, 24, 400}, {370, 0, 24, 400}}) {
                auto wall = Object("wall", {r.x, r.y}, {r.width, r.height}, {52, 73, 90, 255});
                wall->AddComponent<BoxCollider>(Vector2{r.width, r.height});
            }
            auto trigger = CreateGameObject("trigger");
            trigger->transform->position = {0, 110};
            trigger->AddComponent<CircleCollider>(80.0f)->isTrigger = true;
            SpawnBody();
            SpawnBody();
            SpawnBody();
        } else if (page == 2 || page == 4) {
            sheet = ResourceManager::LoadTexture("lab.sheet", Asset("sheet.png"));
            if (page == 2) {
                subject = CreateGameObject("sprite");
                subject->transform->position = {-160, 0};
                subject->transform->scale = {2.4f, 2.4f};
                sprite = subject->AddComponent<SpriteRenderer>(sheet, Rectangle{0, 0, 48, 48});
                satellite = CreateGameObject("animation");
                satellite->transform->position = {160, 0};
                satellite->transform->scale = {2.4f, 2.4f};
                animation = satellite->AddComponent<SpriteAnimation>();
                animation->spriteSheet = sheet;
                animation->CreateFramesFromGrid(48, 48, 4, 0.15f);
            } else {
                subject = CreateGameObject("tilemap");
                subject->transform->position = {-336, -192};
                map = subject->AddComponent<Tilemap>();
                map->tileset = sheet;
                map->tileWidth = map->tileHeight = 48;
                map->Resize(14, 8);
                map->solidTiles.insert(1);
                for (int y = 0; y < 8; ++y)
                    for (int x = 0; x < 14; ++x)
                        map->SetTile(x, y, (x + y) % 4);
            }
        } else if (page == 3) {
            subject = CreateGameObject("emitter");
            emitter = subject->AddComponent<ParticleEmitter>();
            emitter->shape = EmitterShape::Cone;
            emitter->emissionRate = 35;
            emitter->maxParticles = 240;
            emitter->startColor = Cyan;
            emitter->endColor = Fade(Cyan, 0);
            emitter->startVelocityMin = {-55, -100};
            emitter->startVelocityMax = {55, -180};
            emitter->acceleration = {0, 80};
            EventBus::Subscribe("lab.pulse", [this](const EventData&) {
                ++metrics->events;
                emitter->Burst(32);
                if (pulses.size() < 16) {
                    auto pulse = pool.Acquire();
                    *pulse = {0, subject->transform->position};
                    pulses.push_back(pulse);
                }
                auto spark =
                    Object("temporary spark", subject->transform->position, {8, 8}, Gold, true);
                spark->AddComponent<DestroyAfter>(0.3f);
            });
            timer = CreateGameObject("timer")->AddComponent<TimerComponent>(
                0.7f, [this] { Burst(); }, true);
            TweenInstance tween;
            tween.from = -230;
            tween.to = 230;
            tween.duration = 2;
            tween.loop = true;
            tween.pingpong = true;
            tween.ease = Ease::SineInOut;
            tween.onUpdate = [weak = std::weak_ptr<GameObject>(subject)](float x) {
                if (auto object = weak.lock())
                    object->transform->position.x = x;
            };
            TweenManager::Add(tween);
        } else {
            AudioManager::LoadSound("lab.chime", Asset("chime.wav"));
            subject = Object("service node", {0, 0}, {80, 80}, Cyan, true);
            for (int i = 0; i < 4; ++i) {
                float a = i * PI / 2;
                Object("service", {std::cos(a) * 190, std::sin(a) * 130}, {35, 35},
                       i % 2 ? Gold : Pink);
            }
        }
        ready = true;
    }
    void OnUnload() override {
        pulses.clear();
        pool.Reset();
        ready = false;
    }
    void Update() override {
        Scene::Update();
        for (auto it = pulses.begin(); it != pulses.end();) {
            (*it)->age += GameTime::DeltaTime();
            if ((*it)->age > 1) {
                pool.Release(*it);
                it = pulses.erase(it);
            } else
                ++it;
        }
    }
    void Render() override {
        if (!ready)
            return;
        auto canvas = Display::GetCanvasTexture();
        float sx = canvas.width / Display::GetWidth(), sy = canvas.height / Display::GetHeight();
        BeginScissorMode(static_cast<int>(viewport.x * sx), static_cast<int>(viewport.y * sy),
                         static_cast<int>(viewport.width * sx),
                         static_cast<int>(viewport.height * sy));
        struct Clip {
            ~Clip() {
                EndScissorMode();
            }
        } clip;
        camera->Begin();
        for (int x = -400; x <= 400; x += 40)
            DrawLine(x, -210, x, 210, {25, 42, 55, 255});
        for (int y = -200; y <= 200; y += 40)
            DrawLine(-400, y, 400, y, {25, 42, 55, 255});
        if (page == 0) {
            DrawCircleLines(0, 0, 170, Fade(Gold, 0.3f));
            DrawLineEx(subject->transform->GetRenderPosition(),
                       satellite->transform->GetRenderPosition(), 2, Fade(Gold, 0.7f));
        }
        if (page == 1) {
            DrawCircleV({0, 110}, 80, Fade(Pink, 0.07f));
            DrawCircleLines(0, 110, 80, Pink);
        }
        if (page == 5)
            for (int i = 0; i < 4; ++i) {
                float a = i * PI / 2;
                DrawLineEx({0, 0}, {std::cos(a) * 190, std::sin(a) * 130}, 2, Fade(Cyan, 0.4f));
            }
        camera->End();
        Scene::Render();
        camera->Begin();
        for (const auto* pulse : pulses)
            DrawCircleLines(static_cast<int>(pulse->position.x),
                            static_cast<int>(pulse->position.y), 15 + pulse->age * 100,
                            Fade(Gold, 1 - pulse->age));
        if (map) {
            auto p = map->TileToWorld(static_cast<int>(selectedTile.x),
                                      static_cast<int>(selectedTile.y));
            DrawRectangleLinesEx({p.x, p.y, 48, 48}, 3, WHITE);
        }
        camera->End();
    }
    void Layout(Rectangle area) {
        viewport = area;
        if (!camera)
            return;
        camera->offset = {area.x + area.width / 2, area.y + area.height / 2};
        camera->zoom = std::min(area.width / 820, area.height / 440);
        camera->GetCamera().offset = camera->offset;
        camera->GetCamera().zoom = camera->zoom;
    }
    void Paint(Vector2 screen) {
        if (!map)
            return;
        auto local = subject->transform->InverseTransformPoint(camera->ScreenToWorld(screen));
        int x = static_cast<int>(std::floor(local.x / 48)),
            y = static_cast<int>(std::floor(local.y / 48));
        if (x < 0 || y < 0 || x >= map->columns || y >= map->rows)
            return;
        selectedTile = {static_cast<float>(x), static_cast<float>(y)};
        map->SetTile(x, y, (map->GetTile(x, y) + 1) % 4);
    }
    void Action(int action) {
        if (!ready)
            return;
        if (page == 0) {
            if (action == 0)
                spin->SetEnabled(!spin->IsEnabled());
            if (action == 1) {
                if (temporary && !temporary->IsDestroyed())
                    Destroy(temporary);
                else
                    temporary = Object("created object", {-240, 110}, {45, 45}, Pink);
            }
            if (action == 2)
                subject->transform->scale =
                    subject->transform->scale.x == 1 ? Vector2{0.65f, 0.65f} : Vector2{1, 1};
        } else if (page == 1) {
            if (action == 0)
                for (auto object : FindGameObjectsByTag("body"))
                    object->GetComponent<Rigidbody>()->AddImpulse({0, -260});
            if (action == 1)
                SpawnBody();
            if (action == 2) {
                camera->SetTarget(camera->followEnabled ? nullptr : subject.get());
                if (!camera->followEnabled)
                    camera->GetCamera().target = {};
            }
        } else if (page == 2) {
            if (action == 0) {
                alternate = !alternate;
                ResourceManager::ReloadTexture("lab.sheet",
                                               Asset(alternate ? "sheet-alt.png" : "sheet.png"));
            }
            if (action == 1)
                animation->playing = !animation->playing;
            if (action == 2)
                sprite->flipX = !sprite->flipX;
        } else if (page == 3) {
            if (action == 0)
                Burst();
            if (action == 1)
                timer->SetEnabled(!timer->IsEnabled());
            if (action == 2)
                emitter->shape =
                    static_cast<EmitterShape>((static_cast<int>(emitter->shape) + 1) % 4);
        } else if (page == 4) {
            int x = static_cast<int>(selectedTile.x), y = static_cast<int>(selectedTile.y);
            if (action == 0)
                map->SetTile(x, y, (map->GetTile(x, y) + 1) % 4);
            if (action == 1)
                map->SetTile(x, y, -1);
            if (action == 2) {
                if (map->solidTiles.empty())
                    map->solidTiles.insert(1);
                else
                    map->solidTiles.clear();
            }
        }
    }
};
} // namespace Lab
