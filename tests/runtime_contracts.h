#pragma once

namespace runtime_tests {
struct InputProbe : Component {
    int presses = 0, releases = 0, held = 0, updatesPressed = 0, ticks = 0;
    float wheel = 0, dx = 0;
    bool fail = false;
    void Update() override { updatesPressed += Input::GetActionDown("test-fire"); }
    void FixedUpdate() override {
        ++ticks;
        presses += Input::GetActionDown("test-fire");
        releases += Input::GetKeyUp(KEY_SPACE);
        held += Input::GetKey(KEY_SPACE);
        wheel += Input::GetMouseWheelMove(); dx += Input::GetMouseDelta().x;
        if (fail) throw std::runtime_error("input tick failure");
    }
};
static void Inputs() {
    Input::Reset();
    Input::BindAction("test-fire", {KEY_SPACE});
    LogicEngine engine;
    engine.SetFixedTimeStep(0.125f);
    auto scene = CreateRef<Scene>();
    auto probe = scene->CreateGameObject()->AddComponent<InputProbe>();
    engine.LoadScene(scene); engine.Tick(0);
    InputFrame frame;
    frame.keys.held[KEY_SPACE] = true;
    frame.mouseWheel = 2; frame.mouseDelta = {3, 0};
    Input::SetFrame(frame); engine.Tick(0.03125f);
    CHECK(probe->ticks == 0 && probe->updatesPressed == 1);
    frame = {}; Input::SetFrame(frame); engine.Tick(0.03125f);
    CHECK(!Input::GetKeyDown(KEY_SPACE) && Input::GetKeyUp(KEY_SPACE));
    Input::SetFrame({}); engine.Tick(0.0625f);
    CHECK(probe->presses == 1 && probe->releases == 1 && probe->held == 0);
    CHECK(Approx(probe->wheel, 2) && Approx(probe->dx, 3));
    frame.keys.held[KEY_SPACE] = true;
    Input::SetFrame(frame); engine.Tick(0.375f);
    CHECK(probe->presses == 2 && probe->held == 3 && probe->ticks == 4);
    CHECK(Input::GetKeyDown(KEY_SPACE)); // Frame view survives multiple fixed ticks.
    Input::SetFrame(frame); engine.Tick(0.125f);
    CHECK(probe->presses == 2 && probe->held == 4);
    engine.SetPaused(true);
    Input::SetFrame({}); engine.Tick(0.125f);
    Input::SetFrame(frame); engine.Tick(0.125f);
    engine.SetPaused(false); engine.Tick(0.125f);
    CHECK(probe->presses == 2 && probe->releases == 1 && probe->held == 5);
    Input::SetFrame({}); engine.Tick(0);
    Input::SetFrame(frame); engine.Tick(0);
    frame.focused = false; Input::SetFrame(frame); engine.Tick(0.125f);
    CHECK(!Input::GetKey(KEY_SPACE) && probe->presses == 2);
    frame.focused = true; Input::SetFrame(frame); engine.Tick(0.125f);
    CHECK(probe->presses == 2 && Input::GetKey(KEY_SPACE));
    auto next = CreateRef<Scene>();
    auto nextProbe = next->CreateGameObject()->AddComponent<InputProbe>();
    Input::SetFrame({}); Input::SetFrame(frame);
    engine.LoadScene(next); engine.Tick(0.125f);
    CHECK(nextProbe->presses == 0 && nextProbe->held == 1);
    Input::SetFrame({}); Input::SetFrame(frame);
    nextProbe->fail = true;
    bool caught = false;
    try { engine.Tick(0.125f); } catch (const std::runtime_error&) { caught = true; }
    CHECK(caught && Input::GetKeyDown(KEY_SPACE));
    nextProbe->fail = false;
    Input::SetFrame(frame); engine.Tick(0);
    CHECK(nextProbe->presses == 1); // Consumed input is not replayed after an exception.
    CHECK(!Input::GetKey(-1) && !Input::GetKeyDown(512) && !Input::GetMouseButton(8));
    CHECK(!Input::GamepadAvailable(-1) && !Input::GetGamepadButton(32, 4));
    CHECK(Approx(Input::GetGamepadAxis(-1), 0));
    engine.Shutdown();
    // Pausing from a scene callback suppresses later simulation phases this frame.
    LogicEngine pausing;
    auto pauseScene = CreateRef<Scene>();
    auto pauseProbe = pauseScene->CreateGameObject()->AddComponent<LifecycleProbe>();
    pausing.LoadScene(pauseScene); pausing.Tick(0);
    pauseProbe->onUpdate = [&] { pausing.SetPaused(true); };
    pausing.Tick(0.125f);
    CHECK(pausing.IsPaused() && pauseProbe->fixed == 0);
    pauseProbe->onUpdate = nullptr;
    pausing.SetPaused(false); pausing.Tick(0);
    CHECK(pauseProbe->fixed == 0);
    pausing.Shutdown();

    frame = {};
    frame.gamepads[0].available = true;
    frame.gamepads[0].buttons.held[7] = true;
    frame.gamepads[0].axes[0] = 0.1f;
    Input::SetFrame(frame);
    CHECK(Input::GetGamepadButtonDown(7) && Approx(Input::GetGamepadAxis(0), 0));
    frame.gamepads[0].axes[0] = 0.8f;
    Input::SetFrame(frame);
    { Input::FixedTickScope tick; CHECK(Input::GetGamepadButtonDown(7) && Approx(Input::GetGamepadAxis(0), 0.8f)); }
    { Input::FixedTickScope tick; CHECK(!Input::GetGamepadButtonDown(7) && Input::GetGamepadButton(7)); }
    Input::SetFrame({});
    { Input::FixedTickScope tick; CHECK(!Input::GamepadAvailable() && Input::GetGamepadButtonUp(7)); }
    for (int fps : {30, 60, 144}) {
        Input::Reset();
        LogicEngine timed;
        auto stage = CreateRef<Scene>();
        auto counter = stage->CreateGameObject()->AddComponent<InputProbe>();
        timed.LoadScene(stage); timed.Tick(0);
        for (int i = 1; i <= fps * 2; ++i) {
            InputFrame sample;
            sample.keys.pressed[KEY_SPACE] = i * 10 / fps != (i - 1) * 10 / fps;
            Input::SetFrame(sample); timed.Tick(1.0f / fps);
        }
        Input::SetFrame({}); timed.Tick(1.0f / 60);
        CHECK(counter->presses == 20);
        timed.Shutdown();
    }
    Input::Reset();
}

struct ScopedScene : Scene {
    int calls = 0;
    void OnLoad() override { EventBus::Subscribe("scoped", [this](const EventData&) { ++calls; }); }
};
struct ScopedComponent : Component {
    int calls = 0;
    void Awake() override {
        EventBus::Subscribe("component", [this](const EventData&) { ++calls; });
        Tween::Delay(1, [this] { ++calls; });
    }
};
static void Scopes() {
    EventBus::Clear(); TweenManager::Clear();
    int calls = 0;
    {
        LogicEngine uninitialized;
        EventBus::Subscribe("before-init", [&](const EventData&) { ++calls; }, uninitialized.GetCallbackScope());
        uninitialized.Shutdown();
        EventBus::Emit("before-init");
        CHECK(calls == 0 && !uninitialized.GetCallbackScope().IsAlive());
    }
    auto capture = std::make_shared<int>(7);
    std::weak_ptr<int> released = capture;
    CallbackScope parent;
    CallbackScope child(parent.Token());
    EventBus::Subscribe("capture", [capture](const EventData&) {}, child.Token());
    Tween::Delay(10, [capture] {}, child.Token());
    capture.reset();
    CHECK(!released.expired());
    parent.Cancel();
    CHECK(released.expired() && !child.Token().IsAlive() && TweenManager::Count() == 0);
    parent.Renew();
    CHECK(parent.Token().IsAlive() && !child.Token().IsAlive());
    CallbackScope owner;
    EventBus::Subscribe("self", [&](const EventData&) {
        owner.Cancel();
        EventBus::Subscribe("nested", [&](const EventData&) { ++calls; });
        ++calls;
    }, owner.Token());
    EventBus::Subscribe("self", [&](const EventData&) { calls += 100; }, owner.Token());
    EventBus::Emit("self"); EventBus::Emit("nested"); EventBus::Emit("self");
    CHECK(calls == 1);
    EventBus::Subscribe("mutable", [value = 0, &calls](const EventData&) mutable { calls = ++value; });
    EventBus::Emit("mutable"); EventBus::Emit("mutable");
    CHECK(calls == 2);
    owner.Renew();
    Tween::To(0, 1, 0, Ease::Linear, [&](float) { owner.Cancel(); ++calls; },
        [&] { calls += 100; }, owner.Token());
    TweenManager::Update(1);
    CHECK(calls == 3 && TweenManager::Count() == 0);
    Tween::To(0, 1, 1, Ease::Linear, [value = 0, &calls](float) mutable { calls = ++value; });
    TweenManager::Update(0.1f); TweenManager::Update(0.1f);
    CHECK(calls == 2);
    TweenManager::Clear(); EventBus::Clear();

    LogicEngine engine;
    int appCalls = 0, globalCalls = 0, appTween = 0;
    EventBus::Subscribe("scoped", [&](const EventData&) { ++appCalls; }, engine.GetCallbackScope());
    EventBus::Subscribe("scoped", [&](const EventData&) { ++globalCalls; }, {});
    Tween::Delay(0.5f, [&] { ++appTween; }, engine.GetCallbackScope());
    auto first = CreateRef<ScopedScene>();
    auto second = CreateRef<ScopedScene>();
    engine.LoadScene(first); engine.Tick(0);
    auto object = first->CreateGameObject();
    auto component = object->AddComponent<ScopedComponent>();
    EventBus::Emit("component");
    CHECK(component->calls == 1);
    first->Destroy(object);
    EventBus::Emit("component"); TweenManager::Update(0.1f);
    CHECK(component->calls == 1 && !component->GetCallbackScope().IsAlive());
    EventBus::Emit("scoped");
    CHECK(first->calls == 1 && appCalls == 1 && globalCalls == 1);
    engine.LoadScene(second); engine.Tick(0);
    EventBus::Emit("scoped"); TweenManager::Update(1);
    CHECK(first->calls == 1 && second->calls == 1 && appCalls == 2 && globalCalls == 2);
    CHECK(appTween == 1 && component->calls == 1);
    for (int i = 0; i < 1000; ++i) {
        auto previous = second->GetCallbackScope();
        second->Clear(); second->Load();
        if (previous.IsAlive()) { CHECK(false); break; }
    }
    EventBus::Emit("scoped");
    CHECK(second->calls == 2 && appCalls == 3 && globalCalls == 3);
    engine.Shutdown(); EventBus::Emit("scoped");
    CHECK(appCalls == 3 && globalCalls == 3 && TweenManager::Count() == 0);

    // Dispatching a nested event/tween inherits its original owner.
    CallbackScope nested;
    EventBus::Subscribe("outer", [&](const EventData&) {
        Tween::Delay(1, [&] { ++calls; });
        EventBus::Subscribe("inner", [&](const EventData&) { ++calls; });
    }, nested.Token());
    EventBus::Emit("outer");
    nested.Cancel(); calls = 0;
    EventBus::Emit("inner"); TweenManager::Update(2);
    CHECK(calls == 0);
    EventBus::Clear(); TweenManager::Clear();

    Scene phaseScene;
    auto phased = phaseScene.CreateGameObject();
    auto phaseProbe = phased->AddComponent<LifecycleProbe>();
    auto registerEvent = [&] {
        EventBus::Subscribe("phase", [&](const EventData&) { ++calls; });
    };
    phaseProbe->onStart = registerEvent;
    phaseProbe->onUpdate = registerEvent;
    phaseProbe->onRender = registerEvent;
    phaseProbe->onHit = registerEvent;
    phased->AddComponent<CircleCollider>()->isTrigger = true;
    phaseScene.CreateGameObject()->AddComponent<CircleCollider>();
    phaseScene.Update(); phaseScene.Render(); PhysicsWorld::Step();
    calls = 0; EventBus::Emit("phase");
    CHECK(calls == 4);
    phaseScene.Destroy(phased); EventBus::Emit("phase");
    CHECK(calls == 4);
    phaseScene.Clear(); PhysicsWorld::Clear();

    // Cancellation releases captured owners even when their destructors cancel services.
    struct ClearOnDestroy {
        ~ClearOnDestroy() { EventBus::Clear(); TweenManager::Clear(); }
    };
    CallbackScope releasing;
    auto cleanup = std::make_shared<ClearOnDestroy>();
    std::weak_ptr<ClearOnDestroy> cleanupWeak = cleanup;
    EventBus::Subscribe("release", [cleanup](const EventData&) {}, releasing.Token());
    cleanup.reset();
    releasing.Cancel();
    CHECK(cleanupWeak.expired());
    releasing.Renew();
    cleanup = std::make_shared<ClearOnDestroy>(); cleanupWeak = cleanup;
    Tween::Delay(10, [cleanup] {}, releasing.Token());
    cleanup.reset(); releasing.Cancel();
    CHECK(cleanupWeak.expired() && TweenManager::Count() == 0);

    CallbackScope throwing;
    EventBus::Subscribe("throw", [](const EventData&) { throw std::runtime_error("event failure"); }, throwing.Token());
    try { EventBus::Emit("throw"); } catch (const std::runtime_error&) {}
    throwing.Cancel();
    calls = 0;
    EventBus::Subscribe("after-throw", [&](const EventData&) { ++calls; });
    EventBus::Emit("after-throw");
    CHECK(calls == 1); // RAII restores the default owner after a throwing event.
    EventBus::Clear(); TweenManager::Clear();
}
}
static void TestRuntimeContracts() {
    printf("[input snapshots and callback scopes]\n");
    runtime_tests::Inputs();
    runtime_tests::Scopes();
}
