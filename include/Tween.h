#pragma once

#include "Core.h"
#include "CallbackScope.h"

namespace RayGame {

enum class Ease {
    Linear,
    QuadIn, QuadOut, QuadInOut,
    CubicIn, CubicOut, CubicInOut,
    SineIn, SineOut, SineInOut,
    ExpoOut,
    BackOut,
    ElasticOut,
    BounceOut
};

inline float ApplyEase(Ease e, float t) {
    switch (e) {
        case Ease::Linear:    return t;
        case Ease::QuadIn:    return t * t;
        case Ease::QuadOut:   return t * (2 - t);
        case Ease::QuadInOut: return t < 0.5f ? 2*t*t : -1 + (4 - 2*t) * t;
        case Ease::CubicIn:   return t * t * t;
        case Ease::CubicOut:  { float f = t - 1; return f*f*f + 1; }
        case Ease::CubicInOut:return t < 0.5f ? 4*t*t*t : 1 - std::pow(-2*t + 2, 3) / 2;
        case Ease::SineIn:    return 1 - std::cos((t * PI) / 2);
        case Ease::SineOut:   return std::sin((t * PI) / 2);
        case Ease::SineInOut: return -(std::cos(PI * t) - 1) / 2;
        case Ease::ExpoOut:   return t >= 1 ? 1 : 1 - std::pow(2, -10 * t);
        case Ease::BackOut:   { float c1 = 1.70158f, c3 = c1 + 1; float f = t - 1; return 1 + c3*f*f*f + c1*f*f; }
        case Ease::ElasticOut:{
            if (t == 0 || t == 1) return t;
            float c4 = (2 * PI) / 3;
            return std::pow(2, -10*t) * std::sin((t*10 - 0.75f) * c4) + 1;
        }
        case Ease::BounceOut: {
            const float n1 = 7.5625f, d1 = 2.75f;
            if (t < 1/d1) return n1*t*t;
            else if (t < 2/d1) { t -= 1.5f/d1; return n1*t*t + 0.75f; }
            else if (t < 2.5f/d1) { t -= 2.25f/d1; return n1*t*t + 0.9375f; }
            else { t -= 2.625f/d1; return n1*t*t + 0.984375f; }
        }
    }
    return t;
}

struct TweenInstance {
    float from = 0, to = 1;
    float duration = 1, elapsed = 0, delay = 0;
    Ease ease = Ease::Linear;
    bool active = true;
    bool loop = false;
    bool pingpong = false;
    bool reversed = false;
    std::function<void(float)> onUpdate;
    std::function<void()> onComplete;
    unsigned long id = 0;
};

class RAYGAME_API TweenManager {
public:
    static unsigned long Add(const TweenInstance& t, ScopeToken owner = ScopeToken::Current()) {
        auto inst = CreateRef<ManagedTween>(t);
        inst->owner = owner;
        // Copyable wrappers preserve mutable callback state across invocations.
        if (t.onUpdate) {
            auto callback = std::make_shared<std::function<void(float)>>(t.onUpdate);
            inst->onUpdate = [callback](float value) { (*callback)(value); };
        }
        if (t.onComplete) {
            auto callback = std::make_shared<std::function<void()>>(t.onComplete);
            inst->onComplete = [callback] { (*callback)(); };
        }
        inst->id = ++nextId;
        std::weak_ptr<ManagedTween> weak = inst;
        inst->cancellation = owner.OnCancel([weak] {
            if (auto value = weak.lock()) Stop(*value);
        });
        if (inst->active) tweens.push_back(inst);
        return inst->id;
    }

    static void Update(float dt) {
        if (updating || !std::isfinite(dt) || dt < 0) return;
        updating = true;
        struct UpdateScope { ~UpdateScope() { updating = false; } } scope;
        const auto snapshot = tweens;
        for (const auto& instance : snapshot) {
            auto& t = *instance;
            if (!t.active || !t.owner.IsAlive()) { Stop(t); continue; }
            ScopeActivation ownerScope(t.owner);
            float remaining = dt;
            if (t.delay > 0) {
                float consumed = std::min(t.delay, remaining);
                t.delay -= consumed;
                remaining -= consumed;
                if (t.delay > 0) continue;
            }
            bool complete = t.duration <= 0;
            float raw = 1;
            if (t.duration > 0) {
                float position = t.elapsed + (t.reversed ? t.duration : 0) + remaining;
                float cycle = t.duration * (t.pingpong ? 2 : 1);
                complete = !t.loop && position >= cycle;
                position = t.loop ? std::fmod(position, cycle) : std::min(position, cycle);
                t.reversed = t.pingpong && position >= t.duration;
                t.elapsed = position - (t.reversed ? t.duration : 0);
                raw = Clamp(t.elapsed / t.duration, 0.0f, 1.0f);
            }
            float p = t.reversed ? 1.0f - raw : raw;
            float v = Lerp(t.from, t.to, ApplyEase(t.ease, p));
            auto update = t.onUpdate;
            if (update) update(v);

            if (t.active && complete) {
                auto completeCallback = t.onComplete;
                Stop(t);
                if (completeCallback) completeCallback();
            }
        }
        tweens.erase(std::remove_if(tweens.begin(), tweens.end(),
            [](const Ref<ManagedTween>& t) { return !t->active; }), tweens.end());
        updating = false;
    }

    static void Cancel(unsigned long id) {
        auto found = std::find_if(tweens.begin(), tweens.end(), [id](const auto& t) { return t->id == id; });
        if (found == tweens.end()) return;
        auto removed = *found;
        Stop(*removed);
    }

    static void Clear() {
        auto removed = std::move(tweens);
        tweens.clear();
        for (const auto& t : removed) Stop(*t);
    }
    static int Count() {
        return (int)std::count_if(tweens.begin(), tweens.end(),
            [](const Ref<ManagedTween>& t) { return t->active; });
    }

private:
    struct ManagedTween : TweenInstance {
        explicit ManagedTween(const TweenInstance& value) : TweenInstance(value) {}
        ScopeToken owner;
        CancellationLink cancellation;
    };
    static void Stop(ManagedTween& value) {
        value.active = false;
        value.cancellation.Reset();
        auto update = std::move(value.onUpdate);
        auto complete = std::move(value.onComplete);
    }
    static inline std::vector<Ref<ManagedTween>> tweens;
    static inline unsigned long nextId = 0;
    static inline bool updating = false;
};

// Fluent helpers.
struct Tween {
    static unsigned long To(float from, float to, float duration, Ease ease,
                            std::function<void(float)> onUpdate,
                            std::function<void()> onComplete = nullptr,
                            ScopeToken owner = ScopeToken::Current()) {
        TweenInstance t;
        t.from = from; t.to = to; t.duration = duration; t.ease = ease;
        t.onUpdate = std::move(onUpdate); t.onComplete = std::move(onComplete);
        return TweenManager::Add(t, owner);
    }

    static unsigned long Delay(float seconds, std::function<void()> onComplete,
                               ScopeToken owner = ScopeToken::Current()) {
        TweenInstance t;
        t.from = 0; t.to = 1; t.duration = 0; t.delay = std::max(0.0f, seconds);
        t.onComplete = std::move(onComplete);
        return TweenManager::Add(t, owner);
    }
};

}
