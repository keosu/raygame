#include "Input.h"
#include <stdexcept>

namespace RayGame {
InputFrame Input::frame, Input::pending, Input::fixedFrame;
bool Input::inFixedTick = false;
namespace {
template<size_t N> void ClearEdges(InputButtons<N>& value) { value.pressed.fill(false); value.released.fill(false); }
void ClearEdges(InputFrame& value) {
    ClearEdges(value.keys); ClearEdges(value.mouse);
    for (auto& pad : value.gamepads) ClearEdges(pad.buttons);
    value.mouseDelta = {}; value.mouseWheel = 0;
}
template<size_t N> void InferEdges(InputButtons<N>& value, const InputButtons<N>& previous) {
    for (size_t i = 0; i < N; ++i) {
        value.pressed[i] = value.pressed[i] || (value.held[i] && !previous.held[i]);
        value.released[i] = value.released[i] || (!value.held[i] && previous.held[i]);
    }
}
template<size_t N> void Accumulate(InputButtons<N>& pending, const InputButtons<N>& value) {
    pending.held = value.held;
    for (size_t i = 0; i < N; ++i) {
        pending.pressed[i] = pending.pressed[i] || value.pressed[i];
        pending.released[i] = pending.released[i] || value.released[i];
    }
}
}
void Input::CaptureFrame() {
    InputFrame next;
    next.focused = IsWindowFocused();
    next.mousePosition = ::GetMousePosition();
    next.mouseDelta = ::GetMouseDelta();
    next.mouseWheel = ::GetMouseWheelMove();
    if (next.focused) {
        next.touchCount = std::min(8, ::GetTouchPointCount());
        for (int i = 0; i < next.touchCount; ++i)
            next.touches[i] = {::GetTouchPointId(i), ::GetTouchPosition(i)};
        for (int i = 1; i < 512; ++i) {
            next.keys.held[i] = IsKeyDown(i);
            next.keys.pressed[i] = IsKeyPressed(i);
            next.keys.released[i] = IsKeyReleased(i);
        }
        for (int i = 0; i < 8; ++i) {
            next.mouse.held[i] = IsMouseButtonDown(i);
            next.mouse.pressed[i] = IsMouseButtonPressed(i);
            next.mouse.released[i] = IsMouseButtonReleased(i);
        }
        for (int pad = 0; pad < 4; ++pad) {
            auto& device = next.gamepads[pad];
            device.available = IsGamepadAvailable(pad);
            if (!device.available) continue;
            for (int i = 0; i < 32; ++i) {
                device.buttons.held[i] = IsGamepadButtonDown(pad, i);
                device.buttons.pressed[i] = IsGamepadButtonPressed(pad, i);
                device.buttons.released[i] = IsGamepadButtonReleased(pad, i);
            }
            for (int i = 0; i < std::min(8, GetGamepadAxisCount(pad)); ++i)
                device.axes[i] = GetGamepadAxisMovement(pad, i);
        }
    }
    SetFrame(next);
}
void Input::SetFrame(const InputFrame& value) {
    if (inFixedTick) throw std::logic_error("Cannot replace input inside a fixed tick");
    auto next = value;
    if (!next.focused) {
        next = {};
        next.focused = false;
        next.mousePosition = value.mousePosition;
        frame = next; pending = next;
        return;
    }
    InferEdges(next.keys, frame.keys);
    next.touchCount = std::max(0, std::min(8, next.touchCount));
    InferEdges(next.mouse, frame.mouse);
    for (size_t i = 0; i < next.gamepads.size(); ++i) {
        auto& pad = next.gamepads[i];
        if (!pad.available) pad = {};
        for (auto& axis : pad.axes) axis = std::isfinite(axis) ? Clamp(axis, -1.0f, 1.0f) : 0;
        InferEdges(pad.buttons, frame.gamepads[i].buttons);
    }
    if (!frame.focused) ClearEdges(next); // Regaining focus must not synthesize a jump/fire press.
    frame = next;
    pending.focused = next.focused;
    pending.mousePosition = next.mousePosition;
    pending.touchCount = next.touchCount;
    pending.touches = next.touches;
    pending.mouseDelta.x += next.mouseDelta.x; pending.mouseDelta.y += next.mouseDelta.y;
    pending.mouseWheel += next.mouseWheel;
    Accumulate(pending.keys, next.keys); Accumulate(pending.mouse, next.mouse);
    for (size_t i = 0; i < pending.gamepads.size(); ++i) {
        // Disconnect discards buffered presses from this device.
        if (!next.gamepads[i].available) pending.gamepads[i] = {};
        pending.gamepads[i].available = next.gamepads[i].available;
        pending.gamepads[i].axes = next.gamepads[i].axes;
        Accumulate(pending.gamepads[i].buttons, next.gamepads[i].buttons);
    }
}
void Input::Reset() { frame = {}; pending = {}; fixedFrame = {}; inFixedTick = false; }
void Input::DiscardPending() { pending = frame; ClearEdges(pending); }
Input::FixedTickScope::FixedTickScope() {
    if (inFixedTick) throw std::logic_error("Input fixed tick cannot be re-entered");
    fixedFrame = pending;
    ClearEdges(pending);
    inFixedTick = true;
}
Input::FixedTickScope::~FixedTickScope() { inFixedTick = false; }
}
