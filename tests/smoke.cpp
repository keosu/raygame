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
        if (frame >= 110) Quit();
    }
};

int main() {
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SmokeGame game;
    game.Run();
    game.Expect(PhysicsWorld::GetColliderCount() == 0, "shutdown clears physics");
    game.Expect(TweenManager::Count() == 0, "shutdown clears tweens");
    game.Expect(UI::font == nullptr && UI::headingFont == nullptr, "shutdown resets cached UI font pointers");
    game.Expect(Display::GetCanvasTexture().id == 0, "shutdown releases the game canvas");
    std::remove("test_breakout_smoke.tmp");
    std::printf("Example smoke tests: %d failures\n", game.failures);
    return game.failures == 0 ? 0 : 1;
}
