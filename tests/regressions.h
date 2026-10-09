#pragma once

struct LifecycleProbe : Component {
    int configured = 0, seen = -1, starts = 0, updates = 0, fixed = 0, destroys = 0;
    bool foundInAwake = false;
    std::function<void()> onStart, onUpdate, onRender, onHit;
    std::vector<int>* drawOrder = nullptr;
    int drawId = 0, layer = 0;
    void Awake() override { foundInAwake = gameObject->GetComponent<LifecycleProbe>().get() == this; }
    void Start() override { ++starts; seen = configured; if (onStart) onStart(); }
    void Update() override { ++updates; if (onUpdate) onUpdate(); }
    void FixedUpdate() override { ++fixed; }
    void OnDestroy() override { ++destroys; }
    int RenderLayer() const override { return layer; }
    void Render() override { if (drawOrder) drawOrder->push_back(drawId); if (onRender) onRender(); }
    void OnTriggerEnter(Collider*) override { if (onHit) onHit(); }
};

static void TestLifecycle() {
    printf("[lifecycle and rendering]\n");
    PhysicsWorld::Clear();
    Scene scene;
    auto object = scene.CreateGameObject();
    auto probe = object->AddComponent<LifecycleProbe>();
    CHECK(probe->foundInAwake);
    probe->configured = 42;
    scene.Start();
    scene.Start();
    CHECK(probe->starts == 1 && probe->seen == 42);
    auto late = object->AddComponent<LifecycleProbe>();
    late->configured = 17;
    CHECK(late->starts == 0);
    scene.Update();
    CHECK(late->starts == 1 && late->seen == 17 && late->updates == 1);
    late->SetEnabled(false);
    scene.Update();
    CHECK(late->updates == 1);
    late->SetEnabled(true);
    scene.Update();
    CHECK(late->starts == 1 && late->updates == 2);
    auto disabled = object->AddComponent<LifecycleProbe>();
    disabled->SetEnabled(false);
    scene.Update();
    CHECK(disabled->starts == 0);
    disabled->SetEnabled(true);
    scene.FixedUpdate();
    CHECK(disabled->starts == 1 && disabled->fixed == 1);
    object->active = false;
    scene.Update();
    CHECK(scene.GetAllGameObjects().size() == 1);
    object->active = true;

    Ref<LifecycleProbe> spawned;
    probe->onUpdate = [&] {
        if (spawned) return;
        auto child = scene.CreateGameObject("spawned");
        spawned = child->AddComponent<LifecycleProbe>();
        spawned->configured = 99;
    };
    scene.Update();
    CHECK(spawned && spawned->starts == 1 && spawned->seen == 99 && spawned->updates == 0);
    scene.Update();
    CHECK(spawned->updates == 1);
    object->AddComponent<CircleCollider>();
    scene.Update();
    CHECK(PhysicsWorld::GetColliderCount() == 1);
    scene.Destroy(object);
    scene.Update();
    CHECK(probe->destroys == 1 && probe->GetGameObject() == nullptr);
    CHECK(object->scene == nullptr && PhysicsWorld::GetColliderCount() == 0);
    object.reset();
    CHECK(probe->destroys == 1);
    scene.Clear();
    CHECK(scene.GetAllGameObjects().empty() && spawned->destroys == 1);

    auto parent = scene.CreateGameObject("parent");
    auto spawner = parent->AddComponent<LifecycleProbe>();
    Ref<LifecycleProbe> fromStart;
    spawner->onStart = [&] {
        auto child = scene.CreateGameObject();
        fromStart = child->AddComponent<LifecycleProbe>();
        fromStart->configured = 123;
    };
    scene.Start();
    CHECK(fromStart && fromStart->starts == 1 && fromStart->seen == 123);
    scene.Clear();

    std::vector<int> drawOrder;
    for (int i = 0; i < 4; ++i) {
        auto p = scene.CreateGameObject()->AddComponent<LifecycleProbe>();
        p->drawOrder = &drawOrder;
        p->drawId = i;
        p->layer = i == 0 ? 2 : 1;
        p->renderOrder = i == 2 ? -1 : 0;
    }
    scene.Render();
    CHECK((drawOrder == std::vector<int>{2, 1, 3, 0}));
    scene.Clear();
    auto killer = scene.CreateGameObject()->AddComponent<LifecycleProbe>();
    auto victim = scene.CreateGameObject();
    auto victimRenderer = victim->AddComponent<LifecycleProbe>();
    victimRenderer->drawOrder = &drawOrder;
    drawOrder.clear();
    killer->onRender = [&] { scene.Destroy(victim); };
    scene.Render();
    CHECK(drawOrder.empty() && victimRenderer->destroys == 1);
    scene.Clear();
}

