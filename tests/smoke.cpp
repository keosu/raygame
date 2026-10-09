// Hidden-window integration check for the real examples and graphics resources.
#include "RayGame.h"
#include "../examples/common/ExampleUI.h"
#include <cstdio>
#include <cmath>
#include <filesystem>

#define main example_main
namespace shooter {
#include "../examples/space_shooter/main.cpp"
}
namespace balls {
#include "../examples/bouncing_balls/main.cpp"
}
namespace breakout {
#include "../examples/neon_breakout/main.cpp"
}
namespace bullethell {
#include "../examples/bullet_hell/main.cpp"
}
#undef main

using namespace RayGame;

bool NativeWindowThemeMatchesSystem(void* window);

class SmokeGame : public GameEngine {
public:
    int failures = 0;
    int savedWidth = 0, savedHeight = 0;
    Vector2 savedPosition = {0, 0};
    SmokeGame() : GameEngine("RayGame smoke test", 960, 900) { targetFPS = 120; }

    void Expect(bool value, const char* message) {
        if (!value) { ++failures; std::printf("FAIL: %s\n", message); }
    }

    void OnInit() override {
        Expect(NativeWindowThemeMatchesSystem(GetWindowHandle()), "native title bar follows the system theme");
        SetCanvasSize(800, 600);
        LoadScene(CreateRef<shooter::SpaceShooterScene>());
    }

    void Capture(const char* name) {
        std::filesystem::path directory = std::filesystem::path(GetApplicationDirectory()) / "screenshots";
        std::filesystem::create_directories(directory);
        Image frame = LoadImageFromTexture(Display::GetCanvasTexture());
        ImageFlipVertical(&frame);
        const auto viewport = Display::GetViewport();
        ImageResize(&frame, (int)std::lround(viewport.bounds.width), (int)std::lround(viewport.bounds.height));
        std::string path = (directory / (std::string(name) + ".png")).string();
        Expect(ExportImage(frame, path.c_str()), "save rendered example preview");
        UnloadImage(frame);
    }

