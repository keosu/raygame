// Hidden-window integration check for the real examples and graphics resources.
#include "RayGame.h"
#include <cstdio>
#include <cmath>

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

class SmokeGame : public GameEngine {
public:
    int failures = 0;
    SmokeGame() : GameEngine("RayGame smoke test", 960, 900) { targetFPS = 120; }

    void Expect(bool value, const char* message) {
        if (!value) { ++failures; std::printf("FAIL: %s\n", message); }
    }

    void OnInit() override { LoadScene(CreateRef<shooter::SpaceShooterScene>()); }

    void OnUpdate() override {
        int frame = GameTime::GetFrameCount();
        if (frame == 5) {
            auto scene = std::static_pointer_cast<shooter::SpaceShooterScene>(GetCurrentScene());
            scene->SpawnEnemy();
            scene->SpawnBullet({400, 400});
            scene->SpawnExplosion({400, 300});
        }
        if (frame == 25) LoadScene(CreateRef<balls::BouncingBallsScene>());
        if (frame == 30) {
            std::static_pointer_cast<balls::BouncingBallsScene>(GetCurrentScene())->AddRandomBall();
            auto tex = ResourceManager::GetTexture("ball0");
            auto cached = ResourceManager::CreateCircleTexture("ball0", 16, WHITE);
            Expect(tex && tex == cached, "procedural textures reuse their named resource");
        }
        if (frame == 50) {
            auto scene = CreateRef<breakout::BreakoutScene>();
            scene->savePath = "test_breakout_smoke.tmp";
            LoadScene(scene);
        }
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
            scene->Restart();
            Expect(scene->bricksLeft == 60 && scene->lives == 3, "breakout restarts cleanly");
        }
        if (frame == 75) LoadScene(CreateRef<bullethell::BulletHellScene>());
        if (frame == 80 || frame == 95) {
            auto scene = std::static_pointer_cast<bullethell::BulletHellScene>(GetCurrentScene());
            auto turrets = scene->FindGameObjectsWithComponent<bullethell::Turret>();
            Expect(turrets.size() == 3, "boss starts three turrets");
            for (const auto& turret : turrets) {
                Expect(turret->GetComponent<bullethell::Turret>()->field == scene->enemyField,
                       "turrets use the configured bullet field after restart");
            }
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
    std::remove("test_breakout_smoke.tmp");
    std::printf("Example smoke tests: %d failures\n", game.failures);
    return game.failures == 0 ? 0 : 1;
}
