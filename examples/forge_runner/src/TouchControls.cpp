#include "TouchControls.h"

namespace Forge {
namespace {
bool Inside(Vector2 point, Rectangle bounds) {
    return CheckCollisionPointRec(point, bounds);
}
} // namespace
Rectangle TouchControls::Bounds(Rectangle reference) const {
    float scale = std::clamp(sizeScale, 1.0f, 1.4f);
    auto r = reference;
    r.x = reference.x > 640 ? 1280 - (1280 - reference.x) * scale : reference.x * scale;
    r.y = reference.y > 360 ? 720 - (720 - reference.y) * scale : reference.y;
    r.width *= scale;
    r.height *= scale;
    return layout.Place(r, {reference.x > 640 ? 1.0f : 0.0f, reference.y > 360 ? 1.0f : 0.0f});
}
void TouchControls::Reset(const RayGame::InputFrame& frame) {
    tracker.Reset(frame);
    contacts = {};
    held = {};
    pendingJump = pendingDash = pendingDrop = pausePressed = dashHeld = false;
    pointerPressed = pointerHeld = pointerReleased = false;
}
void TouchControls::Update(const RayGame::InputFrame& frame,
                           const RayGame::CanvasViewport& viewport, bool playing) {
    layout.content = {0, 0, viewport.logicalSize.x, viewport.logicalSize.y};
    tracker.Update(frame, viewport);
    pointerPressed = pointerReleased = pausePressed = false;
    if (tracker.Cancelled()) {
        Reset(frame);
        return;
    }
    for (const auto& released : tracker.Released()) {
        if (!released.Active())
            continue;
        for (auto& contact : contacts) {
            if (contact.id != released.id)
                continue;
            if (contact.role == Role::Menu) {
                pointerStart = released.start;
                pointerPosition = released.position;
                pointerReleased = true;
            }
            contact = {};
        }
    }
    for (auto& contact : contacts) {
        if (contact.id >= 0 && !tracker.IsActive(contact.id))
            contact = {};
    }
    for (const auto& point : tracker.Contacts()) {
        if (!point.Active())
            continue;
        auto found = std::find_if(contacts.begin(), contacts.end(),
                                  [&](const Contact& c) { return c.id == point.id; });
        if (found != contacts.end()) {
            found->position = point.position;
            continue;
        }
        if (!point.pressed)
            continue;
        auto free = std::find_if(contacts.begin(), contacts.end(),
                                 [](const Contact& c) { return c.id < 0; });
        if (free == contacts.end())
            break;
        *free = {point.id, Role::Blocked, point.start, point.position};
        if (playing) {
            if (Inside(point.position, Bounds(Left)) || Inside(point.position, Bounds(Right)))
                free->role = Role::Move;
            else if (Inside(point.position, Bounds(JumpButton))) {
                free->role = Role::Jump;
                pendingJump = true;
            } else if (Inside(point.position, Bounds(DashButton))) {
                free->role = Role::Dash;
                pendingDash = true;
            } else if (showDown && Inside(point.position, Bounds(Down))) {
                free->role = Role::Down;
            } else if (Inside(point.position, Bounds(PauseButton))) {
                free->role = Role::Pause;
                pausePressed = true;
            }
        } else if (!pointerReleased &&
                   std::none_of(contacts.begin(), contacts.end(),
                                [](const Contact& c) { return c.role == Role::Menu; })) {
            free->role = Role::Menu;
            pointerPressed = true;
        }
    }
    held = {};
    pointerHeld = dashHeld = false;
    for (const auto& contact : contacts) {
        if (contact.id < 0)
            continue;
        if (contact.role == Role::Move) {
            if (Inside(contact.position, Bounds(Left)))
                held.move -= 1;
            if (Inside(contact.position, Bounds(Right)))
                held.move += 1;
            // Pull the movement finger down, then tap jump to drop through a ledge.
            if (contact.position.y - contact.start.y > 48)
                held.down = true;
        }
        if (contact.role == Role::Jump)
            held.jumpHeld = true;
        if (contact.role == Role::Dash)
            dashHeld = true;
        if (contact.role == Role::Down)
            held.down = true;
        if (contact.role == Role::Menu) {
            pointerHeld = true;
            pointerStart = contact.start;
            pointerPosition = contact.position;
        }
    }
    held.move = std::clamp(held.move, -1.0f, 1.0f);
    if (pendingJump && held.down)
        pendingDrop = true;
}
Command TouchControls::Consume() {
    auto command = held;
    command.jump = pendingJump;
    command.dash = pendingDash;
    command.down |= pendingDrop;
    pendingJump = pendingDash = pendingDrop = false;
    return command;
}
bool TouchControls::Click(Rectangle bounds) {
    if (!pointerReleased || !Inside(pointerStart, bounds) || !Inside(pointerPosition, bounds))
        return false;
    pointerReleased = false;
    return true;
}
} // namespace Forge