static void TestTransformAndTilemap() {
    printf("[hierarchy and tilemap]\n");
    auto parent = CreateRef<GameObject>();
    auto child = CreateRef<GameObject>();
    auto grandchild = CreateRef<GameObject>();
    parent->transform->position = {50, 100};
    parent->transform->rotation = 90;
    parent->transform->scale = {2, 3};
    child->transform->SetParent(parent->transform.get(), false);
    child->transform->position = {5, 10};
    child->transform->rotation = 30;
    grandchild->transform->SetParent(child->transform.get(), false);
    parent->transform->SetParent(grandchild->transform.get());
    CHECK(parent->transform->GetParent() == nullptr);
    Vector2 local{7, 9};
    Vector2 roundtrip = grandchild->transform->InverseTransformPoint(grandchild->transform->TransformPoint(local));
    CHECK(Approx(roundtrip.x, local.x) && Approx(roundtrip.y, local.y));
    child->transform->SetWorldPosition({123, 456});
    Vector2 pos = child->transform->GetWorldPosition();
    CHECK(Approx(pos.x, 123) && Approx(pos.y, 456));
    parent.reset();
    CHECK(child->transform->GetParent() == nullptr);
    CHECK(Approx(child->transform->GetWorldPosition().x, pos.x));
    CHECK(Approx(child->transform->GetWorldPosition().y, pos.y));

    auto map = child->AddComponent<Tilemap>();
    map->Resize(3, 2);
    map->SetTile(1, 1, 7);
    map->solidTiles.insert(7);
    CHECK(map->IsSolid(1, 1) && !map->IsSolid(-1, 1));
    Vector2 point = child->transform->TransformPoint({48, 48});
    CHECK(map->IsSolidAtWorld(point));
    Vector2 corner = map->TileToWorld(1, 1);
    Vector2 expected = child->transform->TransformPoint({32, 32});
    CHECK(Approx(corner.x, expected.x) && Approx(corner.y, expected.y));
    map->tileWidth = 0;
    CHECK(!map->IsSolidAtWorld(point));
    map->Resize(-5, 2);
    CHECK(map->GetTiles().empty());
    auto cameraObject = CreateRef<GameObject>();
    auto camera = cameraObject->AddComponent<Camera2DComponent>();
    camera->SetTarget(child.get());
    GameTime::Update(1);
    camera->Update();
    CHECK(camera->GetCamera().target.x > 0);
    child.reset();
    camera->Update();
    CHECK(!camera->followEnabled && camera->target == nullptr);
}

