#include "TouchControls.h"

namespace Forge {
namespace {
bool Inside(Vector2 point, Rectangle bounds) { return CheckCollisionPointRec(point, bounds); }
bool Valid(const RayGame::InputTouch& touch) {
    return touch.id >= 0 && std::isfinite(touch.position.x) && std::isfinite(touch.position.y);
}
}
void TouchControls::Reset(const RayGame::InputFrame& frame) {
    contacts = {}; held = {};
    pendingJump = pendingDash = pendingDrop = pausePressed = dashHeld = false;
    pointerPressed = pointerHeld = pointerReleased = false;
    // Existing fingers must lift before they can act on the next page.
    for (int i = 0; i < std::clamp(frame.touchCount, 0, 8); ++i)
        if (Valid(frame.touches[i])) contacts[i].id = frame.touches[i].id;
}
void TouchControls::Update(const RayGame::InputFrame& frame, const RayGame::CanvasViewport& viewport, bool playing) {
    pointerPressed = pointerReleased = pausePressed = false;
    auto bounds = viewport.bounds;
    bool resized = hasViewport && (bounds.x != previousViewport.x || bounds.y != previousViewport.y ||
                                  bounds.width != previousViewport.width || bounds.height != previousViewport.height);
    previousViewport = bounds; hasViewport = true;
    if (!frame.focused) { Reset(frame); refocusing = true; return; }
    if (refocusing || resized) { Reset(frame); refocusing = false; return; }
    int count = std::clamp(frame.touchCount, 0, 8);
    for (auto& contact : contacts) {
        if (contact.id < 0) continue;
        const RayGame::InputTouch* point = nullptr;
        for (int i = 0; i < count; ++i)
            if (Valid(frame.touches[i]) && frame.touches[i].id == contact.id) { point = &frame.touches[i]; break; }
        if (point) contact.position = viewport.ToCanvas(point->position);
        else {
            if (contact.role == Role::Menu) {
                pointerStart = contact.start; pointerPosition = contact.position; pointerReleased = true;
            }
            contact = {};
        }
    }
    for (int i = 0; i < count; ++i) {
        auto point = frame.touches[i];
        if (!Valid(point)) continue;
        bool known = std::any_of(contacts.begin(), contacts.end(), [&](const Contact& c) { return c.id == point.id; });
        if (known) continue;
        auto free = std::find_if(contacts.begin(), contacts.end(), [](const Contact& c) { return c.id < 0; });
        if (free == contacts.end()) break;
        *free = {point.id, Role::Blocked, viewport.ToCanvas(point.position), viewport.ToCanvas(point.position)};
        if (!viewport.Contains(point.position)) continue;
        if (playing) {
            if (Inside(free->position, Left) || Inside(free->position, Right)) free->role = Role::Move;
            else if (Inside(free->position, JumpButton)) { free->role = Role::Jump; pendingJump = true; }
            else if (Inside(free->position, DashButton)) { free->role = Role::Dash; pendingDash = true; }
            else if (Inside(free->position, Down)) free->role = Role::Down;
            else if (Inside(free->position, PauseButton)) { free->role = Role::Pause; pausePressed = true; }
        } else if (!pointerReleased && std::none_of(contacts.begin(), contacts.end(), [](const Contact& c) { return c.role == Role::Menu; })) {
            free->role = Role::Menu; pointerPressed = true;
        }
    }
    held = {}; pointerHeld = dashHeld = false;
    for (const auto& contact : contacts) {
        if (contact.id < 0) continue;
        if (contact.role == Role::Move) {
            if (Inside(contact.position, Left)) held.move -= 1;
            if (Inside(contact.position, Right)) held.move += 1;
        }
        if (contact.role == Role::Jump) held.jumpHeld = true;
        if (contact.role == Role::Dash) dashHeld = true;
        if (contact.role == Role::Down) held.down = true;
        if (contact.role == Role::Menu) {
            pointerHeld = true; pointerStart = contact.start; pointerPosition = contact.position;
        }
    }
    held.move = std::clamp(held.move, -1.0f, 1.0f);
    if(pendingJump && held.down) pendingDrop = true;
}
Command TouchControls::Consume() {
    auto command = held;
    command.jump = pendingJump; command.dash = pendingDash; command.down |= pendingDrop;
    pendingJump = pendingDash = pendingDrop = false;
    return command;
}
bool TouchControls::Click(Rectangle bounds) {
    if (!pointerReleased || !Inside(pointerStart, bounds) || !Inside(pointerPosition, bounds)) return false;
    pointerReleased = false;
    return true;
}
}
