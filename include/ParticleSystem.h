#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "GameTime.h"
#include "ResourceHandle.h"

namespace RayGame {

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    Color color;     // current color, interpolated from the emitter's start/end colors
    float lifetime;
    float age;
    float size;      // current radius in pixels
    float startSize;
    float rotation;  // degrees; visible on textured particles
    float rotationSpeed;
    bool active;

    // Advances motion and age. `drag` is the fraction of velocity lost per second.
    void Update(float deltaTime, float drag = 0.0f) {
        if (!active) return;

        age += deltaTime;
        if (age >= lifetime) {
            active = false;
            return;
        }

        velocity.x += acceleration.x * deltaTime;
        velocity.y += acceleration.y * deltaTime;
        if (drag > 0) {
            float keep = std::exp(-drag * deltaTime);
            velocity.x *= keep;
            velocity.y *= keep;
        }

        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;

        rotation += rotationSpeed * deltaTime;
    }

    float Progress() const { return lifetime > 0 ? Clamp(age / lifetime, 0.0f, 1.0f) : 1.0f; }
};

enum class EmitterShape {
    Point,
    Circle,
    Box,
    Cone
};

enum class ParticleBlend {
    Alpha,
    Additive  // light-like effects: sparks, glows, energy beams
};

class RAYGAME_API ParticleEmitter : public Component {
public:
    // Emitter properties
    EmitterShape shape = EmitterShape::Point;
    float emissionRate = 10.0f;  // particles per second
    int maxParticles = 100;
    bool emitting = true;
    bool loop = true;
    float duration = 5.0f;

    // Shape parameters
    float radius = 10.0f;  // For Circle
    Vector2 boxSize = {20, 20};  // For Box
    float coneAngle = 45.0f;  // For Cone (degrees)

    // Particle properties
    Vector2 startVelocityMin = {-50, -50};
    Vector2 startVelocityMax = {50, -100};
    Vector2 acceleration = {0, 100};  // Gravity
    float drag = 0.0f;                // fraction of velocity lost per second

    float lifetimeMin = 1.0f;
    float lifetimeMax = 2.0f;

    float startSizeMin = 2.0f;
    float startSizeMax = 5.0f;
    float endSizeScale = 1.0f;  // size at the end of life, relative to the start size

    // Interpolated over each particle's lifetime, including alpha.
    Color startColor = WHITE;
    Color endColor = {255, 255, 255, 0};

    float rotationSpeedMin = -180.0f;
    float rotationSpeedMax = 180.0f;

    // Optional sprite scaled to the particle diameter; circles are drawn without one.
    TextureHandle texture;
    ParticleBlend blend = ParticleBlend::Alpha;

    void Start() override {
        emissionTimer = 0;
        durationTimer = 0;
        particles.reserve((size_t)std::max(0, maxParticles));
    }

    void Update() override {
        if (!gameObject) return;

        float dt = GameTime::DeltaTime();

        // Update duration
        if (!loop) {
            durationTimer += dt;
            if (durationTimer >= duration) {
                emitting = false;
            }
        }

        // Emit particles
        if (emitting && emissionRate > 0) {
            emissionTimer += dt;
            float interval = 1.0f / emissionRate;

            while (emissionTimer >= interval && HasCapacity()) {
                EmitParticle();
                emissionTimer -= interval;
            }
            emissionTimer = std::fmod(emissionTimer, interval);
        }

        for (auto& p : particles) {
            p.Update(dt, drag);
            if (p.active) Apply(p);
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return !p.active; }),
            particles.end()
        );
    }

    void Render() override {
        const Texture2D* sprite = texture.Get();
        if (blend == ParticleBlend::Additive) BeginBlendMode(BLEND_ADDITIVE);
        for (const auto& p : particles) {
            if (!p.active) continue;
            if (sprite) {
                Rectangle source{0, 0, (float)sprite->width, (float)sprite->height};
                Rectangle dest{p.position.x, p.position.y, p.size * 2, p.size * 2};
                DrawTexturePro(*sprite, source, dest, {p.size, p.size}, p.rotation, p.color);
            } else {
                DrawCircleV(p.position, p.size, p.color);
            }
        }
        if (blend == ParticleBlend::Additive) EndBlendMode();
    }

    void EmitParticle() {
        if (!gameObject || !HasCapacity()) return;

        Particle p;
        p.position = GetEmissionPosition();
        p.velocity = Random(startVelocityMin, startVelocityMax);
        p.acceleration = acceleration;
        p.lifetime = Random(lifetimeMin, lifetimeMax);
        p.age = 0;
        p.startSize = Random(startSizeMin, startSizeMax);
        p.rotation = Random(0.0f, 360.0f);
        p.rotationSpeed = Random(rotationSpeedMin, rotationSpeedMax);
        p.active = true;
        Apply(p);

        particles.push_back(p);
    }

    void Burst(int count) {
        for (int i = 0; i < count && HasCapacity(); i++) {
            EmitParticle();
        }
    }

    void Clear() {
        particles.clear();
    }

    int GetParticleCount() const {
        return (int)particles.size();
    }

    const std::vector<Particle>& GetParticles() const { return particles; }

private:
    std::vector<Particle> particles;
    float emissionTimer = 0;
    float durationTimer = 0;

    bool HasCapacity() const { return maxParticles > 0 && particles.size() < (size_t)maxParticles; }

    void Apply(Particle& p) const {
        float t = p.Progress();
        auto channel = [t](unsigned char a, unsigned char b) {
            return (unsigned char)std::lround(a + (b - a) * t);
        };
        p.color = {channel(startColor.r, endColor.r), channel(startColor.g, endColor.g),
                   channel(startColor.b, endColor.b), channel(startColor.a, endColor.a)};
        p.size = p.startSize * Lerp(1.0f, endSizeScale, t);
    }

    Vector2 GetEmissionPosition() {
        Vector2 pos = gameObject->transform->GetWorldPosition();

        switch (shape) {
            case EmitterShape::Point:
                return pos;

            case EmitterShape::Circle: {
                float angle = Random(0, 360) * DEG2RAD;
                float r = Random(0, radius);
                return {pos.x + cos(angle) * r, pos.y + sin(angle) * r};
            }

            case EmitterShape::Box:
                return {
                    pos.x + Random(-boxSize.x/2, boxSize.x/2),
                    pos.y + Random(-boxSize.y/2, boxSize.y/2)
                };

            case EmitterShape::Cone: {
                float baseAngle = gameObject->transform->GetWorldRotation();
                float angle = (baseAngle + Random(-coneAngle/2, coneAngle/2)) * DEG2RAD;
                float r = Random(0, radius);
                return {pos.x + cos(angle) * r, pos.y + sin(angle) * r};
            }
        }

        return pos;
    }
};

}