static void TestTweenMutationAndTiming() {
    printf("[tween timing and callback mutation]\n");
    TweenManager::Clear();
    int completed = 0;
    Tween::Delay(0.5f, [&] { ++completed; });
    TweenManager::Update(0.5f);
    CHECK(completed == 1 && TweenManager::Count() == 0);
    float value = 0;
    TweenInstance delayed;
    delayed.delay = 0.25f;
    delayed.duration = 1;
    delayed.onUpdate = [&](float v) { value = v; };
    TweenManager::Add(delayed);
    TweenManager::Update(0.75f);
    CHECK(Approx(value, 0.5f));
    TweenManager::Clear();
    delayed.delay = 0;
    delayed.pingpong = true;
    delayed.onComplete = [&] { ++completed; };
    TweenManager::Add(delayed);
    TweenManager::Update(1.25f);
    CHECK(Approx(value, 0.75f));
    TweenManager::Update(0.75f);
    CHECK(Approx(value, 0) && completed == 2);
    delayed.loop = true;
    TweenManager::Add(delayed);
    TweenManager::Update(100.25f);
    CHECK(Approx(value, 0.25f) && TweenManager::Count() == 1);
    TweenManager::Clear();
    int callbacks = 0;
    Tween::Delay(0, [&] {
        for (int i = 0; i < 128; ++i) Tween::Delay(0, [&] { ++callbacks; });
    });
    TweenManager::Update(1);
    CHECK(callbacks == 0 && TweenManager::Count() == 128);
    TweenManager::Update(0);
    CHECK(callbacks == 128 && TweenManager::Count() == 0);
    Tween::To(0, 1, 1, Ease::Linear, [&](float) { TweenManager::Clear(); }, [&] { ++callbacks; });
    Tween::Delay(0, [&] { ++callbacks; });
    TweenManager::Update(1);
    CHECK(callbacks == 128 && TweenManager::Count() == 0);
    auto id = Tween::Delay(0, [&] { ++callbacks; });
    TweenManager::Cancel(id);
    TweenManager::Update(1);
    CHECK(callbacks == 128);
}

static void TestPhysicsResolutionAndMutation() {
    printf("[physics resolution and callback mutation]\n");
    PhysicsWorld::Clear();
    for (int shape = 0; shape < 3; ++shape) {
        Scene scene;
        auto a = scene.CreateGameObject();
        auto b = scene.CreateGameObject();
        if (shape == 0) { a->AddComponent<CircleCollider>(10.0f); b->AddComponent<CircleCollider>(10.0f); }
        if (shape == 1) { a->AddComponent<BoxCollider>(Vector2{20,20}); b->AddComponent<BoxCollider>(Vector2{20,20}); }
        if (shape == 2) { a->AddComponent<CircleCollider>(10.0f); b->AddComponent<BoxCollider>(Vector2{20,80}); }
        b->transform->position = {15, 0};
        auto ra = a->AddComponent<Rigidbody>();
        auto rb = b->AddComponent<Rigidbody>();
        ra->restitution = rb->restitution = 1;
        ra->velocity = {10, 0}; rb->velocity = {-10, 0};
        scene.Start();
        PhysicsWorld::Step();
        CHECK(Approx(ra->velocity.x, -10) && Approx(rb->velocity.x, 10));
        CHECK(b->transform->position.x - a->transform->position.x > 19.99f);
        PhysicsWorld::Step();
        CHECK(Approx(ra->velocity.x, -10) && Approx(rb->velocity.x, 10));
        scene.Clear(); PhysicsWorld::Clear();
    }
    {
        Scene scene;
        auto a = scene.CreateGameObject();
        auto b = scene.CreateGameObject();
        a->transform->position = {14, 30};
        a->AddComponent<CircleCollider>(5.0f);
        b->AddComponent<BoxCollider>(Vector2{20,100});
        auto body = a->AddComponent<Rigidbody>();
        body->velocity = {-20, 7}; body->restitution = 1;
        scene.Start();
        PhysicsWorld::Step();
        CHECK(Approx(body->velocity.x, 20) && Approx(body->velocity.y, 7));
        CHECK(a->transform->position.x > 14.99f);
        a->transform->position = {9, 20}; body->velocity = {0, 0};
        PhysicsWorld::Step();
        CHECK(a->transform->position.x > 14.99f);
    }
    PhysicsWorld::Clear();
    {
        Scene scene;
        auto a = scene.CreateGameObject();
        auto b = scene.CreateGameObject();
        auto ca = a->AddComponent<CircleCollider>(10.0f);
        auto cb = b->AddComponent<CircleCollider>(10.0f);
        auto hits = a->AddComponent<HitCounter>();
        scene.Start();
        b->active = false;
        PhysicsWorld::Step(); CHECK(hits->enters == 0);
        b->active = true; cb->layer = 32;
        PhysicsWorld::Step(); CHECK(hits->enters == 0);
        cb->layer = 1; ca->mask = 1;
        PhysicsWorld::Step(); CHECK(hits->enters == 0);
        ca->mask = 2;
        PhysicsWorld::Step(); CHECK(hits->enters == 1);
        cb->SetEnabled(false);
        PhysicsWorld::Step(); CHECK(hits->exits == 1);
        cb->SetEnabled(true);
        PhysicsWorld::Step(); CHECK(hits->enters == 2);
        scene.Destroy(b); scene.Update();
        PhysicsWorld::Step(); CHECK(hits->exits == 2);
    }
    PhysicsWorld::Clear();
    {
        Scene scene;
        auto a = scene.CreateGameObject();
        auto b = scene.CreateGameObject();
        auto ca = a->AddComponent<CircleCollider>(10.0f);
        b->AddComponent<CircleCollider>(10.0f)->isTrigger = true;
        auto mutation = a->AddComponent<LifecycleProbe>();
        int calls = 0;
        mutation->onHit = [&] {
            ++calls;
            for (int i = 0; i < 64; ++i) {
                a->AddComponent<LifecycleProbe>();
                auto child = scene.CreateGameObject();
                child->transform->position = {1000.0f + i * 50, 1000};
                child->AddComponent<CircleCollider>(1.0f);
                child->Start();
            }
            ca->SetEnabled(false);
        };
        scene.Start();
        PhysicsWorld::Step();
        CHECK(calls == 1 && PhysicsWorld::GetColliderCount() == 65);
        PhysicsWorld::Step();
        CHECK(calls == 1);
        scene.Clear();
    }
    PhysicsWorld::Clear();
    {
        Scene scene;
        auto a = scene.CreateGameObject();
        auto b = scene.CreateGameObject();
        a->AddComponent<CircleCollider>();
        b->AddComponent<CircleCollider>()->isTrigger = true;
        auto clear = a->AddComponent<LifecycleProbe>();
        clear->onHit = [&] { PhysicsWorld::Clear(); scene.Clear(); };
        scene.Start();
        PhysicsWorld::Step();
        CHECK(PhysicsWorld::GetColliderCount() == 0 && scene.GetAllGameObjects().empty());
    }
    PhysicsWorld::Clear();
}

