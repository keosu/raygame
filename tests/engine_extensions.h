#pragma once

// Depth sorting, particles, physics queries/broadphase, time scale, UI pointer capture,
// audio channels and text codepoints. None of these checks need a window.
namespace extension_tests {

struct DrawRecorder : Component {
    std::vector<int>* log = nullptr;
    int id = 0;
    bool customDepth = false;
    float depth = 0;
    float RenderDepth() const override {
        return customDepth ? depth : Component::RenderDepth();
    }
    void Render() override {
        log->push_back(id);
    }
};

static void DepthSorting() {
    Scene scene;
    std::vector<int> log;
    std::vector<Ref<DrawRecorder>> recorders;
    auto add = [&](int id, float y, bool sorted, int order) {
        auto object = scene.CreateGameObject();
        object->transform->position = {0, y};
        auto recorder = object->AddComponent<DrawRecorder>();
        recorder->log = &log;
        recorder->id = id;
        recorder->ySort = sorted;
        recorder->renderOrder = order;
        recorders.push_back(recorder);
    };
    add(1, 50, true, 0);
    add(2, 10, true, 0);
    add(3, 30, true, 0);
    add(4, 0, false, 1);
    add(5, 99, true, -1);
    scene.Render();
    CHECK((log == std::vector<int>{5, 2, 3, 1, 4})); // order buckets first, then lower y first
    for (auto& recorder : recorders)
        recorder->ySort = false;
    log.clear();
    scene.Render();
    CHECK((log == std::vector<int>{5, 1, 2, 3, 4})); // without ySort registration order is kept
    recorders[0]->GetGameObject()->transform->position.y = NAN;
    recorders[0]->ySort = true;
    log.clear();
    scene.Render();
    CHECK(log.size() == 5); // invalid positions sort as zero instead of corrupting the order
    recorders[0]->customDepth = true;
    recorders[0]->depth = NAN;
    recorders[1]->customDepth = true;
    recorders[1]->depth = -10;
    recorders[2]->customDepth = true;
    recorders[2]->depth = 10;
    log.clear();
    scene.Render();
    CHECK((log == std::vector<int>{5, 2, 1, 3, 4})); // custom NaN keys also sort as zero
    recorders[0]->depth = INFINITY;
    log.clear();
    scene.Render();
    CHECK((log == std::vector<int>{5, 2, 1, 3, 4}));
}

static void Particles() {
    auto object = CreateRef<GameObject>();
    auto emitter = object->AddComponent<ParticleEmitter>();
    emitter->emitting = false;
    emitter->maxParticles = 3;
    emitter->lifetimeMin = emitter->lifetimeMax = 1;
    emitter->startSizeMin = emitter->startSizeMax = 4;
    emitter->endSizeScale = 0.5f;
    emitter->startColor = {200, 100, 0, 128};
    emitter->endColor = {0, 100, 200, 0};
    emitter->startVelocityMin = emitter->startVelocityMax = {100, 0};
    emitter->acceleration = {0, 0};
    object->Start();
    emitter->Burst(10);
    CHECK(emitter->GetParticleCount() == 3); // capacity changed after construction is honored
    const Particle& first = emitter->GetParticles()[0];
    CHECK(first.color.r == 200 && first.color.a == 128); // the starting alpha is kept
    GameTime::Advance(0.5f, 0.5f);
    emitter->Update();
    const Particle& half = emitter->GetParticles()[0];
    CHECK(half.color.r == 100 && half.color.g == 100 && half.color.b == 100 && half.color.a == 64);
    CHECK(Approx(half.size, 3) && Approx(half.position.x, 50, 0.01f));

    emitter->Clear();
    emitter->drag = 2;
    emitter->Burst(1);
    GameTime::Advance(0.5f, 0.5f);
    emitter->Update();
    CHECK(Approx(emitter->GetParticles()[0].velocity.x, 100 * std::exp(-1.0f), 0.01f));
    GameTime::Advance(0.6f, 0.6f);
    emitter->Update();
    CHECK(emitter->GetParticleCount() == 0);
    emitter->maxParticles = 0;
    emitter->Burst(5);
    CHECK(emitter->GetParticleCount() == 0);
    GameTime::Reset();
}

static Collider* AddCircle(Scene& scene, Vector2 at, float radius, int layer = 0,
                           bool trigger = false) {
    auto object = scene.CreateGameObject();
    object->transform->position = at;
    auto collider = object->AddComponent<CircleCollider>(radius);
    collider->layer = layer;
    collider->isTrigger = trigger;
    return collider.get();
}

static void Queries() {
    Scene scene;
    Collider* a = AddCircle(scene, {0, 0}, 10);
    Collider* b = AddCircle(scene, {50, 0}, 10, 1);
    Collider* trigger = AddCircle(scene, {100, 0}, 10, 0, true);
    Collider* beside = AddCircle(scene, {150, 18}, 5);
    auto boxObject = scene.CreateGameObject();
    boxObject->transform->position = {200, 0};
    Collider* box = boxObject->AddComponent<BoxCollider>(Vector2{40, 20}).get();
    scene.Start();
    auto& world = scene.GetPhysicsWorld();

    auto near = world.OverlapCircle({0, 0}, 45);
    CHECK(near.size() == 2 && near[0] == a && near[1] == b);
    auto layered = world.OverlapCircle({0, 0}, 45, 1u << 1);
    CHECK(layered.size() == 1 && layered[0] == b);
    CHECK(world.OverlapCircle({100, 0}, 1).size() == 1 &&
          world.OverlapCircle({100, 0}, 1, ~0u, false).empty());
    auto boxed = world.OverlapBox({190, -5, 4, 4});
    CHECK(boxed.size() == 1 && boxed[0] == box);
    CHECK(world.OverlapCircle({0, 0}, -1).empty() && world.OverlapCircle({0, 0}, NAN).empty());

    auto line = world.SegmentCast({-30, 0}, {300, 0});
    CHECK(line.size() == 4 && line[0].collider == a && line[1].collider == b &&
          line[2].collider == trigger && line[3].collider == box);
    CHECK(Approx(line[0].point.x, -10, 0.01f) && Approx(line[0].fraction, 20.0f / 330, 0.0001f));
    CHECK(Approx(line[3].point.x, 180, 0.01f));
    auto thick = world.SegmentCast({-30, 0}, {300, 0}, 14);
    CHECK(thick.size() == 5 && std::any_of(thick.begin(), thick.end(), [&](const SegmentHit& h) {
              return h.collider == beside;
          }));
    CHECK(world.SegmentCast({-30, 0}, {300, 0}, 0, ~0u, false).size() == 3);
    auto inside = world.SegmentCast({0, 0}, {5, 0});
    CHECK(inside.size() == 1 && inside[0].fraction == 0);
    CHECK(world.SegmentCast({0, 40}, {0, 40}).empty());

    SegmentHit hit;
    CHECK(world.Raycast({-30, 0}, {2, 0}, 1000, &hit) && hit.collider == a &&
          Approx(hit.point.x, -10, 0.01f));
    CHECK(!world.Raycast({-30, 0}, {-1, 0}, 1000, &hit) && !world.Raycast({0, 0}, {0, 0}, 10));
    CHECK(world.Raycast({0, 30}, {0, -1}, 100, &hit, 1u << 1) == false);

    a->GetGameObject()->active = false;
    CHECK(world.OverlapCircle({0, 0}, 5).empty());
    a->GetGameObject()->active = true;
    b->SetEnabled(false);
    CHECK(world.OverlapCircle({50, 0}, 5).empty());
    CHECK(world.GetContactCount() == 0); // queries never create contacts
}

struct EnterLog : Component {
    std::vector<std::string>* log = nullptr;
    void OnTriggerEnter(Collider* other) override {
        log->push_back(gameObject->name + ">" + other->GetGameObject()->name);
    }
    void OnTriggerExit(Collider* other) override {
        log->push_back(gameObject->name + "<" + other->GetGameObject()->name);
    }
};

static void Broadphase() {
    Scene pruned, brute;
    pruned.GetPhysicsWorld().SetBroadphaseThreshold(0);
    brute.GetPhysicsWorld().SetBroadphaseThreshold(std::numeric_limits<size_t>::max());
    CHECK(PhysicsWorld().GetBroadphaseThreshold() == 48);
    std::vector<std::string> prunedLog, bruteLog;
    std::uint32_t seed = 12345;
    auto next = [&] {
        seed = seed * 1664525u + 1013904223u;
        return (seed >> 8) / float(1 << 24);
    };
    for (int i = 0; i < 160; ++i) {
        Vector2 at = {next() * 600, next() * 400};
        float radius = 4 + next() * 14;
        bool box = i % 5 == 0;
        for (auto* scene : {&pruned, &brute}) {
            auto object = scene->CreateGameObject(std::to_string(i));
            object->transform->position = at;
            Ref<Collider> collider;
            if (box)
                collider = object->AddComponent<BoxCollider>(Vector2{radius * 2, radius});
            else
                collider = object->AddComponent<CircleCollider>(radius);
            collider->isTrigger = true;
            collider->layer = i % 3;
            if (i % 7 == 0)
                collider->mask = ~(1u << 2);
            object->AddComponent<EnterLog>()->log = scene == &pruned ? &prunedLog : &bruteLog;
        }
    }
    pruned.Start();
    brute.Start();
    pruned.GetPhysicsWorld().Step();
    brute.GetPhysicsWorld().Step();
    CHECK(!bruteLog.empty() && prunedLog == bruteLog);
    CHECK(pruned.GetPhysicsWorld().GetContactCount() == brute.GetPhysicsWorld().GetContactCount());
    for (int i = 0; i < 160; i += 3) {
        pruned.GetAllGameObjects()[i]->transform->position.x += 37;
        brute.GetAllGameObjects()[i]->transform->position.x += 37;
    }
    pruned.GetAllGameObjects()[1]->transform->position.x = NAN;
    brute.GetAllGameObjects()[1]->transform->position.x = NAN;
    prunedLog.clear();
    bruteLog.clear();
    pruned.GetPhysicsWorld().Step();
    brute.GetPhysicsWorld().Step();
    // Enters follow pair order and must match exactly. Exits are dispatched in contact-table
    // order, which is keyed by collider address and so differs between two worlds.
    auto split = [](const std::vector<std::string>& log, bool exits) {
        std::vector<std::string> out;
        for (const auto& entry : log)
            if ((entry.find('<') != std::string::npos) == exits)
                out.push_back(entry);
        if (exits)
            std::sort(out.begin(), out.end());
        return out;
    };
    CHECK(!split(bruteLog, false).empty() && split(prunedLog, false) == split(bruteLog, false));
    CHECK(!split(bruteLog, true).empty() && split(prunedLog, true) == split(bruteLog, true));
    CHECK(pruned.GetPhysicsWorld().GetContactCount() == brute.GetPhysicsWorld().GetContactCount());
}

static void TimeScale() {
    LogicEngine engine;
    engine.SetFixedTimeStep(0.125f);
    engine.LoadScene(CreateRef<Scene>());
    engine.Tick(0);
    engine.SetTimeScale(2);
    engine.Tick(0.125f);
    CHECK(engine.GetFrameTiming().fixedSteps == 2 && Approx(GameTime::DeltaTime(), 0.25f));
    CHECK(Approx(GameTime::UnscaledDeltaTime(), 0.125f) && Approx(GameTime::GetTimeScale(), 2));
    engine.Tick(1.0f);
    CHECK(engine.GetFrameTiming().fixedSteps == 16 &&
          engine.GetFrameTiming().droppedSeconds < 1e-6);
    engine.Tick(2.0f);
    CHECK(engine.GetFrameTiming().fixedSteps == 16 &&
          Approx((float)engine.GetFrameTiming().droppedSeconds, 2));
    engine.SetTimeScale(0.5f);
    engine.Tick(0.125f);
    CHECK(engine.GetFrameTiming().fixedSteps == 0);
    engine.Tick(0.125f);
    CHECK(engine.GetFrameTiming().fixedSteps == 1);
    engine.SetTimeScale(0);
    engine.Tick(1);
    CHECK(engine.GetFrameTiming().fixedSteps == 0 && GameTime::DeltaTime() == 0 &&
          Approx(GameTime::UnscaledDeltaTime(), 1));
    engine.SetTimeScale(NAN);
    CHECK(engine.GetTimeScale() == 0);
    engine.SetTimeScale(100);
    CHECK(engine.GetTimeScale() == 8);
    engine.SetTimeScale(1);
    engine.Shutdown();
}

static void PointerCapture() {
    Display::Configure(800, 600);
    InputFrame frame;
    frame.mousePosition = {100, 100};
    Input::SetFrame(frame);
    UI::BeginFrame();
    UI::EndFrame();
    CHECK(!UI::IsMouseOverUI());
    UI::BeginFrame();
    UI::Capture({50, 50, 100, 100});
    CHECK(UI::IsMouseOverUI());
    UI::EndFrame();
    CHECK(UI::IsMouseOverUI()); // visible to the following Update
    UI::BeginFrame();
    UI::Capture({300, 300, 10, 10});
    UI::EndFrame();
    CHECK(!UI::IsMouseOverUI());
    frame.mousePosition = {-5, -5};
    Input::SetFrame(frame);
    UI::BeginFrame();
    UI::Capture({-10, -10, 20, 20});
    UI::EndFrame();
    CHECK(!UI::IsMouseOverUI()); // pointers outside the canvas never count

    // Update uses the new input snapshot before UI is rendered again.
    frame.mousePosition = {300, 300};
    Input::SetFrame(frame);
    UI::BeginFrame();
    UI::Capture({50, 50, 100, 100});
    UI::EndFrame();
    frame.mousePosition = {100, 100};
    Input::SetFrame(frame);
    CHECK(UI::IsMouseOverUI()); // entering the HUD and clicking must not leak to gameplay
    UI::BeginFrame();
    UI::Capture({50, 50, 100, 100});
    UI::EndFrame();
    frame.mousePosition = {300, 300};
    Input::SetFrame(frame);
    CHECK(!UI::IsMouseOverUI()); // leaving the HUD releases capture immediately
    frame.mousePosition = {100, 100};
    Input::SetFrame(frame);
    frame.focused = false;
    Input::SetFrame(frame);
    CHECK(!UI::IsMouseOverUI());
    frame.focused = true;
    Input::SetFrame(frame);
    UI::BeginFrame();
    UI::Capture({50, 50, 100, 100});
    UI::EndFrame();
    CHECK(UI::IsMouseOverUI());
    UI::ResetInteraction();
    CHECK(!UI::IsMouseOverUI()); // scene transitions must release old UI regions
    Input::Reset();
}

static void AudioChannels() {
    CHECK(!AudioManager::PlaySound("missing") && AudioManager::GetVoiceCount("missing") == 0);
    AudioManager::SetChannelVolume(AudioChannel::Sfx, 0.25f);
    CHECK(Approx(AudioManager::GetChannelVolume(AudioChannel::Sfx), 0.25f));
    AudioManager::SetChannelVolume(AudioChannel::Music, 3);
    AudioManager::SetChannelVolume(AudioChannel::Music, NAN);
    CHECK(Approx(AudioManager::GetChannelVolume(AudioChannel::Music), 1));
    AudioManager::SetChannelVolume(AudioChannel::Sfx, -1);
    CHECK(AudioManager::GetChannelVolume(AudioChannel::Sfx) == 0);
    AudioManager::SetChannelVolume(AudioChannel::Sfx, 1);
}

static void Codepoints() {
    auto points = ResourceManager::CodepointsFromText("\u5149\u4e4b\u5149 A\n\xff", false);
    CHECK((points == std::vector<int>{' ', 'A', 0x4e4b, 0x5149}));
    CHECK(ResourceManager::CodepointsFromText("", true).size() == 95);
    CHECK(ResourceManager::CodepointsFromText("\xc0\xaf\xed\xa0\x80\xe4\xb8", false).empty());
    auto emoji = ResourceManager::CodepointsFromText("\U0001F600", false);
    CHECK(emoji.size() == 1 && emoji[0] == 0x1f600);
}

static void Run() {
    printf("[engine extensions]\n");
    DepthSorting();
    Particles();
    Queries();
    Broadphase();
    TimeScale();
    PointerCapture();
    AudioChannels();
    Codepoints();
}

} // namespace extension_tests
