#pragma once
#include "TouchTracker.h"

namespace RayGame {
// One captured mouse/touch pointer per context. A second held finger is never promoted.
// Update once per render frame; consume edges there or buffer commands for fixed ticks.
class PointerGesture {
public:
    float dragThreshold = 8;
    bool pressed = false, released = false, held = false, dragging = false, cancelled = false;
    Vector2 start{}, position{};
    bool IsTouch() const {
        return touch;
    }
    void Reset(const InputFrame& frame = {}) {
        tracker.Reset(frame);
        finger = -1;
        pressed = released = held = dragging = cancelled = false;
        mouseBlocked = frame.mouse.held[MOUSE_BUTTON_LEFT];
    }
    void Update(const InputFrame& frame, const CanvasViewport& viewport) {
        pressed = released = cancelled = false;
        tracker.Update(frame, viewport);
        if (tracker.Cancelled()) {
            Cancel(frame);
            return;
        }
        if (!held) {
            dragging = false;
            for (const auto& contact : tracker.Contacts()) {
                if (contact.Active() && contact.pressed) {
                    finger = contact.id;
                    touch = held = pressed = true;
                    start = position = contact.position;
                    break;
                }
            }
#if !defined(__ANDROID__)
            if (!held && frame.touchCount == 0 && !mouseBlocked &&
                frame.mouse.pressed[MOUSE_BUTTON_LEFT] && viewport.Contains(frame.mousePosition) &&
                Valid(frame.mousePosition)) {
                touch = false;
                held = pressed = true;
                start = position = viewport.ToCanvas(frame.mousePosition);
            }
#endif
        }
        if (held && touch) {
            for (const auto& contact : tracker.Contacts())
                if (contact.Active() && contact.id == finger)
                    position = contact.position;
            for (const auto& contact : tracker.Released())
                if (contact.Active() && contact.id == finger) {
                    position = contact.position;
                    held = false;
                    released = true;
                    finger = -1;
                }
            if (held && !tracker.IsActive(finger)) {
                Cancel(frame);
                return;
            }
        } else if (held) {
            if (!Valid(frame.mousePosition)) {
                Cancel(frame);
                return;
            }
            position = viewport.ToCanvas(frame.mousePosition);
            if (frame.mouse.released[MOUSE_BUTTON_LEFT]) {
                held = false;
                released = true;
            } else if (!frame.mouse.held[MOUSE_BUTTON_LEFT]) {
                Cancel(frame);
                return;
            }
        }
        if (held || released) {
            float dx = position.x - start.x, dy = position.y - start.y;
            dragging |= dx * dx + dy * dy >= dragThreshold * dragThreshold;
        }
        if (!frame.mouse.held[MOUSE_BUTTON_LEFT])
            mouseBlocked = false;
    }

private:
    TouchTracker tracker;
    int finger = -1;
    bool touch = false, mouseBlocked = false;
    static bool Valid(Vector2 p) {
        return std::isfinite(p.x) && std::isfinite(p.y);
    }
    void Cancel(const InputFrame& frame) {
        Reset(frame);
        cancelled = true;
    }
};
} // namespace RayGame
