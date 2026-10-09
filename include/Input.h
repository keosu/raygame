#pragma once

#include "Core.h"

namespace RayGame {

class RAYGAME_API Input {
public:
    // ---- Keyboard ----
    static bool GetKey(int key) { return IsKeyDown(key); }
    static bool GetKeyDown(int key) { return IsKeyPressed(key); }
    static bool GetKeyUp(int key) { return IsKeyReleased(key); }

    // ---- Mouse ----
    static bool GetMouseButton(int button) { return IsMouseButtonDown(button); }
    static bool GetMouseButtonDown(int button) { return IsMouseButtonPressed(button); }
    static bool GetMouseButtonUp(int button) { return IsMouseButtonReleased(button); }
    static Vector2 GetMousePosition() { return ::GetMousePosition(); }
    static Vector2 GetMouseDelta() { return ::GetMouseDelta(); }
    static float GetMouseWheelMove() { return ::GetMouseWheelMove(); }

    // ---- Gamepad ----
    static bool GamepadAvailable(int pad = 0) { return IsGamepadAvailable(pad); }
    static bool GetGamepadButton(int button, int pad = 0) { return IsGamepadButtonDown(pad, button); }
    static bool GetGamepadButtonDown(int button, int pad = 0) { return IsGamepadButtonPressed(pad, button); }
    static bool GetGamepadButtonUp(int button, int pad = 0) { return IsGamepadButtonReleased(pad, button); }
    static float GetGamepadAxis(int axis, int pad = 0) {
        float v = GetGamepadAxisMovement(pad, axis);
        return (v > -deadzone && v < deadzone) ? 0.0f : v;
    }
    static void SetGamepadDeadzone(float dz) { deadzone = dz; }

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
    struct ActionBinding { std::vector<int> keys; std::vector<int> buttons; };
    static inline std::unordered_map<std::string, ActionBinding> actions;
    static inline float deadzone = 0.2f;
};

}
