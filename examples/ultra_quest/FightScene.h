#pragma once

#include "Controls.h"
#include "Fight.h"
#include "Fx.h"
#include "Story.h"

namespace UQ {

class FightScene : public Scene {
public:
    explicit FightScene(bool story);
    void OnLoad() override;
    void OnUnload() override;
    void Update() override;
    void Render() override;

    // Capture tour hooks.
    Fight& Model() { return fight; }
    void DebugAutoplay(bool on, float skill = 1.0f) { autoplay = on; bot.skill = skill; }
    void DebugSkipIntro();
    void DebugPause(bool on) { paused = on; }
    void DebugInput(const FightInput& in) { latched = in; }
    void DebugSkipDialogue() { dialogue.Skip(); if (phase == Phase::Epilogue) SetPhase(Phase::Result); }
    int DebugPhase() const { return (int)phase; } // Intro, Fight, Finish, Epilogue, Result, Defeat
    float DebugPhaseTime() const { return phaseTime; }
    bool DebugExploded() const { return exploded; }
    float DebugRecoil() const { return kaijuRecoil; }

private:
    enum class Phase { Intro, Fight, Finish, Epilogue, Result, Defeat };
    struct Building {
        float x, w, h;
        int style;
        Color color;
        bool down = false;
        float fall = 0;
    };
    struct Ghost {
        float x, y;
        int facing;
        float rot, life;
        std::string pose;
    };

    bool story;
    Fight fight;
    FightBot bot;
    bool autoplay = false;
    Controls controls;
    FightInput latched;
    float accumulator = 0;
    Phase phase = Phase::Intro;
    float phaseTime = 0;
    float sceneTime = 0;
    bool paused = false;
    bool showHelp = false;
    Fx fx;
    float shake = 0;
    Vector2 shakeOffset{};
    float camX = kArenaW / 2, zoom = 0.9f, baseZoom = 0.9f; // zoom = baseZoom plus the hit kick
    // Hit feedback, presentation only.
    float kaijuRecoil = 0, recoilPower = 0;
    int recoilDir = 1;
    float heroRecoil = 0;
    float zoomKick = 0;
    float whiteFlash = 0, redFlash = 0;
    float shakeDir = 0;     // -1/1 biases the shake along the blow, 0 is all-round
    float lastKaijuX = 0;
    bool kaijuMoving = false;
    bool kaijuFell = false; // the dying fall has thrown up its dust
    Dialogue dialogue;
    float timerBeep = 0;
    float slowmo = 0;
    bool exploded = false;
    float explodedAt = 0;
    float hintTime = 0;
    std::vector<Building> buildings;
    std::vector<Ghost> ghosts;
    float ghostTimer = 0;
    int rank = -1;
    bool newBest = false;
    bool recorded = false;
    Rng rng{2020};

    void SetPhase(Phase p);
    void StepModel(float dt);
    void HandleEvents();
    void UpdateCamera(float dt);
    Vector2 ToScreen(Vector2 world) const;
    float Scale() const { return zoom; }

    void DrawBackground() const;
    void DrawBuildings() const;
    void DrawGround() const;
    void DrawPoseAt(const std::string& key, Vector2 feet, float worldPerPixel, int facing, float rotation,
                    Vector2 scale, Color tint, float flash) const;
    void DrawHero() const;
    void DrawKaiju() const;
    void DrawProjectiles() const;
    void DrawBeam() const;
    void DrawWarning() const;
    void DrawHud() const;
    void DrawBanner(const char* text, const char* sub, Color color, float t) const;
    void DrawPause();
    void DrawResult();
    void DrawDefeat();
    void CollapseNear(float x, float radius);
};

} // namespace UQ
