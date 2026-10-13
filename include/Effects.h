#pragma once

#include "Component.h"
#include "GameTime.h"
#include "UI.h"

namespace RayGame {

// Short-lived combat feedback: glowing beams, expanding shockwave rings and floating
// text. Add one EffectLayer to a scene object and give it a renderOrder above actors.
// Effects age with simulated time, so they freeze while the game is paused.
class RAYGAME_API EffectLayer : public Component {
public:
    struct BeamOptions {
        float width = 8;
        float life = 0.3f;
        Color glow = {120, 220, 255, 255};
        Color core = WHITE;
    };
    struct RingOptions {
        float startRadius = 4;
        float endRadius = 40;
        float width = 4;
        float life = 0.45f;
        Color color = {255, 220, 160, 255};
    };
    struct TextOptions {
        float size = 16;
        float life = 1.1f;
        float rise = 28; // pixels per second
        Color color = WHITE;
        Color outline = {20, 10, 20, 255};
        bool heading = true;
    };

    // Beams and rings use additive blending when drawn; text is alpha blended.
    void Beam(Vector2 from, Vector2 to, const BeamOptions& options) {
        if (Finite(from) && Finite(to) && options.life > 0) beams.push_back({from, to, options, options.life});
    }
    void Ring(Vector2 center, const RingOptions& options) {
        if (Finite(center) && options.life > 0) rings.push_back({center, options, options.life});
    }
    void Text(Vector2 position, const std::string& text, const TextOptions& options) {
        if (Finite(position) && options.life > 0 && !text.empty())
            texts.push_back({position, text, options, options.life});
    }
    void Clear() {
        beams.clear();
        rings.clear();
        texts.clear();
    }

    size_t BeamCount() const { return beams.size(); }
    size_t RingCount() const { return rings.size(); }
    size_t TextCount() const { return texts.size(); }

    // Current ring radius, for gameplay that wants to follow a shockwave front.
    static float RingRadius(const RingOptions& options, float remaining) {
        float t = 1 - Clamp(remaining / options.life, 0.0f, 1.0f);
        float eased = 1 - (1 - t) * (1 - t) * (1 - t);
        return Lerp(options.startRadius, options.endRadius, eased);
    }

    void Update() override { Advance(GameTime::DeltaTime()); }

    void Advance(float dt) {
        if (!(dt > 0)) return;
        for (auto& b : beams) b.remaining -= dt;
        for (auto& r : rings) r.remaining -= dt;
        for (auto& t : texts) {
            t.remaining -= dt;
            t.position.y -= t.options.rise * dt;
        }
        auto expired = [](const auto& e) { return e.remaining <= 0; };
        beams.erase(std::remove_if(beams.begin(), beams.end(), expired), beams.end());
        rings.erase(std::remove_if(rings.begin(), rings.end(), expired), rings.end());
        texts.erase(std::remove_if(texts.begin(), texts.end(), expired), texts.end());
    }

    void Render() override {
        if (!beams.empty() || !rings.empty()) {
            BeginBlendMode(BLEND_ADDITIVE);
            for (const auto& b : beams) {
                float a = Clamp(b.remaining / b.options.life, 0.0f, 1.0f);
                float w = b.options.width * (0.5f + 0.5f * a);
                DrawLineEx(b.from, b.to, w * 2.6f, Fade(b.options.glow, a * 0.25f));
                DrawLineEx(b.from, b.to, w * 1.3f, Fade(b.options.glow, a * 0.7f));
                DrawLineEx(b.from, b.to, w * 0.45f, Fade(b.options.core, a));
                DrawCircleV(b.to, w * 0.9f, Fade(b.options.glow, a * 0.6f));
                DrawCircleV(b.from, w * 0.7f, Fade(b.options.core, a * 0.6f));
            }
            for (const auto& r : rings) {
                float a = Clamp(r.remaining / r.options.life, 0.0f, 1.0f);
                float radius = RingRadius(r.options, r.remaining);
                float w = r.options.width;
                DrawRing(r.center, std::max(0.0f, radius - w), radius, 0, 360, 48, Fade(r.options.color, a * 0.85f));
                DrawRing(r.center, std::max(0.0f, radius - w * 3), std::max(0.0f, radius - w), 0, 360, 48,
                         Fade(r.options.color, a * 0.2f));
            }
            EndBlendMode();
        }
        for (const auto& t : texts) {
            float a = Clamp(t.remaining / (t.options.life * 0.4f), 0.0f, 1.0f);
            // A brief pop when the text appears.
            float age = t.options.life - t.remaining;
            float size = t.options.size * (1 + 0.3f * Clamp(1 - age / 0.15f, 0.0f, 1.0f));
            UI::Text(t.position.x, t.position.y, t.text, size, Fade(t.options.color, a), UI::Align::Center,
                     t.options.heading, 1.6f, Fade(t.options.outline, a * 0.9f));
        }
    }

private:
    struct BeamFx { Vector2 from, to; BeamOptions options; float remaining; };
    struct RingFx { Vector2 center; RingOptions options; float remaining; };
    struct TextFx { Vector2 position; std::string text; TextOptions options; float remaining; };
    std::vector<BeamFx> beams;
    std::vector<RingFx> rings;
    std::vector<TextFx> texts;

    static bool Finite(Vector2 v) { return std::isfinite(v.x) && std::isfinite(v.y); }
};

} // namespace RayGame