    void OnUpdate() override {
        int frame = GameTime::GetFrameCount();
        if (frame == 2) {
            Expect(UI::font && UI::font->baseSize == 64, "bundled body font loads");
            Expect(UI::headingFont && UI::headingFont->baseSize == 96, "bundled heading font loads");
            SetWindowSize(1280, 720);
        }
        if (frame == 5) {
            auto scene = std::static_pointer_cast<shooter::SpaceShooterScene>(GetCurrentScene());
            scene->SpawnEnemy();
            scene->SpawnBullet({400, 400});
            scene->SpawnExplosion({400, 300});
        }
        if (frame == 10) Capture("space_shooter");
        if (frame == 12) {
            savedWidth = ::GetScreenWidth(); savedHeight = ::GetScreenHeight();
            savedPosition = GetWindowPosition();
            SetFullscreen(true);
            Expect(IsFullscreen(), "enter borderless fullscreen");
        }
        if (frame == 16) {
            auto viewport = Display::GetViewport();
            Vector2 center = viewport.ToCanvas({::GetScreenWidth() / 2.0f, ::GetScreenHeight() / 2.0f});
            Expect(std::fabs(center.x - 400) < 0.1f && std::fabs(center.y - 300) < 0.1f,
                   "fullscreen maps the mouse into the logical canvas");
            Expect(Display::GetCanvasTexture().width >= viewport.bounds.width,
                   "fullscreen renders enough pixels for sharp text");
            Capture("space_shooter_fullscreen");
            SetFullscreen(false);
        }
        if (frame == 20) {
            Expect(NativeWindowThemeMatchesSystem(GetWindowHandle()), "fullscreen preserves the native title bar theme");
            Expect(!IsFullscreen() && ::GetScreenWidth() == savedWidth && ::GetScreenHeight() == savedHeight,
                   "fullscreen restores the resized window dimensions");
            Vector2 position = GetWindowPosition();
            Expect(std::fabs(position.x - savedPosition.x) < 2 && std::fabs(position.y - savedPosition.y) < 2,
                   "fullscreen restores the window position");
        }
        if (frame == 25) LoadScene(CreateRef<balls::BouncingBallsScene>());
        if (frame == 30) {
            std::static_pointer_cast<balls::BouncingBallsScene>(GetCurrentScene())->AddRandomBall();
            auto tex = ResourceManager::GetTexture("ball");
            auto cached = ResourceManager::CreateCircleTexture("ball", 16, WHITE);
            Expect(tex && tex == cached, "procedural textures reuse their named resource");
        }
        if (frame == 35) Capture("bouncing_balls");
        if (frame == 50) {
            SetCanvasSize(960, 720);
            SetClearColor({8, 10, 24, 255});
            auto scene = CreateRef<breakout::BreakoutScene>();
            scene->savePath = "test_breakout_smoke.tmp";
            LoadScene(scene);
        }
        if (frame == 58) Capture("neon_breakout");
        if (frame == 55) {
            auto scene = std::static_pointer_cast<breakout::BreakoutScene>(GetCurrentScene());
            auto bricks = scene->FindGameObjectsByTag("Brick");
            auto ball = scene->ball->GetComponent<breakout::Ball>();
            scene->ball->transform->position = bricks.front()->transform->position;
            ball->launched = true;
            ball->velocity = {0, 0};
        }
        if (frame == 60) {
            auto scene = std::static_pointer_cast<breakout::BreakoutScene>(GetCurrentScene());
            Expect(scene->bricksLeft < 60 && scene->score > 0, "ball trigger destroys a brick and awards score");
            scene->EndGame(false);
            int lives = scene->lives;
            scene->LoseLife();
            Expect(scene->lives == lives, "game over stops life loss");
        }
        if (frame == 62) {
            Capture("neon_breakout_result");
            auto scene = std::static_pointer_cast<breakout::BreakoutScene>(GetCurrentScene());
            scene->Restart();
            Expect(scene->bricksLeft == 60 && scene->lives == 3, "breakout restarts cleanly");
        }
        if (frame == 75) {
            SetCanvasSize(820, 900);
            LoadScene(CreateRef<bullethell::BulletHellScene>());
        }
        if (frame == 85) Capture("bullet_hell");
        if (frame == 80 || frame == 95) {
            auto scene = std::static_pointer_cast<bullethell::BulletHellScene>(GetCurrentScene());
            auto turrets = scene->FindGameObjectsWithComponent<bullethell::Turret>();
            Expect(turrets.size() == 3, "boss starts three turrets");
            for (const auto& turret : turrets) {
                Expect(turret->GetComponent<bullethell::Turret>()->field == scene->enemyField,
                       "turrets use the configured bullet field after restart");
            }
            scene->boss->GetComponent<bullethell::Boss>()->ringTimer = 2;
        }
        if (frame == 90) {
            auto scene = std::static_pointer_cast<bullethell::BulletHellScene>(GetCurrentScene());
            scene->Restart();
        }
        if (frame >= 110) TestAspectMatrix(frame - 110);
    }

