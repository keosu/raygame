#pragma once

#include "Input.h"

namespace RayGame {

struct TouchContact {
    int id = -1;
    Vector2 start{}, position{}; // Logical canvas coordinates.
    bool pressed = false;
    bool blocked = false;
    bool Active() const {
        return id >= 0 && !blocked;
    }
};

// Update once per rendered frame, then route contacts to game-specific controls.
// Contacts retain their slots/IDs when Android reorders its input array. A reset,
// focus change or viewport change blocks existing fingers until they lift.
// Press/release arrays last one update; buffer action edges for fixed ticks.
class TouchTracker {
public:
    bool IsActive(int id) const {
        return std::any_of(contacts.begin(), contacts.end(),
                           [id](const TouchContact& c) { return c.id == id && c.Active(); });
    }
    const std::array<TouchContact, 8>& Contacts() const {
        return contacts;
    }
    const std::array<TouchContact, 8>& Released() const {
        return released;
    }
    bool Cancelled() const {
        return cancelled;
    }

    void Reset(const InputFrame& frame) {
        contacts = {};
        released = {};
        for (int i = 0; i < std::clamp(frame.touchCount, 0, 8); ++i) {
            if (Valid(frame.touches[i]) && !Find(frame.touches[i].id)) {
                contacts[i].id = frame.touches[i].id;
                contacts[i].blocked = true;
            }
        }
    }

    void Update(const InputFrame& frame, const CanvasViewport& viewport) {
        released = {};
        cancelled = !frame.focused || !wasFocused || (hasViewport && Changed(viewport));
        previous = viewport;
        hasViewport = true;
        wasFocused = frame.focused;
        if (cancelled) {
            Reset(frame);
            return;
        }
        int count = std::clamp(frame.touchCount, 0, 8);
        int releaseCount = 0;
        for (auto& contact : contacts) {
            contact.pressed = false;
            if (contact.id < 0)
                continue;
            const InputTouch* point = nullptr;
            for (int i = 0; i < count; ++i) {
                if (frame.touches[i].id == contact.id) {
                    point = &frame.touches[i];
                    if (!Valid(*point))
                        contact.blocked = true;
                    break;
                }
            }
            if (point) {
                if (Valid(*point))
                    contact.position = viewport.ToCanvas(point->position);
            } else {
                if (contact.Active())
                    released[releaseCount++] = contact;
                contact = {};
            }
        }
        for (int i = 0; i < count; ++i) {
            const auto& point = frame.touches[i];
            if (!Valid(point) || Find(point.id))
                continue;
            auto slot = std::find_if(contacts.begin(), contacts.end(),
                                     [](const TouchContact& c) { return c.id < 0; });
            if (slot == contacts.end())
                break;
            auto position = viewport.ToCanvas(point.position);
            *slot = {point.id, position, position, true, !viewport.Contains(point.position)};
        }
    }

private:
    std::array<TouchContact, 8> contacts{}, released{};
    CanvasViewport previous;
    bool hasViewport = false, wasFocused = true, cancelled = false;

    static bool Valid(const InputTouch& touch) {
        return touch.id >= 0 && std::isfinite(touch.position.x) && std::isfinite(touch.position.y);
    }
    bool Find(int id) const {
        return std::any_of(contacts.begin(), contacts.end(),
                           [id](const TouchContact& c) { return c.id == id; });
    }
    bool Changed(const CanvasViewport& next) const {
        return previous.bounds.x != next.bounds.x || previous.bounds.y != next.bounds.y ||
               previous.bounds.width != next.bounds.width ||
               previous.bounds.height != next.bounds.height || previous.scale != next.scale ||
               previous.logicalSize.x != next.logicalSize.x ||
               previous.logicalSize.y != next.logicalSize.y;
    }
};

} // namespace RayGame
