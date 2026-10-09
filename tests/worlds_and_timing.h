#pragma once

namespace world_tests {

struct ContactProbe : Component {
    int enters = 0, stays = 0, exits = 0;
    std::function<void()> onEnter;
    void OnTriggerEnter(Collider*) override { ++enters; if (onEnter) onEnter(); }
    void OnTriggerStay(Collider*) override { ++stays; }
    void OnTriggerExit(Collider*) override { ++exits; }
};

static Ref<ContactProbe> Pair(Scene& scene) {
    auto a = scene.CreateGameObject();
    a->AddComponent<CircleCollider>()->isTrigger = true;
    auto probe = a->AddComponent<ContactProbe>();
    scene.CreateGameObject()->AddComponent<CircleCollider>();
    scene.Start();
    return probe;
}

static void Isolation() {
    Scene first, second;
    auto a = Pair(first), b = Pair(second);
    a->onEnter = [&] { second.GetPhysicsWorld().Step(); };
    first.GetPhysicsWorld().Step();
    CHECK(a->enters == 1 && b->enters == 1); // Nested stepping of distinct worlds is independent.
    CHECK(first.GetPhysicsWorld().GetContactCount() == 1 && second.GetPhysicsWorld().GetContactCount() == 1);
    first.Clear();
    second.GetPhysicsWorld().Step();
    CHECK(b->stays == 1 && b->exits == 0 && second.GetPhysicsWorld().GetColliderCount() == 2);
    CHECK(first.GetPhysicsWorld().GetContactCount() == 0);
    bool rejected = false;
    try { first.GetPhysicsWorld().AddCollider(second.GetAllGameObjects()[0]->GetComponent<Collider>().get()); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);

    // Clear from a collision callback aborts only that world's dispatch.
    a = Pair(first);
    a->onEnter = [&] { first.Clear(); };
    first.GetPhysicsWorld().Step();
    CHECK(first.GetAllGameObjects().empty() && first.GetPhysicsWorld().GetContactCount() == 0);
    second.GetPhysicsWorld().Step();
    CHECK(b->stays == 2);

    // Standalone worlds have explicit registration and safe binding lifetimes.
    auto object = CreateRef<GameObject>();
    auto collider = object->AddComponent<CircleCollider>();
    object->Start();
    CHECK(collider->GetPhysicsWorld() == nullptr);
    {
        PhysicsWorld world;
        world.AddCollider(collider.get());
        collider->SetEnabled(false);
        CHECK(world.GetColliderCount() == 0 && collider->GetPhysicsWorld() == &world);
        collider->SetEnabled(true);
        CHECK(world.GetColliderCount() == 1);
    }
    CHECK(collider->GetPhysicsWorld() == nullptr && collider->GetGameObject() == object.get());
    object.reset();
    CHECK(collider->GetGameObject() == nullptr);

    // Engine teardown and scene replacement must leave an unrelated scene alive.
    {
        LogicEngine engine;
        auto owned = CreateRef<Scene>();
        Pair(*owned);
        engine.LoadScene(owned); engine.Tick(0.02f);
        engine.LoadScene(CreateRef<Scene>()); engine.Tick(0);
        CHECK(owned->GetPhysicsWorld().GetColliderCount() == 0);
        engine.Shutdown();
    }
    second.GetPhysicsWorld().Step();
    CHECK(b->stays == 3 && b->GetGameObject());
    for (int i = 0; i < 1000; ++i) {
        auto probe = Pair(first);
        first.GetPhysicsWorld().Step();
        if (probe->enters != 1) { CHECK(false); break; }
        first.Clear();
    }
    CHECK(first.GetPhysicsWorld().GetColliderCount() == 0 && first.GetPhysicsWorld().GetContactCount() == 0);
    CHECK(second.GetPhysicsWorld().GetColliderCount() == 2 && second.GetPhysicsWorld().GetContactCount() == 1);
}

struct Driver : Component {
    std::vector<int>* order = nullptr;
    float before = -1, after = -1, render = -1, dt = 0;
    bool lateWasFixed = true;
    void Update() override { if (order) order->push_back(1); }
    void FixedUpdate() override {
        before = gameObject->transform->position.x;
        dt = GameTime::DeltaTime();
        gameObject->GetComponent<Rigidbody>()->velocity = {8, 0};
        if (order) order->push_back(2);
    }
    void OnTriggerEnter(Collider*) override { if (order) order->push_back(3); }
    void PostFixedUpdate() override {
        after = gameObject->transform->position.x;
        if (order) order->push_back(4);
    }
    void LateUpdate() override {
        render = gameObject->transform->GetRenderPosition().x;
        lateWasFixed = GameTime::IsFixedStep();
        if (order) order->push_back(5);
    }
};

static void PhasesAndInterpolation() {
    LogicEngine engine;
    engine.SetFixedTimeStep(0.125f);
    auto scene = CreateRef<Scene>();
    auto object = scene->CreateGameObject();
    // Body deliberately precedes the driver: integration must not depend on component order.
    auto body = object->AddComponent<Rigidbody>(); body->drag = 1;
    auto driver = object->AddComponent<Driver>();
    object->AddComponent<CircleCollider>(0.1f)->isTrigger = true;
    auto wall = scene->CreateGameObject(); wall->transform->position = {1, 0};
    wall->AddComponent<CircleCollider>(0.1f);
    engine.LoadScene(scene); engine.Tick(0);
    std::vector<int> order; driver->order = &order;
    engine.Tick(0.1875f);
    CHECK(order == std::vector<int>({1, 2, 3, 4, 5}));
    CHECK(Approx(driver->before, 0) && Approx(driver->after, 1) && Approx(driver->dt, 0.125f));
    CHECK(Approx(driver->render, 0.5f) && !driver->lateWasFixed);
    CHECK(Approx(GameTime::DeltaTime(), 0.1875f) && !GameTime::IsFixedStep());
    CHECK(engine.GetFrameTiming().fixedSteps == 1 && Approx(engine.GetFrameTiming().interpolationAlpha, 0.5f));
    auto position = object->transform->position;
    for (int i = 0; i < 10; ++i) (void)object->transform->GetRenderPosition();
    CHECK(Approx(position.x, object->transform->position.x));
    engine.Tick(0.03125f);
    CHECK(Approx(driver->render, 0.75f) && engine.GetFrameTiming().fixedSteps == 0);
    engine.SetPaused(true);
    CHECK(Approx(object->transform->GetRenderPosition().x, 1));
    engine.Tick(10);
    CHECK(engine.GetFrameTiming().fixedSteps == 0 && engine.GetFrameTiming().droppedSeconds == 0);
    engine.SetPaused(false);
    object->transform->Teleport({100, 20});
    CHECK(Approx(object->transform->GetRenderPosition().x, 100));
    engine.Tick(2);
    CHECK(engine.GetFrameTiming().fixedSteps == 8 && Approx((float)engine.GetFrameTiming().droppedSeconds, 1));
    CHECK(Approx((float)engine.GetFrameTiming().totalDroppedSeconds, 1));
    engine.SetFixedTimeStep(0.25f);
    CHECK(Approx(object->transform->GetRenderPosition().x, object->transform->position.x));
    engine.Shutdown();

    RayGame::Transform parent, child;
    parent.SetInterpolationEnabled(true);
    child.SetParent(&parent, false); child.position = {10, 0};
    parent.rotation = 350; parent.CapturePreviousPose();
    parent.position = {20, 0}; parent.rotation = 10; parent.scale = {3, 3}; parent.SetRenderAlpha(0.5f);
    CHECK(Approx(parent.GetRenderRotation(), 360));
    CHECK(Approx(child.GetRenderPosition().x, 30) && Approx(child.GetRenderPosition().y, 0));
    CHECK(!Approx(child.GetWorldPosition().x, child.GetRenderPosition().x));
    child.SetParent(nullptr);
    CHECK(Approx(child.GetWorldPosition().x, child.GetRenderPosition().x));

    for (int fps : {30, 60, 144}) {
        LogicEngine timed;
        auto stage = CreateRef<Scene>();
        auto actor = stage->CreateGameObject();
        actor->AddComponent<Rigidbody>()->drag = 1;
        actor->AddComponent<Driver>();
        timed.LoadScene(stage); timed.Tick(0);
        int ticks = 0;
        for (int i = 0; i < fps * 2; ++i) {
            timed.Tick(1.0f / fps);
            ticks += timed.GetFrameTiming().fixedSteps;
        }
        CHECK(ticks == 120 && Approx(actor->transform->position.x, 16));
        CHECK(timed.GetFrameTiming().interpolationAlpha >= 0 && timed.GetFrameTiming().interpolationAlpha < 1);
    }
}

struct Interrupt : Component {
    int failPhase = 0;
    std::function<void()> pre;
    void FixedUpdate() override {
        if (pre) pre();
        if (failPhase == 1) throw std::runtime_error("pre-physics failure");
    }
    void PostFixedUpdate() override { if (failPhase == 2) throw std::runtime_error("post-physics failure"); }
    void LateUpdate() override { if (failPhase == 3) throw std::runtime_error("presentation failure"); }
};

static void Interruptions() {
    for (int phase : {1, 2, 3}) {
        LogicEngine engine;
        engine.SetFixedTimeStep(0.125f);
        auto scene = CreateRef<Scene>();
        auto obj = scene->CreateGameObject();
        obj->AddComponent<Rigidbody>()->drag = 1;
        obj->AddComponent<Driver>();
        auto stop = obj->AddComponent<Interrupt>();
        engine.LoadScene(scene); engine.Tick(0);
        stop->failPhase = phase;
        bool caught = false;
        try { engine.Tick(0.1875f); } catch (const std::runtime_error&) { caught = true; }
        CHECK(caught && !GameTime::IsFixedStep());
        CHECK(Approx(obj->transform->GetRenderPosition().x, obj->transform->position.x));
        stop->failPhase = 0;
        float position = obj->transform->position.x;
        engine.Tick(0.125f);
        CHECK(engine.GetFrameTiming().fixedSteps == 1 && Approx(obj->transform->position.x, position + 1));
    }

    struct ReloadingScene : Scene {
        bool reload = false;
        Ref<GameObject> replacement;
        void FixedUpdate() override {
            Scene::FixedUpdate();
            if (!reload) return;
            reload = false;
            Clear(); // Outside component traversal, as required by the clearing contract.
            replacement = CreateGameObject();
            auto body = replacement->AddComponent<Rigidbody>();
            body->drag = 1; body->velocity = {8, 0};
            Load();
        }
    };
    LogicEngine engine;
    engine.SetFixedTimeStep(0.125f);
    auto scene = CreateRef<ReloadingScene>();
    auto obj = scene->CreateGameObject();
    obj->AddComponent<Rigidbody>()->drag = 1;
    obj->AddComponent<Driver>();
    auto stop = obj->AddComponent<Interrupt>();
    engine.LoadScene(scene); engine.Tick(0);
    stop->pre = [&] { engine.SetPaused(true); };
    engine.Tick(0.25f);
    CHECK(engine.IsPaused() && Approx(obj->transform->position.x, 0));
    stop->pre = nullptr;
    engine.SetPaused(false); engine.Tick(0.125f);
    CHECK(Approx(obj->transform->position.x, 1));
    scene->reload = true;
    engine.Tick(0.25f);
    CHECK(obj->IsDestroyed() && scene->replacement && Approx(scene->replacement->transform->position.x, 0));
    engine.Tick(0.125f);
    CHECK(Approx(scene->replacement->transform->position.x, 1));
}

static void Run() {
    printf("[isolated physics worlds, frame phases and interpolation]\n");
    Isolation();
    PhasesAndInterpolation();
    Interruptions();
}
}
