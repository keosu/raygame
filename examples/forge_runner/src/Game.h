#pragma once
#include "Presentation.h"

namespace Forge {
enum class Page { Home, Play, Pause, Settings, Debrief, Credits, ConfirmNew };
class Stage : public RayGame::Scene {
public:
    std::shared_ptr<World> world;
    RayGame::Ref<RayGame::GameObject> actor;
    bool* playing;
    Stage(std::shared_ptr<World> value, bool* enabled);
    void OnLoad() override;
};
class Game : public RayGame::GameEngine {
public:
    Game(std::string assets, std::filesystem::path data, bool smoke = false);
    ~Game() override;
    int failures = 0;
    std::string assets;
    Profile profile;
    Presentation art;
    std::shared_ptr<World> world;
    RayGame::Ref<Stage> stage;
    Page page = Page::Home, settingsReturn = Page::Home;
    bool playing = false, smoke = false;
    int focus = 0, frame = 0;
    float toastTime = 0, pageTime = 0;
    std::string toast;
    void StartLevel(int index, bool resume);
    void Switch(Page next);
    void Persist();
protected:
    void OnInit() override;
    void OnUnscaledUpdate() override;
    void OnUpdate() override;
    void OnRender() override;
private:
    bool activate = false, mousePress = false;
    Vector2 pressedAt{}, lastMouse{};
    int settingsDirection = 0;
    bool savedExit = false;
    float pausedWorldTime = 0;
    float smokeMusic = 0;
    bool smokeShake = true;
    float smokeElapsed = 0;
    std::uint64_t smokeCollected = 0;
    void Audio(unsigned events);
    bool Button(int index, Rectangle rect, const std::string& label, const std::string& detail = "", bool disabled = false);
    void DrawHome();
    void DrawHUD();
    void DrawModal();
    void DrawSettings();
    void DrawDebrief();
    void ApplySettings();
    void SmokeStep();
    void SmokeInput();
    void Capture(const std::string& name);
    void Expect(bool condition, const std::string& message);
};
}