class LogicEngine : public GameEngine {
public:
    LogicEngine() : GameEngine("logic", 1, 1) {}
    using GameEngine::Tick;
};

class CountingScene : public Scene {
public:
    int loads = 0, unloads = 0;
    Ref<GameObject> retained;
    Ref<LifecycleProbe> probe;
    void OnLoad() override {
        ++loads;
        retained = CreateGameObject();
        retained->AddComponent<CircleCollider>();
        auto body = retained->AddComponent<Rigidbody>();
        body->drag = 1;
        body->velocity = {8, 0};
        probe = retained->AddComponent<LifecycleProbe>();
    }
    void OnUnload() override { ++unloads; }
};

static void TestFixedLoopAndSceneSwitch() {
    printf("[fixed loop and scene switching]\n");
    LogicEngine engine;
    auto first = CreateRef<CountingScene>();
    auto second = CreateRef<CountingScene>();
    engine.SetFixedTimeStep(0.125f);
    engine.LoadScene(first);
    CHECK(first->loads == 0);
    engine.Tick(0.25f);
    CHECK(first->loads == 1 && first->probe->starts == 1);
    CHECK(first->probe->updates == 1 && first->probe->fixed == 2);
    CHECK(Approx(first->retained->transform->position.x, 2));
    CHECK(Approx(GameTime::FixedDeltaTime(), 0.125f));
    engine.SetFixedTimeStep(0);
    engine.SetFixedTimeStep(-1);
    engine.Tick(0.125f);
    CHECK(first->probe->fixed == 3);
    engine.Tick(10);
    CHECK(first->probe->fixed == 11);
    engine.Tick(0);
    CHECK(first->probe->fixed == 11);
    int staleCalls = 0;
    Tween::Delay(1, [&] { ++staleCalls; });
    EventBus::Subscribe("stale", [&](const EventData&) { ++staleCalls; });
    first->probe->onUpdate = [&] { engine.LoadScene(second); };
    engine.Tick(0);
    CHECK(engine.GetCurrentScene() == first && first->unloads == 0);
    engine.Tick(0);
    EventBus::Emit("stale");
    TweenManager::Update(2);
    CHECK(engine.GetCurrentScene() == second && second->loads == 1);
    CHECK(first->unloads == 1 && first->probe->destroys == 1);
    CHECK(first->retained->IsDestroyed() && first->retained->scene == nullptr);
    CHECK(staleCalls == 0 && PhysicsWorld::GetColliderCount() == 1);
    engine.Shutdown();
    engine.Shutdown();
    CHECK(second->unloads == 1 && second->probe->destroys == 1);
    CHECK(PhysicsWorld::GetColliderCount() == 0);
}

