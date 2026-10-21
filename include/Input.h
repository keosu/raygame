#pragma once

#include "Core.h"
#include "Display.h"
#include <array>
#include <functional>

namespace RayGame {

template<size_t N> struct InputButtons {
    std::array<bool, N> held{}, pressed{}, released{};
};
struct InputGamepad {
    bool available = false;
    InputButtons<32> buttons;
    std::array<float, 8> axes{};
};
struct InputTouch {
    int id = -1;
    Vector2 position{};
};
struct InputFrame {
    InputButtons<512> keys;
    InputButtons<8> mouse;
    std::array<InputGamepad, 4> gamepads;
    std::array<InputTouch, 8> touches;
    int touchCount = 0;
    Vector2 mousePosition{}, mouseDelta{}; // Raw window coordinates.
    float mouseWheel = 0;
    bool focused = true;
};

class RAYGAME_API Input {
public:
    static void CaptureFrame();                // polls the devices, then commits like the overload
    static void CaptureFrame(InputFrame raw);  // passes a device frame through the capture filter
    // Edits every captured frame before it is committed, e.g. to layer scripted input over the
    // devices. SetFrame bypasses it. One filter at a time; an empty function removes it.
    static void SetCaptureFilter(std::function<void(InputFrame&)> filter) { captureFilter = std::move(filter); }
    static void SetFrame(const InputFrame& value);
    static const InputFrame& GetFrame() { return inFixedTick ? fixedFrame : frame; }
    static void Reset(); // Preserves action bindings and configured deadzone.
    static void DiscardPending();
    class FixedTickScope {
    public:
        FixedTickScope();
        ~FixedTickScope();
        FixedTickScope(const FixedTickScope&) = delete;
        FixedTickScope& operator=(const FixedTickScope&) = delete;
    };
    // ---- Keyboard ----
    static bool GetKey(int key) { return Read(GetFrame().keys.held, key); }
    static bool GetKeyDown(int key) { return Read(GetFrame().keys.pressed, key); }
    static bool GetKeyUp(int key) { return Read(GetFrame().keys.released, key); }

    // ---- Mouse ----
    static bool GetMouseButton(int button) { return Read(GetFrame().mouse.held, button); }
    static bool GetMouseButtonDown(int button) { return Read(GetFrame().mouse.pressed, button); }
    static bool GetMouseButtonUp(int button) { return Read(GetFrame().mouse.released, button); }
    static Vector2 GetMousePosition() { return Display::ScreenToCanvas(GetFrame().mousePosition); }
    static Vector2 GetMouseDelta() {
        Vector2 delta = GetFrame().mouseDelta;
        float scale = Display::GetViewport().scale;
        return {delta.x / scale, delta.y / scale};
    }
    static bool IsMouseInCanvas() { return Display::GetViewport().Contains(GetFrame().mousePosition); }
    static float GetMouseWheelMove() { return GetFrame().mouseWheel; }
    static int GetTouchCount() { return GetFrame().touchCount; }
    static int GetTouchId(int index) {
        return index >= 0 && index < GetTouchCount() ? GetFrame().touches[index].id : -1;
    }
    static Vector2 GetTouchPosition(int index) {
        return index >= 0 && index < GetTouchCount()
            ? Display::ScreenToCanvas(GetFrame().touches[index].position) : Vector2{-1, -1};
    }

    // ---- Gamepad ----
    static bool GamepadAvailable(int pad = 0) { return pad >= 0 && pad < 4 && GetFrame().gamepads[pad].available; }
    static bool GetGamepadButton(int button, int pad = 0) { return GamepadAvailable(pad) && Read(GetFrame().gamepads[pad].buttons.held, button); }
    static bool GetGamepadButtonDown(int button, int pad = 0) { return GamepadAvailable(pad) && Read(GetFrame().gamepads[pad].buttons.pressed, button); }
    static bool GetGamepadButtonUp(int button, int pad = 0) { return pad >= 0 && pad < 4 && Read(GetFrame().gamepads[pad].buttons.released, button); }
    static float GetGamepadAxis(int axis, int pad = 0) {
        if (!GamepadAvailable(pad) || axis < 0 || axis >= 8) return 0;
        float v = GetFrame().gamepads[pad].axes[axis];
        return (v > -deadzone && v < deadzone) ? 0.0f : v;
    }
    static void SetGamepadDeadzone(float dz) { if (std::isfinite(dz)) deadzone = Clamp(dz, 0.0f, 1.0f); }

    // ---- Axis (keyboard + gamepad) ----
    static float GetAxis(const std::string& axis) {
        if (axis == "Horizontal") {
            float h = 0;
            if (GetKey(KEY_A) || GetKey(KEY_LEFT)) h -= 1;
            if (GetKey(KEY_D) || GetKey(KEY_RIGHT)) h += 1;
            if (h == 0 && GamepadAvailable()) h = GetGamepadAxis(GAMEPAD_AXIS_LEFT_X);
            return Clamp(h, -1.0f, 1.0f);
        } else if (axis == "Vertical") {
            float v = 0;
            if (GetKey(KEY_W) || GetKey(KEY_UP)) v -= 1;
            if (GetKey(KEY_S) || GetKey(KEY_DOWN)) v += 1;
            if (v == 0 && GamepadAvailable()) v = GetGamepadAxis(GAMEPAD_AXIS_LEFT_Y);
            return Clamp(v, -1.0f, 1.0f);
        }
        return 0;
    }

    static Vector2 GetMovementInput() {
        Vector2 movement = {GetAxis("Horizontal"), GetAxis("Vertical")};
        return movement.x * movement.x + movement.y * movement.y > 1 ? Normalize(movement) : movement;
    }

    // ---- Action mapping ----
    // Bind a named action to one or more keyboard keys / gamepad buttons.
    static void BindAction(const std::string& action, std::vector<int> keys,
                           std::vector<int> gamepadButtons = {}) {
        actions[action] = {std::move(keys), std::move(gamepadButtons)};
    }

    static bool GetAction(const std::string& action) {
        auto it = actions.find(action);
        if (it == actions.end()) return false;
        for (int k : it->second.keys) if (GetKey(k)) return true;
        if (GamepadAvailable())
            for (int b : it->second.buttons) if (GetGamepadButton(b)) return true;
        return false;
    }

    static bool GetActionDown(const std::string& action) {
        auto it = actions.find(action);
        if (it == actions.end()) return false;
        for (int k : it->second.keys) if (GetKeyDown(k)) return true;
        if (GamepadAvailable())
            for (int b : it->second.buttons) if (GetGamepadButtonDown(b)) return true;
        return false;
    }

private:
    template<size_t N> static bool Read(const std::array<bool, N>& values, int index) {
        return index >= 0 && (size_t)index < N && values[index];
    }
    static InputFrame frame, pending, fixedFrame;
    static bool inFixedTick;
    struct ActionBinding { std::vector<int> keys; std::vector<int> buttons; };
    static inline std::unordered_map<std::string, ActionBinding> actions;
    static inline float deadzone = 0.2f;
    static inline std::function<void(InputFrame&)> captureFilter;
};

}