    void TestAspectMatrix(int frame) {
        constexpr Vector2 sizes[] = {{1280,720}, {1680,720}, {540,960}, {960,720}};
        constexpr const char* aspects[] = {"16x9", "21x9", "9x16", "4x3"};
        constexpr const char* games[] = {"space_shooter", "bouncing_balls", "neon_breakout", "bullet_hell"};
        int sample = frame / 6, phase = frame % 6;
        if (sample >= 16) { Quit(); return; }
        int game = sample / 4, aspect = sample % 4;
        if (phase == 0) {
            SetCanvasScaleMode(CanvasScaleMode::Expand);
            if (aspect == 0) {
                if (game == 0) { SetCanvasSize(800, 600); LoadScene(CreateRef<shooter::SpaceShooterScene>()); }
                if (game == 1) { SetCanvasSize(800, 600); LoadScene(CreateRef<balls::BouncingBallsScene>()); }
                if (game == 2) {
                    SetCanvasSize(960, 720);
                    auto scene = CreateRef<breakout::BreakoutScene>();
                    scene->savePath = "test_breakout_smoke.tmp";
                    LoadScene(scene);
                }
                if (game == 3) { SetCanvasSize(820, 900); LoadScene(CreateRef<bullethell::BulletHellScene>()); }
            }
            SetWindowSize((int)sizes[aspect].x, (int)sizes[aspect].y);
        }
        if (phase == 3) {
            auto view = Display::GetViewport();
            Expect(std::fabs(view.bounds.x) < 0.01f && std::fabs(view.bounds.y) < 0.01f
                && std::fabs(view.bounds.width - ::GetScreenWidth()) < 0.01f
                && std::fabs(view.bounds.height - ::GetScreenHeight()) < 0.01f, "adaptive canvas fills each window ratio");
            auto end = Display::ScreenToCanvas({(float)::GetScreenWidth(), (float)::GetScreenHeight()});
            Expect(std::fabs(end.x - Display::GetWidth()) < 0.01f && std::fabs(end.y - Display::GetHeight()) < 0.01f,
                "adaptive mouse reaches the entire canvas");
            if (game == 1) {
                auto scene = std::static_pointer_cast<balls::BouncingBallsScene>(GetCurrentScene());
                Expect(std::fabs(scene->arena.width - Display::GetWidth() + 48) < 0.01f
                    && std::fabs(scene->arena.height - Display::GetHeight() + 146) < 0.01f, "ball arena follows the viewport");
                for (auto& object : scene->GetAllGameObjects())
                    Expect(CheckCollisionPointRec(object->transform->position, scene->arena), "balls stay in the resized arena");
            }
            if (game == 2) {
                auto scene = std::static_pointer_cast<breakout::BreakoutScene>(GetCurrentScene());
                Expect(std::fabs(scene->paddle->transform->position.y - Display::GetHeight() + 60) < 0.01f,
                    "paddle remains anchored to the bottom after resize");
                auto bricks = scene->FindGameObjectsByTag("Brick");
                Expect(bricks.size() == 60, "resize preserves the brick layout");
                float left = bricks.front()->transform->position.x;
                float right = bricks[9]->transform->position.x;
                Expect(std::fabs((left + right) / 2 - Display::GetWidth() / 2) < 0.01f, "brick rows remain centered");
            }
        }
        if (phase == 4) {
            std::string name = std::string(games[game]) + "_" + aspects[aspect];
            Capture(name.c_str());
        }
    }
};

class FailingGame : public GameEngine {
public:
    bool failInit;
    explicit FailingGame(bool value) : GameEngine("failure recovery test", 320, 240), failInit(value) {}
    void OnInit() override {
        ResourceManager::CreateColorTexture("failure-test", 8, 8, WHITE);
        if (failInit) throw std::runtime_error("injected initialization error");
        LoadScene(CreateRef<Scene>());
    }
    void OnRender() override { throw std::runtime_error("injected render error"); }
};

int main() {
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SmokeGame game;
    game.Run();
    game.Expect(PhysicsWorld::GetColliderCount() == 0, "shutdown clears physics");
    game.Expect(TweenManager::Count() == 0, "shutdown clears tweens");
    game.Expect(UI::font == nullptr && UI::headingFont == nullptr, "shutdown resets cached UI font pointers");
    game.Expect(Display::GetCanvasTexture().id == 0, "shutdown releases the game canvas");
    for (bool failInit : {true, false}) {
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        bool caught = false;
        FailingGame failure(failInit);
        try { failure.Run(); } catch (const std::runtime_error&) { caught = true; }
        game.Expect(caught, "initialization/render exceptions propagate to the caller");
        game.Expect(!IsWindowReady() && !IsAudioDeviceReady(), "exceptions close native devices");
        game.Expect(ResourceManager::GetTexture("failure-test") == nullptr && Display::GetCanvasTexture().id == 0,
            "exceptions release graphics resources");
        failure.Shutdown();
    }
    std::remove("test_breakout_smoke.tmp");
    std::remove("test_breakout_smoke.tmp.bak");
    std::printf("Example smoke tests: %d failures\n", game.failures);
    return game.failures == 0 ? 0 : 1;
}
