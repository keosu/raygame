#pragma once
#include <chrono>

#ifdef _WIN32
bool AtomicReplacementPreservesLockedFile(const std::filesystem::path& path);
#endif

namespace foundation_tests {
struct Cleanup : Component {
    int* destroyed = nullptr;
    std::function<void()> cleanup;
    bool failDisable = false;
    void OnDisable() override { if (failDisable) throw std::runtime_error("disable failure"); }
    void OnDestroy() override { if (destroyed) ++*destroyed; if (cleanup) cleanup(); }
};

static void Lifecycle() {
    Scene scene;
    int destroyed = 0;
    bool rejected = false, clearing = false;
    auto retained = scene.CreateGameObject();
    auto probe = retained->AddComponent<Cleanup>();
    probe->destroyed = &destroyed;
    probe->failDisable = true;
    probe->cleanup = [&] {
        clearing = scene.IsClearing();
        try { scene.CreateGameObject("resurrected"); }
        catch (const std::logic_error&) { rejected = true; }
        throw std::runtime_error("destroy failure");
    };
    auto sibling = scene.CreateGameObject()->AddComponent<Cleanup>();
    sibling->destroyed = &destroyed;
    scene.Clear();
    scene.Update();
    CHECK(clearing && rejected && !scene.IsClearing());
    CHECK(scene.GetAllGameObjects().empty() && destroyed == 2);
    CHECK(retained->scene == nullptr && probe->GetGameObject() == nullptr);
    scene.Clear();
    CHECK(destroyed == 2);

    // An exception after a deferred Clear must not leave the scene stuck in traversal.
    auto updating = scene.CreateGameObject()->AddComponent<LifecycleProbe>();
    Ref<GameObject> pending;
    updating->onUpdate = [&] {
        pending = scene.CreateGameObject();
        scene.Clear();
        throw std::runtime_error("update failure");
    };
    bool threw = false;
    try { scene.Update(); } catch (const std::runtime_error&) { threw = true; }
    scene.Clear();
    CHECK(threw && !scene.IsClearing() && scene.GetAllGameObjects().empty());
    CHECK(pending && pending->IsDestroyed() && pending->scene == nullptr);
    CHECK(updating->destroys == 1);
    auto fresh = scene.CreateGameObject();
    scene.Update();
    CHECK(fresh->scene == &scene && !fresh->IsDestroyed());
    scene.Clear();

    struct FailingUnload : CountingScene {
        void OnUnload() override { ++unloads; throw std::runtime_error("unload failure"); }
    };
    auto loaded = CreateRef<FailingUnload>();
    {
        LogicEngine engine;
        engine.LoadScene(loaded);
        engine.Tick(0);
    }
    CHECK(loaded->unloads == 1 && loaded->probe->destroys == 1);
    CHECK(loaded->retained->scene == nullptr && loaded->GetPhysicsWorld().GetColliderCount() == 0);

    // A throwing callback must not permanently latch a global scheduler's guard.
    Tween::Delay(0, [] { throw std::runtime_error("tween failure"); });
    try { TweenManager::Update(0); } catch (const std::runtime_error&) {}
    TweenManager::Clear();
    int completed = 0;
    Tween::Delay(0, [&] { ++completed; });
    TweenManager::Update(0);
    CHECK(completed == 1);
    auto a = scene.CreateGameObject();
    a->AddComponent<CircleCollider>()->isTrigger = true;
    auto trigger = a->AddComponent<LifecycleProbe>();
    trigger->onHit = [] { throw std::runtime_error("physics callback failure"); };
    scene.CreateGameObject()->AddComponent<CircleCollider>();
    scene.Start();
    try { scene.GetPhysicsWorld().Step(); } catch (const std::runtime_error&) {}
    trigger->onHit = [&] { ++completed; };
    scene.GetPhysicsWorld().Step();
    CHECK(completed == 2);
    scene.Clear();
    scene.GetPhysicsWorld().Clear();
}

static void TimingAndAspect() {
    GameTime::Reset();
    LogicEngine engine;
    auto scene = CreateRef<CountingScene>();
    engine.LoadScene(scene);
    engine.Tick(1);
    CHECK(Approx(GameTime::DeltaTime(), 8.0f / 60));
    CHECK(Approx(GameTime::GetTime(), 8.0f / 60));
    CHECK(Approx(GameTime::UnscaledDeltaTime(), 1) && std::fabs(GameTime::UnscaledTime() - 1) < 0.001);
    CHECK(scene->probe->fixed == 8);
    engine.SetPaused(true);
    engine.Tick(3);
    CHECK(Approx(GameTime::DeltaTime(), 0) && std::fabs(GameTime::UnscaledTime() - 4) < 0.001);
    CHECK(scene->probe->fixed == 8 && Approx(GameTime::GetTime(), 8.0f / 60));
    engine.Shutdown();
    for (auto window : {Vector2{1280,720}, Vector2{1680,720}, Vector2{540,960}, Vector2{960,720}}) {
        auto viewport = CanvasViewport::Calculate(800, 600, (int)window.x, (int)window.y, CanvasScaleMode::Expand);
        CHECK(Approx(viewport.bounds.x, 0) && Approx(viewport.bounds.y, 0));
        CHECK(Approx(viewport.bounds.width, window.x) && Approx(viewport.bounds.height, window.y));
        CHECK(viewport.logicalSize.x >= 799.99f && viewport.logicalSize.y >= 599.99f);
        auto center = viewport.ToCanvas({window.x / 2, window.y / 2});
        CHECK(Approx(center.x, viewport.logicalSize.x / 2) && Approx(center.y, viewport.logicalSize.y / 2));
        auto p = viewport.ToScreen({123, 234});
        auto q = viewport.ToCanvas(p);
        CHECK(Approx(q.x, 123) && Approx(q.y, 234));
        auto x = viewport.ToScreen({124, 234}), y = viewport.ToScreen({123, 235});
        CHECK(Approx(x.x - p.x, y.y - p.y));
    }
}

static void StorageAndDiagnostics() {
    namespace fs = std::filesystem;
    auto temp = fs::temp_directory_path();
    auto directory = temp / ("raygame-foundation-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(directory)) throw std::runtime_error("Cannot create test directory");
    auto file = directory / fs::u8path(u8"存档.txt");
    auto backup = file; backup += ".bak";
    SaveSystem first;
    first.SetInt("score", 10);
    first.SetString("name", u8"玩家\n第一名");
    CHECK(first.Save(file));
    SaveSystem loaded;
    CHECK(loaded.Load(file) && loaded.GetInt("score") == 10 && !loaded.WasRecovered());
    CHECK(loaded.GetString("name") == first.GetString("name"));
    first.SetInt("score", 20);
    CHECK(first.Save(file));
    CHECK(fs::exists(backup));
    CHECK(loaded.Load(file) && loaded.GetInt("score") == 20);
    CHECK(FileSystem::WriteAtomic(file, "# RayGame save 1 truncated", false));
    CHECK(loaded.Load(file) && loaded.WasRecovered() && loaded.GetInt("score") == 10);
    // Saving a recovered value must retain the known-good backup.
    loaded.SetInt("score", 30);
    CHECK(loaded.Save(file));
    SaveSystem previous;
    CHECK(previous.Load(backup) && previous.GetInt("score") == 10);
    CHECK(loaded.Load(file) && !loaded.WasRecovered() && loaded.GetInt("score") == 30);

    std::string content;
    CHECK(FileSystem::ReadText(file, content));
    CHECK(!FileSystem::ReadText(file, content, 4));
    auto previousContent = content;
    CHECK(FileSystem::ReadAssetText(file.u8string(), content) && content == previousContent);
    CHECK(!FileSystem::ReadAssetText(file.u8string(), content, 4) && content == previousContent);
    CHECK(!FileSystem::ReadAssetText((directory / "missing-asset").u8string(), content) && content == previousContent);
    auto emptyAsset = directory / "empty-asset.txt";
    CHECK(FileSystem::WriteAtomic(emptyAsset, "", false));
    CHECK(FileSystem::ReadAssetText(emptyAsset.u8string(), content, 0) && content.empty());
    auto blocked = directory / "blocked";
    CHECK(fs::create_directory(blocked));
    CHECK(!FileSystem::WriteAtomic(blocked, "replacement"));
    CHECK(fs::is_directory(blocked));
    auto child = file / "child";
    CHECK(!first.Save(child));
    CHECK(loaded.Load(file) && loaded.GetInt("score") == 30);
#ifdef _WIN32
    CHECK(AtomicReplacementPreservesLockedFile(directory / "locked.txt"));
#endif
    auto legacy = directory / "legacy.txt";
    CHECK(FileSystem::WriteAtomic(legacy, "highscore=42\nname=old\\=value\n", false));
    CHECK(loaded.Load(legacy) && loaded.GetInt("highscore") == 42 && loaded.GetString("name") == "old=value");
    CHECK(!loaded.Load(directory / "missing") && loaded.GetInt("highscore") == 42);
    bool invalidId = false;
    try { FileSystem::UserDataDirectory("../escape"); }
    catch (const std::invalid_argument&) { invalidId = true; }
    CHECK(invalidId);

    auto log = directory / "game.log";
    CHECK(Diagnostics::Initialize(log, false, 1024));
    for (int i = 0; i < 30; ++i) Diagnostics::Write(LogLevel::Info, std::string(100, 'x'));
    Diagnostics::Write(LogLevel::Error, "failure marker");
    Diagnostics::Shutdown();
    auto priorLog = log; priorLog += ".previous";
    CHECK(fs::exists(priorLog));
    CHECK(FileSystem::ReadText(log, content) && content.find("failure marker") != std::string::npos);
    CHECK(fs::file_size(log) <= 1024 && fs::file_size(priorLog) <= 1024);
    // Only remove the files and empty directory created by this test.
    for (const auto& entry : fs::directory_iterator(directory)) fs::remove(entry.path());
    fs::remove(directory);
}
}

static void TestFoundation() {
    printf("[foundation lifecycle, timing, adaptive canvas, storage and diagnostics]\n");
    foundation_tests::Lifecycle();
    foundation_tests::TimingAndAspect();
    foundation_tests::StorageAndDiagnostics();
}
