#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "Time.h"

namespace RayGame {

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    Color color;
    float lifetime;
    float age;
    float size;
    float rotation;
    float rotationSpeed;
    bool active;
    
    void Update(float deltaTime) {
        if (!active) return;
        
        age += deltaTime;
        if (age >= lifetime) {
            active = false;
            return;
        }
        
        velocity.x += acceleration.x * deltaTime;
        velocity.y += acceleration.y * deltaTime;
        
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        
        rotation += rotationSpeed * deltaTime;
        
        // Fade out over lifetime
        float t = age / lifetime;
        color.a = (unsigned char)((1.0f - t) * 255);
    }
};

enum class EmitterShape {
    Point,
    Circle,
    Box,
    Cone
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
    
    float lifetimeMin = 1.0f;
    float lifetimeMax = 2.0f;
    
    float startSizeMin = 2.0f;
    float startSizeMax = 5.0f;
    
    Color startColor = WHITE;
    Color endColor = {255, 255, 255, 0};
    
    float rotationSpeedMin = -180.0f;
    float rotationSpeedMax = 180.0f;
    
    ParticleEmitter() {
        particles.reserve(maxParticles);
    }
    
    void Start() override {
        emissionTimer = 0;
        durationTimer = 0;
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
        if (emitting) {
            emissionTimer += dt;
            float interval = 1.0f / emissionRate;
            
            while (emissionTimer >= interval && particles.size() < maxParticles) {
                EmitParticle();
                emissionTimer -= interval;
            }
        }
        
        // Update particles
        for (auto& p : particles) {
            p.Update(dt);
        }
        
        // Remove dead particles
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return !p.active; }),
            particles.end()
        );
    }
    
    void Render() override {
        for (const auto& p : particles) {
            if (p.active) {
                DrawCircleV(p.position, p.size, p.color);
            }
        }
    }
    
    void EmitParticle() {
        if (particles.size() >= maxParticles) return;
        
        Particle p;
        p.position = GetEmissionPosition();
        p.velocity = Random(startVelocityMin, startVelocityMax);
        p.acceleration = acceleration;
        p.lifetime = Random(lifetimeMin, lifetimeMax);
        p.age = 0;
        p.size = Random(startSizeMin, startSizeMax);
        p.rotation = Random(0.0f, 360.0f);
        p.rotationSpeed = Random(rotationSpeedMin, rotationSpeedMax);
        p.color = startColor;
        p.active = true;
        
        particles.push_back(p);
    }
    
    void Burst(int count) {
        for (int i = 0; i < count && particles.size() < maxParticles; i++) {
            EmitParticle();
        }
    }
    
    void Clear() {
        particles.clear();
    }
    
    int GetParticleCount() const {
        return particles.size();
    }
    
private:
    std::vector<Particle> particles;
    float emissionTimer = 0;
    float durationTimer = 0;
    
    Vector2 GetEmissionPosition() {
        Vector2 pos = gameObject->transform->position;
        
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
                float baseAngle = gameObject->transform->rotation;
                float angle = (baseAngle + Random(-coneAngle/2, coneAngle/2)) * DEG2RAD;
                float r = Random(0, radius);
                return {pos.x + cos(angle) * r, pos.y + sin(angle) * r};
            }
        }
        
        return pos;
    }
};

}

