// RayGame logic tests — no window/GL required.
#include "RayGame.h"
#include <cstdio>
#include <cmath>

using namespace RayGame;

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  FAIL: %s  (line %d)\n", #cond, __LINE__); } \
} while (0)

static bool Approx(float a, float b, float eps = 0.001f) { return std::fabs(a - b) < eps; }

static void TestMath() {
    printf("[math]\n");
    CHECK(Approx(Lerp(0.0f, 10.0f, 0.5f), 5.0f));
    CHECK(Approx(Clamp(15.0f, 0.0f, 10.0f), 10.0f));
    CHECK(Approx(Clamp(-5.0f, 0.0f, 10.0f), 0.0f));
    CHECK(Approx(Distance({0,0}, {3,4}), 5.0f));
    Vector2 n = Normalize({0, 5});
    CHECK(Approx(n.x, 0) && Approx(n.y, 1));
}

static void TestTween() {
    printf("[tween]\n");
    CHECK(Approx(ApplyEase(Ease::Linear, 0.5f), 0.5f));
    CHECK(Approx(ApplyEase(Ease::QuadIn, 0.5f), 0.25f));
    CHECK(Approx(ApplyEase(Ease::CubicOut, 1.0f), 1.0f));

    float value = 0;
    bool done = false;
    Tween::To(0, 100, 1.0f, Ease::Linear,
              [&](float v) { value = v; },
              [&]() { done = true; });
    TweenManager::Update(0.5f);
    CHECK(Approx(value, 50.0f));
    TweenManager::Update(0.6f);
    CHECK(Approx(value, 100.0f));
    CHECK(done);
    TweenManager::Clear();
}

static void TestPool() {
    printf("[pool]\n");
    ObjectPool<int> pool(2);
    CHECK(pool.Capacity() == 2);
    CHECK(pool.Available() == 2);
    int* a = pool.Acquire();
    int* b = pool.Acquire();
    CHECK(pool.InUse() == 2);
    int* c = pool.Acquire();  // grows
    CHECK(pool.Capacity() == 3);
    pool.Release(a);
    CHECK(pool.Available() == 1);
    (void)b; (void)c;
}

static void TestEvents() {
    printf("[events]\n");
    int counter = 0;
    auto id = EventBus::Subscribe("score", [&](const EventData& d) { counter += d.i; });
    EventBus::Emit("score", EventData{10});
    EventBus::Emit("score", EventData{5});
    CHECK(counter == 15);
    EventBus::Unsubscribe("score", id);
    EventBus::Emit("score", EventData{100});
    CHECK(counter == 15);
    EventBus::Clear();
}

static void TestSave() {
    printf("[save]\n");
    SaveSystem save;
    save.SetInt("highscore", 4200);
    save.SetFloat("volume", 0.75f);
    save.SetBool("muted", true);
    save.SetString("name", "player one");
    const char* path = "test_save.tmp";
    CHECK(save.Save(path));

    SaveSystem loaded;
    CHECK(loaded.Load(path));
    CHECK(loaded.GetInt("highscore") == 4200);
    CHECK(Approx(loaded.GetFloat("volume"), 0.75f));
    CHECK(loaded.GetBool("muted") == true);
    CHECK(loaded.GetString("name") == "player one");
    CHECK(loaded.GetInt("missing", -1) == -1);
    remove(path);
}

static void TestTransformHierarchy() {
    printf("[transform]\n");
    auto parent = CreateRef<GameObject>("parent");
    auto child = CreateRef<GameObject>("child");
    parent->transform->position = {100, 100};
    child->transform->SetParent(parent->transform.get(), false);
    child->transform->position = {10, 0};  // local
    Vector2 w = child->transform->GetWorldPosition();
    CHECK(Approx(w.x, 110) && Approx(w.y, 100));

    parent->transform->rotation = 90;
    w = child->transform->GetWorldPosition();
    // local (10,0) rotated 90deg -> (0,10), plus parent (100,100)
    CHECK(Approx(w.x, 100, 0.01f) && Approx(w.y, 110, 0.01f));
}

// Captures collision callbacks.
class HitCounter : public Component {
public:
    int enters = 0, exits = 0, triggers = 0;
    void OnCollisionEnter(Collider*) override { enters++; }
    void OnCollisionExit(Collider*) override { exits++; }
    void OnTriggerEnter(Collider*) override { triggers++; }
};

static void TestPhysicsCallbacks() {
    printf("[physics]\n");
    Scene scene;

    auto a = scene.CreateGameObject("a");
    auto b = scene.CreateGameObject("b");
    a->transform->position = {0, 0};
    b->transform->position = {10, 0};
    auto ca = a->AddComponent<CircleCollider>(20.0f);
    auto cb = b->AddComponent<CircleCollider>(20.0f);
    auto counter = a->AddComponent<HitCounter>();
    // Register colliders (normally done by Scene::Start).
    a->Start(); b->Start();

    scene.GetPhysicsWorld().Step();              // overlapping -> enter
    CHECK(counter->enters == 1);
    scene.GetPhysicsWorld().Step();              // still overlapping -> stay (no new enter)
    CHECK(counter->enters == 1);

    b->transform->position = {100, 0}; // move apart
    scene.GetPhysicsWorld().Step();              // exit
    CHECK(counter->exits == 1);

    // Box vs Circle
    scene.GetPhysicsWorld().Clear();
    auto boxObj = scene.CreateGameObject("box");
    auto cirObj = scene.CreateGameObject("cir");
    boxObj->transform->position = {0, 0};
    cirObj->transform->position = {15, 0};
    auto box = boxObj->AddComponent<BoxCollider>(Vector2{40, 40});
    auto cir = cirObj->AddComponent<CircleCollider>(10.0f);
    cir->isTrigger = true;
    auto tc = cirObj->AddComponent<HitCounter>();
    boxObj->Start(); cirObj->Start();
    scene.GetPhysicsWorld().Step();
    CHECK(tc->triggers == 1);
    scene.GetPhysicsWorld().Clear();
}

#include "regressions.h"
#include "foundation.h"
#include "runtime_contracts.h"
#include "worlds_and_timing.h"
#include "touch_layout.h"
#include "engine_extensions.h"
#include "speech.h"
#include "pointer_gesture.h"

int main() {
    printf("RayGame tests\n=============\n");
    TestMath();
    TestTween();
    TestPool();
    TestEvents();
    TestSave();
    TestTransformHierarchy();
    TestPhysicsCallbacks();
    TestRegressions();
    TestFoundation();
    TestRuntimeContracts();
    world_tests::Run();
    TestTouchLayout();
    extension_tests::Run();
    TestSpeech();
    TestPointerGesture();

    printf("=============\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
