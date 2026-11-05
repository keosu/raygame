#pragma once

#include "Common.h"

namespace UQ {

enum class Action { Left, Right, Attack, Jump, Guard, Dash, Beam, Pause, Count };

// Keyboard, gamepad and on-screen buttons merged into one set of actions. Call Update once per
// rendered frame; Pressed() is true for that frame only, so fixed-step users latch it.
class Controls {
public:
    enum class Layout { None, Explore, Fight };

    void Update(Layout layout);
    bool Held(Action a) const { return held[(int)a]; }
    bool Pressed(Action a) const { return pressed[(int)a]; }
    float MoveX() const;
    // True when a confirm-style press happened: click, tap, Enter, Space, J/Z/E or pad A.
    bool Confirm() const { return confirm; }
    bool TouchVisible() const;
    void Draw(Layout layout, bool beamReady, bool interactReady = false) const;
    void Reset();

private:
    struct Pad {
        Action action;
        Vector2 c;
        float r;
    };
    static std::vector<Pad> Pads(Layout layout);
    TouchTracker tracker;
    std::array<bool, (int)Action::Count> held{}, pressed{}, touchHeld{};
    bool confirm = false;
    float lastTouch = -100;
};

} // namespace UQ