static void TestUtilities() {
    printf("[timer, pool, event and save edge cases]\n");
    auto object = CreateRef<GameObject>();
    int calls = 0;
    auto timer = object->AddComponent<TimerComponent>(0.25f, [&] { ++calls; }, true);
    GameTime::Update(0.875f);
    object->Update();
    CHECK(calls == 3);
    GameTime::Update(0.125f);
    object->Update();
    CHECK(calls == 4);
    timer->interval = 0;
    object->Update();
    CHECK(calls == 5);
    object->AddComponent<DestroyAfter>(0.1f);
    object->Update();
    CHECK(object->IsDestroyed());
    ObjectPool<int> pool(1);
    auto p = pool.Acquire();
    pool.Release(p); pool.Release(p);
    int foreign = 0;
    pool.Release(&foreign);
    CHECK(pool.Available() == 1 && pool.InUse() == 0);
    auto x = pool.Acquire(), y = pool.Acquire();
    CHECK(x != y && pool.InUse() == 2);
    pool.Reset();
    CHECK(pool.InUse() == 0 && pool.Available() == pool.Capacity());
    EventBus::Clear();
    int observed = 0;
    unsigned long second = 0;
    EventBus::Subscribe("mutate", [&](const EventData&) { EventBus::Unsubscribe("mutate", second); });
    second = EventBus::Subscribe("mutate", [&](const EventData&) { ++observed; });
    EventBus::Emit("mutate");
    CHECK(observed == 0);
    EventBus::Clear();
    EventBus::Subscribe("mutate", [&](const EventData&) { EventBus::Clear(); });
    EventBus::Subscribe("mutate", [&](const EventData&) { ++observed; });
    EventBus::Emit("mutate");
    CHECK(observed == 0);
    SaveSystem save;
    save.SetString("#a=b", "line1\nline2\r\nC:\\new\\path=value");
    save.SetString("invalid", "123abc");
    save.SetString("nan", "nan");
    CHECK(save.Save("test_save_edges.tmp"));
    SaveSystem loaded;
    CHECK(loaded.Load("test_save_edges.tmp"));
    CHECK(loaded.GetString("#a=b") == save.GetString("#a=b"));
    CHECK(loaded.GetInt("invalid", -1) == -1);
    CHECK(loaded.GetFloat("nan", -1) == -1);
    CHECK(loaded.GetBool("invalid", true));
    CHECK(!loaded.Load("raygame_nonexistent_directory/save.txt"));
    CHECK(loaded.Has("#a=b"));
    remove("test_save_edges.tmp");
}

static void TestRegressions() {
    TestLifecycle();
    TestTransformAndTilemap();
    TestTweenMutationAndTiming();
    TestPhysicsResolutionAndMutation();
    TestFixedLoopAndSceneSwitch();
    TestUtilities();
}
