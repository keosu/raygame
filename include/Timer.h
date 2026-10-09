#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "GameTime.h"

namespace RayGame {

// Destroys the owning GameObject after `lifetime` seconds.
class RAYGAME_API DestroyAfter : public Component {
public:
    float lifetime = 1.0f;
    float elapsed = 0.0f;

    DestroyAfter() = default;
    DestroyAfter(float seconds) : lifetime(seconds) {}

    void Update() override {
        elapsed += GameTime::DeltaTime();
        if (elapsed >= lifetime && gameObject) {
            gameObject->MarkDestroyed();
        }
    }
};

// Fires a callback after `interval` seconds; optionally repeating.
class RAYGAME_API TimerComponent : public Component {
public:
    float interval = 1.0f;
    bool repeat = false;
    std::function<void()> onElapsed;

    TimerComponent() = default;
    TimerComponent(float seconds, std::function<void()> cb, bool repeating = false)
        : interval(seconds), repeat(repeating), onElapsed(std::move(cb)) {}

    void Reset() { elapsed = 0; done = false; }

    void Update() override {
        if (done) return;
        elapsed += GameTime::DeltaTime();
        while (!done && elapsed >= std::max(0.0f, interval)) {
            if (repeat && interval > 0) elapsed -= interval;
            else { elapsed = 0; done = !repeat; }
            auto callback = onElapsed;
            if (callback) callback();
            if (interval <= 0 || !IsEnabled() || (gameObject && gameObject->IsDestroyed())) break;
        }
    }

private:
    float elapsed = 0;
    bool done = false;
};

}
