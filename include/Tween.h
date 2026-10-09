#pragma once

#include "Core.h"

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
    static unsigned long Add(const TweenInstance& t) {
        auto inst = CreateRef<TweenInstance>(t);
        inst->id = ++nextId;
        tweens.push_back(inst);
        return inst->id;
    }

    static void Update(float dt) {
        if (updating || !std::isfinite(dt) || dt < 0) return;
        updating = true;
        struct UpdateScope { ~UpdateScope() { updating = false; } } scope;
        const auto snapshot = tweens;
        for (const auto& instance : snapshot) {
            auto& t = *instance;
            if (!t.active) continue;
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
            if (t.onUpdate) t.onUpdate(v);

            if (t.active && complete) {
                t.active = false;
                if (t.onComplete) t.onComplete();
            }
        }
        tweens.erase(std::remove_if(tweens.begin(), tweens.end(),
            [](const Ref<TweenInstance>& t) { return !t->active; }), tweens.end());
        updating = false;
    }

    static void Cancel(unsigned long id) {
        for (auto& t : tweens) if (t->id == id) t->active = false;
    }

    static void Clear() {
        for (const auto& t : tweens) t->active = false;
        tweens.clear();
    }
    static int Count() {
        return (int)std::count_if(tweens.begin(), tweens.end(),
            [](const Ref<TweenInstance>& t) { return t->active; });
    }

private:
    static inline std::vector<Ref<TweenInstance>> tweens;
    static inline unsigned long nextId = 0;
    static inline bool updating = false;
};

// Fluent helpers.
struct Tween {
    static unsigned long To(float from, float to, float duration, Ease ease,
                            std::function<void(float)> onUpdate,
                            std::function<void()> onComplete = nullptr) {
        TweenInstance t;
        t.from = from; t.to = to; t.duration = duration; t.ease = ease;
        t.onUpdate = std::move(onUpdate); t.onComplete = std::move(onComplete);
        return TweenManager::Add(t);
    }

    static unsigned long Delay(float seconds, std::function<void()> onComplete) {
        TweenInstance t;
        t.from = 0; t.to = 1; t.duration = 0; t.delay = std::max(0.0f, seconds);
        t.onComplete = std::move(onComplete);
        return TweenManager::Add(t);
    }
};

}
