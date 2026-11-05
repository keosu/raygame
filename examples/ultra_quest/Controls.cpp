#include "Controls.h"
#include "Assets.h"
#include "Draw.h"
#include "Gui.h"

namespace UQ {

namespace {
const char* Label(Action a, bool explore) {
    switch (a) {
    case Action::Attack: return explore ? "调查" : "攻击";
    case Action::Jump: return "跳跃";
    case Action::Guard: return "防御";
    case Action::Dash: return "冲刺";
    case Action::Beam: return "光线";
    default: return "";
    }
}

bool Key(std::initializer_list<int> keys, bool edge) {
    for (int k : keys)
        if (edge ? Input::GetKeyDown(k) : Input::GetKey(k)) return true;
    return false;
}

bool Button(std::initializer_list<int> buttons, bool edge) {
    if (!Input::GamepadAvailable()) return false;
    for (int b : buttons)
        if (edge ? Input::GetGamepadButtonDown(b) : Input::GetGamepadButton(b)) return true;
    return false;
}
} // namespace

std::vector<Controls::Pad> Controls::Pads(Layout layout) {
    std::vector<Pad> pads = {{Action::Pause, {1234, 140}, 28}}; // below the kaiju panel
    if (layout == Layout::None) return pads;
    pads.push_back({Action::Left, {92, 620}, 62});
    pads.push_back({Action::Right, {240, 620}, 62});
    pads.push_back({Action::Attack, {1158, 612}, 72});
    if (layout == Layout::Fight) {
        pads.push_back({Action::Jump, {1018, 660}, 54});
        pads.push_back({Action::Guard, {1030, 528}, 50});
        pads.push_back({Action::Dash, {1166, 470}, 48});
        pads.push_back({Action::Beam, {888, 652}, 48});
    }
    return pads;
}

void Controls::Reset() {
    held = {};
    pressed = {};
    touchHeld = {};
    confirm = false;
    tracker.Reset(Input::GetFrame());
}

void Controls::Update(Layout layout) {
    tracker.Update(Input::GetFrame(), Display::GetViewport());
    std::array<bool, (int)Action::Count> nowTouch{}, edgeTouch{};
    auto pads = Pads(layout);
    for (const auto& contact : tracker.Contacts()) {
        if (!contact.Active()) continue;
        lastTouch = RealTime();
        // Movement follows the finger across both arrows; the other buttons stay where pressed.
        if (contact.start.x < 330 && contact.start.y > 500 && layout != Layout::None) {
            Action a = contact.position.x < 166 ? Action::Left : Action::Right;
            nowTouch[(int)a] = true;
            if (contact.pressed) edgeTouch[(int)a] = true;
            continue;
        }
        for (const auto& pad : pads) {
            if (pad.action == Action::Left || pad.action == Action::Right) continue;
            if (Len(Sub(contact.start, pad.c)) <= pad.r * 1.2f) {
                nowTouch[(int)pad.action] = true;
                if (contact.pressed) edgeTouch[(int)pad.action] = true;
            }
        }
    }
    bool touch = TouchVisible();
    float stick = Input::GamepadAvailable() ? Input::GetGamepadAxis(GAMEPAD_AXIS_LEFT_X) : 0;
    auto set = [&](Action a, bool down, bool edge) {
        bool t = touch && nowTouch[(int)a];
        held[(int)a] = down || t;
        pressed[(int)a] = edge || (touch && edgeTouch[(int)a]);
    };
    set(Action::Left, Key({KEY_A, KEY_LEFT}, false) || Button({GAMEPAD_BUTTON_LEFT_FACE_LEFT}, false) || stick < -0.4f,
        Key({KEY_A, KEY_LEFT}, true) || Button({GAMEPAD_BUTTON_LEFT_FACE_LEFT}, true));
    set(Action::Right, Key({KEY_D, KEY_RIGHT}, false) || Button({GAMEPAD_BUTTON_LEFT_FACE_RIGHT}, false) || stick > 0.4f,
        Key({KEY_D, KEY_RIGHT}, true) || Button({GAMEPAD_BUTTON_LEFT_FACE_RIGHT}, true));
    set(Action::Attack, Key({KEY_J, KEY_Z}, false) || Button({GAMEPAD_BUTTON_RIGHT_FACE_LEFT}, false),
        Key({KEY_J, KEY_Z}, true) || Button({GAMEPAD_BUTTON_RIGHT_FACE_LEFT}, true));
    set(Action::Jump, Key({KEY_K, KEY_W, KEY_UP, KEY_SPACE}, false) || Button({GAMEPAD_BUTTON_RIGHT_FACE_DOWN}, false),
        Key({KEY_K, KEY_W, KEY_UP, KEY_SPACE}, true) || Button({GAMEPAD_BUTTON_RIGHT_FACE_DOWN}, true));
    set(Action::Guard, Key({KEY_L, KEY_S, KEY_DOWN, KEY_C}, false) ||
                           Button({GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_1}, false),
        Key({KEY_L, KEY_S, KEY_DOWN, KEY_C}, true) ||
            Button({GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_1}, true));
    set(Action::Dash, Key({KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_I, KEY_X}, false) || Button({GAMEPAD_BUTTON_RIGHT_FACE_RIGHT}, false),
        Key({KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_I, KEY_X}, true) || Button({GAMEPAD_BUTTON_RIGHT_FACE_RIGHT}, true));
    set(Action::Beam, Key({KEY_U, KEY_V}, false) || Button({GAMEPAD_BUTTON_RIGHT_FACE_UP}, false),
        Key({KEY_U, KEY_V}, true) || Button({GAMEPAD_BUTTON_RIGHT_FACE_UP}, true));
    set(Action::Pause, Key({KEY_ESCAPE, KEY_P}, false) || Button({GAMEPAD_BUTTON_MIDDLE_RIGHT}, false),
        Key({KEY_ESCAPE, KEY_P}, true) || Button({GAMEPAD_BUTTON_MIDDLE_RIGHT}, true));
    bool tapped = false;
    for (const auto& r : tracker.Released())
        if (r.id >= 0 && Len(Sub(r.position, r.start)) < 30) tapped = true;
    confirm = Key({KEY_ENTER, KEY_KP_ENTER, KEY_SPACE, KEY_J, KEY_Z, KEY_E}, true) ||
              Button({GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_RIGHT_FACE_LEFT}, true) ||
              (Input::GetMouseButtonDown(MOUSE_BUTTON_LEFT) && Input::GetTouchCount() == 0) || tapped;
    if (layout == Layout::Explore) {
        if (Key({KEY_E, KEY_ENTER}, true)) pressed[(int)Action::Attack] = true;
    }
}

float Controls::MoveX() const {
    float x = 0;
    if (held[(int)Action::Left]) x -= 1;
    if (held[(int)Action::Right]) x += 1;
    if (x == 0 && Input::GamepadAvailable()) {
        float stick = Input::GetGamepadAxis(GAMEPAD_AXIS_LEFT_X);
        if (std::fabs(stick) > 0.2f) x = stick;
    }
    return Clamp(x, -1.0f, 1.0f);
}

bool Controls::TouchVisible() const {
#if defined(__ANDROID__)
    return true;
#else
    return Progress::Options().touchControls || RealTime() - lastTouch < 20.0f;
#endif
}

void Controls::Draw(Layout layout, bool beamReady, bool interactReady) const {
    if (!TouchVisible()) return;
    bool explore = layout == Layout::Explore;
    for (const auto& pad : Pads(layout)) {
        bool down = held[(int)pad.action];
        float alpha = down ? 0.75f : 0.42f;
        Color rim = Palette::cyan;
        if (pad.action == Action::Beam) rim = beamReady ? Palette::gold : Palette::muted;
        if (pad.action == Action::Attack && explore) rim = interactReady ? Palette::gold : Palette::muted;
        if (pad.action == Action::Pause) {
            DrawCircleV(pad.c, pad.r, WithAlpha(Palette::ink, 0.55f));
            DrawRing(pad.c, pad.r - 2, pad.r, 0, 360, 32, WithAlpha(rim, 0.6f));
            Gui::PauseIcon(pad.c, pad.r * 0.45f, WithAlpha(Palette::text, 0.9f));
            continue;
        }
        DrawCircleV(pad.c, pad.r, WithAlpha(Palette::ink, alpha * 0.8f));
        DrawRing(pad.c, pad.r - 3, pad.r, 0, 360, 40, WithAlpha(rim, alpha + 0.2f));
        if (down) DrawCircleV(pad.c, pad.r - 3, WithAlpha(rim, 0.25f));
        if (pad.action == Action::Left || pad.action == Action::Right) {
            float s = pad.action == Action::Left ? -1.0f : 1.0f;
            Draw::Tri({pad.c.x + s * 22, pad.c.y}, {pad.c.x - s * 14, pad.c.y - 22}, {pad.c.x - s * 14, pad.c.y + 22},
                      WithAlpha(Palette::text, 0.85f));
        } else {
            Gui::Text(Label(pad.action, explore), pad.c.x, pad.c.y - pad.r * 0.28f, pad.r * 0.52f,
                      WithAlpha(Palette::text, 0.92f), Gui::Align::Center, true);
        }
    }
}

} // namespace UQ
