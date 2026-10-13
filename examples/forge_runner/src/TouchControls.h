#pragma once
#include "World.h"
#include <raygame/Input.h>
#include <raygame/CanvasLayout.h>
#include <raygame/TouchTracker.h>

namespace Forge {
// Contacts keep their role by ID, not by the order reported by Android.
// Render-frame presses are buffered until the next fixed simulation step.
class TouchControls {
public:
    static constexpr Rectangle Left{32, 508, 132, 136}, Right{176, 508, 132, 136};
    static constexpr Rectangle Down{324, 556, 88, 88}, JumpButton{1080, 490, 152, 158};
    static constexpr Rectangle DashButton{904, 522, 152, 126}, PauseButton{1144, 92, 88, 72};
    float sizeScale = 1.0f;
    bool showDown = false;
    Rectangle Bounds(Rectangle reference) const;
    void Update(const RayGame::InputFrame& frame, const RayGame::CanvasViewport& viewport,
                bool playing);
    void Reset(const RayGame::InputFrame& frame);
    Command Consume();
    const Command& Held() const {
        return held;
    }
    bool Click(Rectangle bounds);
    bool pointerPressed = false, pointerHeld = false, pointerReleased = false;
    bool pausePressed = false, dashHeld = false;
    Vector2 pointerStart{}, pointerPosition{};

private:
    enum class Role { Blocked, Move, Jump, Dash, Down, Pause, Menu };
    struct Contact {
        int id = -1;
        Role role = Role::Blocked;
        Vector2 start{}, position{};
    };
    std::array<Contact, 8> contacts;
    RayGame::TouchTracker tracker;
    RayGame::CanvasLayout layout;
    Command held;
    bool pendingJump = false, pendingDash = false, pendingDrop = false;
};
} // namespace Forge
