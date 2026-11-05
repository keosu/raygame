#pragma once

#include "Controls.h"
#include "Fx.h"
#include "Story.h"

namespace UQ {

// Haruki investigates the tremors on foot: walk the street, talk to people, collect three clues,
// then reach the construction site where Red King breaks out.
class ExploreScene : public Scene {
public:
    void OnLoad() override;
    void Update() override;
    void Render() override;

    // Capture tour hooks.
    void DebugSkipDialogue() { dialogue.Skip(); }
    void DebugWalkTo(float x) { player = x; }
    void DebugInteract(const std::string& id);
    void DebugFindAll();
    void DebugReveal();

private:
    enum class Mark { Clue, Talk, Look };
    struct Spot {
        std::string id;
        float x;
        std::string look; // a person's Look key, empty for objects
        Mark mark;
        int clue = -1;    // index into kClues
        bool done = false;
        int facing = -1;
        float react = 0;
    };
    enum class Phase { Walk, Reveal, Rise, Finish };

    Controls controls;
    Dialogue dialogue;
    Fx fx;
    std::vector<Spot> spots;
    std::array<bool, 3> clues{};
    float player = 220;
    int facing = 1;
    float walk = 0;
    float camX = 0;
    float time = 0;
    float tremorAt = 9;
    float tremor = 0;
    float shake = 0;
    float toastTime = 99;
    std::string toast;
    bool cluesAnnounced = false;
    bool siteOpen = false;
    bool gateWarned = false;
    Phase phase = Phase::Walk;
    float phaseTime = 0;
    bool paused = false;
    std::string pendingClue; // the clue a dialogue grants when it closes
    int pendingIndex = -1;

    int ClueCount() const;
    Spot* Nearby();
    void Interact(Spot& s);
    void Talk(const std::string& block);
    void DrawSky() const;
    void DrawStreet() const;
    void DrawSite() const;
    void DrawSpots() const;
    void DrawHud() const;
    void DrawPause();
    float ScreenX(float x) const { return x - camX; }
};

} // namespace UQ
