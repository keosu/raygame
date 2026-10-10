#pragma once
#include "World.h"
#include <raygame/Input.h>

namespace Forge {
// Contacts keep their role by ID, not by the order reported by Android.
// Render-frame presses are buffered until the next fixed simulation step.
class TouchControls {
public:
    static constexpr Rectangle Left{40, 540, 96, 96}, Right{156, 540, 96, 96};
    static constexpr Rectangle Down{280, 564, 80, 72}, JumpButton{1112, 516, 112, 120};
    static constexpr Rectangle DashButton{972, 548, 112, 88}, PauseButton{1152, 92, 72, 60};
    void Update(const RayGame::InputFrame& frame, const RayGame::CanvasViewport& viewport, bool playing);
    void Reset(const RayGame::InputFrame& frame);
    Command Consume();
    const Command& Held() const { return held; }
    bool Click(Rectangle bounds);
    bool pointerPressed = false, pointerHeld = false, pointerReleased = false;
    bool pausePressed = false, dashHeld = false;
    Vector2 pointerStart{}, pointerPosition{};
private:
    enum class Role { Blocked, Move, Jump, Dash, Down, Pause, Menu };
    struct Contact { int id = -1; Role role = Role::Blocked; Vector2 start{}, position{}; };
    std::array<Contact, 8> contacts;
    Command held;
    bool pendingJump = false, pendingDash = false, pendingDrop = false, refocusing = false, hasViewport = false;
    Rectangle previousViewport{};
};
}
