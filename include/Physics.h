#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "Time.h"

namespace RayGame {

enum class ColliderType {
    Box,
    Circle
};

class RAYGAME_API Collider : public Component {
public:
    ColliderType type;
    Vector2 offset = {0, 0};
    bool isTrigger = false;
    
    virtual bool CheckCollision(Collider* other) = 0;
    virtual void DebugDraw() = 0;
    
    Vector2 GetWorldPosition() const {
        if (!gameObject) return {0, 0};
        return {
            gameObject->transform->position.x + offset.x,
            gameObject->transform->position.y + offset.y
        };
    }
    
protected:
    Collider(ColliderType t) : type(t) {}
};

class RAYGAME_API BoxCollider : public Collider {
public:
    Vector2 size = {32, 32};
    
    BoxCollider() : Collider(ColliderType::Box) {}
    BoxCollider(Vector2 sz) : Collider(ColliderType::Box), size(sz) {}
    
    Rectangle GetBounds() const {
        Vector2 pos = GetWorldPosition();
        return {
            pos.x - size.x / 2,
            pos.y - size.y / 2,
            size.x,
            size.y
        };
    }
    
    bool CheckCollision(Collider* other) override {
        if (other->type == ColliderType::Box) {
            auto* otherBox = static_cast<BoxCollider*>(other);
            return CheckCollisionRecs(GetBounds(), otherBox->GetBounds());
        }
        return false;
    }
    
    void DebugDraw() override {
        Rectangle bounds = GetBounds();
        DrawRectangleLinesEx(bounds, 2, isTrigger ? YELLOW : GREEN);
    }
};

class RAYGAME_API CircleCollider : public Collider {
public:
    float radius = 16.0f;
    
    CircleCollider() : Collider(ColliderType::Circle) {}
    CircleCollider(float r) : Collider(ColliderType::Circle), radius(r) {}
    
    bool CheckCollision(Collider* other) override {
        if (other->type == ColliderType::Circle) {
            auto* otherCircle = static_cast<CircleCollider*>(other);
            return CheckCollisionCircles(
                GetWorldPosition(), radius,
                otherCircle->GetWorldPosition(), otherCircle->radius
            );
        }
        return false;
    }
    
    void DebugDraw() override {
        Vector2 pos = GetWorldPosition();
        DrawCircleLines(pos.x, pos.y, radius, isTrigger ? YELLOW : GREEN);
    }
};

class RAYGAME_API Rigidbody : public Component {
public:
    Vector2 velocity = {0, 0};
    Vector2 acceleration = {0, 0};
    float mass = 1.0f;
    float drag = 0.99f;  // Air resistance
    float gravity = 0.0f;
    bool useGravity = false;
    bool isKinematic = false;  // If true, not affected by forces
    
    Rigidbody() = default;
    
    void AddForce(Vector2 force) {
        if (isKinematic) return;
        acceleration.x += force.x / mass;
        acceleration.y += force.y / mass;
    }
    
    void AddImpulse(Vector2 impulse) {
        if (isKinematic) return;
        velocity.x += impulse.x / mass;
        velocity.y += impulse.y / mass;
    }
    
    void Update() override {
        if (isKinematic || !gameObject) return;
        
        float dt = GameTime::DeltaTime();
        
        // Apply gravity
        if (useGravity) {
            acceleration.y += gravity;
        }
        
        // Update velocity
        velocity.x += acceleration.x * dt;
        velocity.y += acceleration.y * dt;
        
        // Apply drag
        velocity.x *= drag;
        velocity.y *= drag;
        
        // Update position
        gameObject->transform->position.x += velocity.x * dt;
        gameObject->transform->position.y += velocity.y * dt;
        
        // Reset acceleration
        acceleration = {0, 0};
    }
    
    void SetVelocity(Vector2 vel) {
        velocity = vel;
    }
    
    Vector2 GetVelocity() const {
        return velocity;
    }
    
    float GetSpeed() const {
        return sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    }
};

class RAYGAME_API PhysicsWorld {
public:
    static void AddCollider(Collider* collider) {
        colliders.push_back(collider);
    }
    
    static void RemoveCollider(Collider* collider) {
        colliders.erase(
            std::remove(colliders.begin(), colliders.end(), collider),
            colliders.end()
        );
    }
    
    static void CheckCollisions() {
        for (size_t i = 0; i < colliders.size(); i++) {
            for (size_t j = i + 1; j < colliders.size(); j++) {
                if (!colliders[i]->IsEnabled() || !colliders[j]->IsEnabled()) continue;
                
                if (colliders[i]->CheckCollision(colliders[j])) {
                    OnCollision(colliders[i], colliders[j]);
                }
            }
        }
    }
    
    static void DebugDraw() {
        for (auto* collider : colliders) {
            if (collider->IsEnabled()) {
                collider->DebugDraw();
            }
        }
    }
    
    static void Clear() {
        colliders.clear();
    }
    
private:
    static inline std::vector<Collider*> colliders;
    
    static void OnCollision(Collider* a, Collider* b) {
        // Simple collision response for non-triggers
        if (a->isTrigger || b->isTrigger) return;
        
        auto* rbA = a->GetGameObject()->GetComponent<Rigidbody>().get();
        auto* rbB = b->GetGameObject()->GetComponent<Rigidbody>().get();
        
        if (rbA && rbB && !rbA->isKinematic && !rbB->isKinematic) {
            // Simple elastic collision
            Vector2 normal = Normalize({
                b->GetWorldPosition().x - a->GetWorldPosition().x,
                b->GetWorldPosition().y - a->GetWorldPosition().y
            });
            
            Vector2 relVel = {
                rbA->velocity.x - rbB->velocity.x,
                rbA->velocity.y - rbB->velocity.y
            };
            
            float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;
            
            if (velAlongNormal > 0) return;  // Objects moving apart
            
            float restitution = 0.8f;  // Bounciness
            float j = -(1 + restitution) * velAlongNormal;
            j /= 1/rbA->mass + 1/rbB->mass;
            
            Vector2 impulse = {normal.x * j, normal.y * j};
            
            rbA->AddImpulse({-impulse.x, -impulse.y});
            rbB->AddImpulse(impulse);
        }
    }
};

}

