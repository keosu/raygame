#include "Fx.h"
#include "Draw.h"
#include "Gui.h"

namespace UQ {

void Fx::Sparks(Vector2 at, Color color, int count, float speed, float size) {
    for (int i = 0; i < count; ++i) {
        float a = rng.Range(0, 2 * PI);
        float s = speed * rng.Range(0.35f, 1.0f);
        particles.push_back({Kind::Spark, at, Mul(Dir(a), s), 0, rng.Range(0.18f, 0.38f), size * rng.Range(0.7f, 1.3f),
                             color, 0, a, 0});
    }
}

void Fx::Dust(Vector2 at, int count, float spread, Color color) {
    for (int i = 0; i < count; ++i) {
        Vector2 p = {at.x + rng.Range(-spread, spread) * 0.4f, at.y + rng.Range(0, 10)};
        Vector2 v = {rng.Range(-spread, spread), rng.Range(10, 70)};
        particles.push_back({Kind::Dust, p, v, 0, rng.Range(0.6f, 1.2f), rng.Range(18, 40), color, 0, 0, -20});
    }
}

void Fx::Debris(Vector2 at, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        Vector2 v = {rng.Range(-420, 420), rng.Range(200, 760)};
        particles.push_back({Kind::Debris, at, v, 0, rng.Range(0.8f, 1.5f), rng.Range(6, 16),
                             Mix(color, BLACK, rng.Range(0, 0.4f)), rng.Range(-8, 8), rng.Range(0, 6), 1800});
    }
}

void Fx::Ring(Vector2 at, float radius, Color color, float life) {
    particles.push_back({Kind::Ring, at, {0, 0}, 0, life, radius, color});
}

void Fx::Flash(Vector2 at, float radius, Color color, float life) {
    particles.push_back({Kind::Flash, at, {0, 0}, 0, life, radius, color});
}

void Fx::Embers(Vector2 at, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        Vector2 v = {rng.Range(-120, 120), rng.Range(60, 260)};
        particles.push_back({Kind::Ember, {at.x + rng.Range(-30, 30), at.y + rng.Range(-30, 30)}, v, 0,
                             rng.Range(0.6f, 1.4f), rng.Range(2, 5), color, 0, 0, -60});
    }
}

void Fx::Streak(Vector2 at, Vector2 velocity, float length, Color color, float life) {
    particles.push_back({Kind::Streak, at, velocity, 0, life, length, color});
}

void Fx::Text(Vector2 at, const std::string& text, Color color, float size, float life) {
    Particle p{Kind::Text, at, {0, 50}, 0, life, size, color};
    p.text = text;
    particles.push_back(p);
}

void Fx::Update(float dt, bool upIsPositive) {
    float up = upIsPositive ? 1.0f : -1.0f;
    for (auto& p : particles) {
        p.life += dt;
        p.v.y -= p.gravity * dt;
        if (p.kind == Kind::Spark) p.v = Mul(p.v, std::pow(0.02f, dt));
        if (p.kind == Kind::Dust) p.v = Mul(p.v, std::pow(0.15f, dt));
        p.p.x += p.v.x * dt;
        p.p.y += p.v.y * dt * up;
        p.angle += p.spin * dt;
        if (p.kind == Kind::Debris && upIsPositive && p.p.y < 0) {
            p.p.y = 0;
            p.v.y = std::fabs(p.v.y) * 0.3f;
            p.v.x *= 0.6f;
        }
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p) { return p.life >= p.maxLife; }),
                    particles.end());
}

void Fx::DrawOne(const Particle& p, Vector2 at, float scale) const {
    float t = p.life / p.maxLife;
    float fade = 1 - t;
    switch (p.kind) {
    case Kind::Spark: {
        Draw::BlendAdd();
        Vector2 tail = {at.x - p.v.x * 0.03f * scale, at.y + p.v.y * 0.03f * scale};
        Draw::Line(tail, at, p.size * scale * fade, MulAlpha(p.color, fade));
        Draw::Glow(at, p.size * 3 * scale, MulAlpha(p.color, fade * 0.5f));
        Draw::BlendNormal();
        break;
    }
    case Kind::Dust:
        DrawCircleV(at, p.size * scale * (0.6f + t * 0.8f), MulAlpha(p.color, fade * 0.55f));
        break;
    case Kind::Debris: {
        float s = p.size * scale;
        DrawRectanglePro({at.x, at.y, s, s * 0.7f}, {s / 2, s * 0.35f}, p.angle * RAD2DEG, MulAlpha(p.color, std::min(1.0f, fade * 3)));
        break;
    }
    case Kind::Ring:
        DrawRing(at, p.size * scale * EaseOut(t) * 0.9f, p.size * scale * EaseOut(t), 0, 360, 48,
                 MulAlpha(p.color, fade));
        break;
    case Kind::Flash:
        Draw::BlendAdd();
        Draw::Glow(at, p.size * scale * (0.6f + 0.6f * t), MulAlpha(p.color, fade));
        Draw::BlendNormal();
        break;
    case Kind::Ember:
        Draw::BlendAdd();
        DrawCircleV(at, p.size * scale, MulAlpha(p.color, fade));
        Draw::BlendNormal();
        break;
    case Kind::Streak: {
        Vector2 dir = Len(p.v) > 0 ? Mul(p.v, 1 / Len(p.v)) : Vector2{1, 0};
        Vector2 tail = {at.x - dir.x * p.size * scale, at.y + dir.y * p.size * scale};
        Draw::BlendAdd();
        Draw::Line(tail, at, 6 * scale * fade, MulAlpha(p.color, fade * 0.8f));
        Draw::BlendNormal();
        break;
    }
    case Kind::Text: {
        float pop = t < 0.12f ? 0.6f + t / 0.12f * 0.5f : 1.1f - std::min(0.1f, (t - 0.12f));
        float size = p.size * pop;
        Gui::TextOutline(p.text, at.x, at.y - size / 2, size, MulAlpha(p.color, std::min(1.0f, fade * 2.5f)),
                         MulAlpha({10, 14, 30, 255}, std::min(1.0f, fade * 2.5f)), 3, Gui::Align::Center);
        break;
    }
    }
}

} // namespace UQ
