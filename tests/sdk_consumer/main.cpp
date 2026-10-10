#include <raygame/RayGame.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace RayGame;
bool CheckIndividualHeaders();

class ConsumerGame : public GameEngine {
public:
    ConsumerGame() : GameEngine("RayGame SDK consumer", 320, 240) { targetFPS = 120; }
    int renderedFrames = 0;
    TextureHandle texture;
    Ref<GameObject> ball;

    void Step() { Tick(1.0f / 60.0f); }

protected:
    void OnInit() override {
        texture = ResourceManager::CreateCircleTexture("sdk-ball", 8, SKYBLUE);
        if (!texture) throw std::runtime_error("Consumer texture creation failed");
        auto scene = CreateRef<Scene>("consumer");
        ball = scene->CreateGameObject("ball");
        ball->transform->position = {32, 32};
        ball->AddComponent<SpriteRenderer>(texture);
        ball->AddComponent<CircleCollider>(8.0f);
        auto body = ball->AddComponent<Rigidbody>();
        body->drag = 1;
        body->velocity = {60, 0};
        LoadScene(scene);
    }
    void OnRender() override {
        ++renderedFrames;
        DrawText("Installed SDK", 16, 64, 20, WHITE);
        if (renderedFrames == 4) Quit();
    }
};

int main(int argc, char** argv) {
    try {
        bool graphics = argc == 2 && std::strcmp(argv[1], "--graphics") == 0;
        if (argc > 1 && !graphics) throw std::invalid_argument("Usage: sdk_consumer [--graphics]");
        if (!CheckIndividualHeaders()) throw std::runtime_error("Individual header/physics/input check failed");
        ConsumerGame game;
        auto scene = CreateRef<Scene>("headless consumer");
        auto ball = scene->CreateGameObject("ball");
        auto body = ball->AddComponent<Rigidbody>();
        body->drag = 1;
        body->velocity = {60, 0};
        game.LoadScene(scene);
        game.Step();
        if (std::fabs(ball->transform->position.x - 1) > 0.001f)
            throw std::runtime_error("Installed library fixed-step check failed");
        game.Shutdown();

        auto savePath = std::filesystem::path("consumer-save.tmp");
        SaveSystem saved, loaded;
        saved.SetInt("score", 42);
        bool saveOk = saved.Save(savePath) && loaded.Load(savePath) && loaded.GetInt("score") == 42;
        std::filesystem::remove(savePath);
        std::filesystem::remove(savePath.string() + ".bak");
        if (!saveOk) throw std::runtime_error("Installed library storage check failed");

        if (graphics) {
            SetConfigFlags(FLAG_WINDOW_HIDDEN);
            game.Run();
            if (game.renderedFrames != 4 || game.texture || IsWindowReady() || IsAudioDeviceReady())
                throw std::runtime_error("Installed SDK graphics/cleanup check failed");
        }
        std::printf("Installed RayGame SDK consumer: passed%s\n", graphics ? " (graphics)" : "");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "SDK consumer failed: %s\n", error.what());
        return 1;
    }
}
