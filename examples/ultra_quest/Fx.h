#pragma once

#include "Common.h"

namespace UQ {

// Short-lived screen-space-agnostic effects. Positions are in whatever space the owner draws
// them in (the fight uses world units, the street uses street pixels); Draw takes a mapping.
class Fx {
public:
    enum class Kind { Spark, Dust, Debris, Ring, Flash, Ember, Text, Streak, Burst };
    struct Particle {
        Kind kind;
        Vector2 p, v;
        float life, maxLife;
        float size;
        Color color;
        float spin = 0, angle = 0;
        float gravity = 0;
        std::string text;
    };

    void Sparks(Vector2 at, Color color, int count, float speed, float size = 4);
    void Dust(Vector2 at, int count, float spread, Color color = {170, 150, 120, 200});
    void Debris(Vector2 at, int count, Color color = {120, 104, 90, 255});
    void Ring(Vector2 at, float radius, Color color, float life = 0.35f);
    void Flash(Vector2 at, float radius, Color color, float life = 0.18f);
    void Embers(Vector2 at, int count, Color color);
    void Streak(Vector2 at, Vector2 velocity, float length, Color color, float life = 0.18f);
    void Text(Vector2 at, const std::string& text, Color color, float size = 34, float life = 1.1f);
    // A hit: star-shaped flash, ring, sparks and speed lines along `dir` (+1 right, -1 left).
    // `power` 1 is a jab, 2 a finisher.
    void Impact(Vector2 at, int dir, float power, Color color);
    // A damage number that pops and drifts up.
    void Number(Vector2 at, int value, Color color, float size);

    // Ground height for bouncing debris, in the owner's space; y grows upward when `upIsPositive`.
    void Update(float dt, bool upIsPositive);
    // `map` converts a particle position to canvas pixels; `scale` converts sizes.
    template <class Map> void Draw(Map map, float scale) const;
    void Clear() { particles.clear(); }
    bool Empty() const { return particles.empty(); }

private:
    std::vector<Particle> particles;
    Rng rng{99};
    void DrawOne(const Particle& p, Vector2 at, float scale) const;
};

template <class Map> void Fx::Draw(Map map, float scale) const {
    for (const auto& p : particles) DrawOne(p, map(p.p), scale);
}

} // namespace UQ
