#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "GameTime.h"
#include <map>
#include <unordered_set>

namespace RayGame {

enum class ColliderType { Box, Circle };

class RAYGAME_API Collider : public Component {
public:
    ColliderType type;
    Vector2 offset = {0, 0};
    bool isTrigger = false;
    int layer = 0;                  // 0..31
    unsigned int mask = 0xFFFFFFFFu;

    virtual void DebugDraw() = 0;
    bool CheckCollision(Collider* other);

    Vector2 GetWorldPosition() const {
        return gameObject ? gameObject->transform->TransformPoint(offset) : offset;
    }

    void Start() override;
    void OnDestroy() override;
    void OnEnable() override;
    void OnDisable() override;

protected:
    explicit Collider(ColliderType t) : type(t) {}
};

// Axis-aligned box; rotation affects its offset, not its orientation.
class RAYGAME_API BoxCollider : public Collider {
public:
    Vector2 size = {32, 32};
    BoxCollider() : Collider(ColliderType::Box) {}
    explicit BoxCollider(Vector2 sz) : Collider(ColliderType::Box), size(sz) {}

    Rectangle GetBounds() const {
        Vector2 pos = GetWorldPosition();
        Vector2 scale = gameObject ? gameObject->transform->GetWorldScale() : Vector2{1, 1};
        Vector2 s = {std::fabs(size.x * scale.x), std::fabs(size.y * scale.y)};
        return {pos.x - s.x / 2, pos.y - s.y / 2, s.x, s.y};
    }

    void DebugDraw() override { DrawRectangleLinesEx(GetBounds(), 2, isTrigger ? YELLOW : GREEN); }
};

class RAYGAME_API CircleCollider : public Collider {
public:
    float radius = 16.0f;
    CircleCollider() : Collider(ColliderType::Circle) {}
    explicit CircleCollider(float r) : Collider(ColliderType::Circle), radius(r) {}

    float GetWorldRadius() const {
        Vector2 scale = gameObject ? gameObject->transform->GetWorldScale() : Vector2{1, 1};
        return std::fabs(radius) * std::max(std::fabs(scale.x), std::fabs(scale.y));
    }

    void DebugDraw() override {
        Vector2 pos = GetWorldPosition();
        DrawCircleLines((int)pos.x, (int)pos.y, GetWorldRadius(), isTrigger ? YELLOW : GREEN);
    }
};

inline bool Collide(Collider* a, Collider* b) {
    if (!a || !b) return false;
    bool aBox = a->type == ColliderType::Box;
    bool bBox = b->type == ColliderType::Box;
    if (aBox && bBox)
        return CheckCollisionRecs(static_cast<BoxCollider*>(a)->GetBounds(), static_cast<BoxCollider*>(b)->GetBounds());
    if (!aBox && !bBox) {
        auto* ca = static_cast<CircleCollider*>(a);
        auto* cb = static_cast<CircleCollider*>(b);
        return CheckCollisionCircles(ca->GetWorldPosition(), ca->GetWorldRadius(), cb->GetWorldPosition(), cb->GetWorldRadius());
    }
    auto* box = static_cast<BoxCollider*>(aBox ? a : b);
    auto* circle = static_cast<CircleCollider*>(aBox ? b : a);
    return CheckCollisionCircleRec(circle->GetWorldPosition(), circle->GetWorldRadius(), box->GetBounds());
}

class RAYGAME_API Rigidbody : public Component {
public:
    Vector2 velocity = {0, 0};
    Vector2 acceleration = {0, 0};
    float mass = 1.0f;
    float drag = 0.99f;           // retained velocity per 1/60 second
    float gravity = 0.0f;
    bool useGravity = false;
    bool isKinematic = false;
    float restitution = 0.8f;

    float InverseMass() const {
        return IsEnabled() && !isKinematic && std::isfinite(mass) && mass > 0 ? 1.0f / mass : 0.0f;
    }

    void AddForce(Vector2 force) {
        acceleration.x += force.x * InverseMass();
        acceleration.y += force.y * InverseMass();
    }

    void AddImpulse(Vector2 impulse) {
        velocity.x += impulse.x * InverseMass();
        velocity.y += impulse.y * InverseMass();
    }

    void FixedUpdate() override {
        if (!gameObject || InverseMass() == 0) { acceleration = {0, 0}; return; }
        float dt = GameTime::FixedDeltaTime();
        velocity.x += acceleration.x * dt;
        velocity.y += (acceleration.y + (useGravity ? gravity : 0.0f)) * dt;
        float damping = std::pow(Clamp(drag, 0.0f, 1.0f), dt * 60.0f);
        velocity.x *= damping;
        velocity.y *= damping;
        Vector2 pos = gameObject->transform->GetWorldPosition();
        gameObject->transform->SetWorldPosition({pos.x + velocity.x * dt, pos.y + velocity.y * dt});
        acceleration = {0, 0};
    }

    void SetVelocity(Vector2 vel) { velocity = vel; }
    Vector2 GetVelocity() const { return velocity; }
    float GetSpeed() const { return std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y); }
};

class RAYGAME_API PhysicsWorld {
public:
    static void AddCollider(Collider* collider) {
        if (!collider || !collider->GetGameObject() || registered.count(collider)) return;
        for (const auto& comp : collider->GetGameObject()->GetAllComponents()) {
            if (comp.get() == collider) {
                colliders.push_back(std::static_pointer_cast<Collider>(comp));
                registered.insert(collider);
                break;
            }
        }
    }

    static void RemoveCollider(Collider* collider) {
        registered.erase(collider);
        colliders.erase(std::remove_if(colliders.begin(), colliders.end(),
            [collider](const Ref<Collider>& c) { return c.get() == collider; }), colliders.end());
        // Keep contact references until Step sends Exit to surviving objects.
    }

    // Detection/resolution runs after each fixed-timestep integration.
    static void Step() {
        if (stepping) return;
        stepping = true;
        struct StepScope { ~StepScope() { stepping = false; } } scope;
        const auto snapshot = colliders;
        const auto previous = activePairs;
        const auto generation = clearGeneration;
        std::map<Key, Contact, KeyLess> current;
        for (size_t i = 0; i < snapshot.size(); ++i) {
            for (size_t j = i + 1; j < snapshot.size(); ++j) {
                auto a = snapshot[i];
                auto b = snapshot[j];
                if (!IsActive(a.get()) || !IsActive(b.get())) continue;
                if (a->GetGameObject() == b->GetGameObject()) continue;
                if (a->layer < 0 || a->layer >= 32 || b->layer < 0 || b->layer >= 32) continue;
                if (((a->mask >> b->layer) & 1u) == 0 || ((b->mask >> a->layer) & 1u) == 0) continue;
                if (!Collide(a.get(), b.get())) continue;
                auto key = MakeKey(a.get(), b.get());
                Contact contact{a, b, a->isTrigger || b->isTrigger};
                auto old = previous.find(key);
                bool existed = old != previous.end() && old->second.trigger == contact.trigger;
                if (old != previous.end() && !existed) Dispatch(old->second, Phase::Exit);
                if (generation != clearGeneration) { stepping = false; return; }
                if (!IsActive(a.get()) || !IsActive(b.get())) continue;
                if (!contact.trigger) ResolveCollision(a.get(), b.get());
                current.emplace(key, contact);
                Dispatch(contact, existed ? Phase::Stay : Phase::Enter);
                if (generation != clearGeneration) { stepping = false; return; }
            }
        }
        for (const auto& entry : previous) {
            if (!current.count(entry.first)) Dispatch(entry.second, Phase::Exit);
            if (generation != clearGeneration) { stepping = false; return; }
        }
        activePairs.swap(current);
        stepping = false;
    }

    static void CheckCollisions() { Step(); }
    static void DebugDraw() { for (const auto& c : colliders) if (IsActive(c.get())) c->DebugDraw(); }
    static void Clear() {
        ++clearGeneration;
        registered.clear();
        colliders.clear();
        activePairs.clear();
    }
    static int GetColliderCount() { return (int)colliders.size(); }

private:
    enum class Phase { Enter, Stay, Exit };
    using Key = std::pair<Collider*, Collider*>;
    struct KeyLess {
        bool operator()(const Key& a, const Key& b) const {
            std::less<Collider*> less;
            return a.first == b.first ? less(a.second, b.second) : less(a.first, b.first);
        }
    };
    struct Contact { Ref<Collider> a, b; bool trigger; };
    struct Manifold { Vector2 normal; float depth; };
    static inline std::vector<Ref<Collider>> colliders;
    static inline std::unordered_set<Collider*> registered;
    static inline std::map<Key, Contact, KeyLess> activePairs;
    static inline unsigned long clearGeneration = 0;
    static inline bool stepping = false;

    static Key MakeKey(Collider* a, Collider* b) {
        return std::less<Collider*>{}(a, b) ? Key{a, b} : Key{b, a};
    }
    static bool IsActive(Collider* c) {
        return registered.count(c) && c->IsEnabled() && c->GetGameObject()
            && c->GetGameObject()->active && !c->GetGameObject()->IsDestroyed();
    }
    static void Dispatch(const Contact& c, Phase phase) {
        const auto generation = clearGeneration;
        NotifyAll(c.a.get(), c.b.get(), c.trigger, phase);
        if (generation == clearGeneration) NotifyAll(c.b.get(), c.a.get(), c.trigger, phase);
    }
    static void NotifyAll(Collider* self, Collider* other, bool trigger, Phase phase) {
        auto* obj = self->GetGameObject();
        if (!obj || !obj->active || obj->IsDestroyed() || !self->IsEnabled()) return;
        const auto components = obj->GetAllComponents();
        const auto generation = clearGeneration;
        for (const auto& comp : components) {
            if (generation != clearGeneration || !self->GetGameObject()) break;
            if (!obj->active || obj->IsDestroyed() || !self->IsEnabled()) break;
            if (!comp->IsEnabled() || !comp->HasStarted()) continue;
            if (trigger) {
                if (phase == Phase::Enter) comp->OnTriggerEnter(other);
                else if (phase == Phase::Stay) comp->OnTriggerStay(other);
                else comp->OnTriggerExit(other);
            } else {
                if (phase == Phase::Enter) comp->OnCollisionEnter(other);
                else if (phase == Phase::Stay) comp->OnCollisionStay(other);
                else comp->OnCollisionExit(other);
            }
        }
    }

    // Surface normal points from a toward b; boxes remain axis aligned.
    static Manifold GetManifold(Collider* a, Collider* b) {
        Vector2 pa = a->GetWorldPosition(), pb = b->GetWorldPosition();
        if (a->type == ColliderType::Circle && b->type == ColliderType::Circle) {
            Vector2 delta = {pb.x - pa.x, pb.y - pa.y};
            float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            float radius = static_cast<CircleCollider*>(a)->GetWorldRadius() + static_cast<CircleCollider*>(b)->GetWorldRadius();
            return {distance > 0 ? Vector2{delta.x / distance, delta.y / distance} : Vector2{1, 0}, radius - distance};
        }
        if (a->type == ColliderType::Box && b->type == ColliderType::Box) {
            auto ra = static_cast<BoxCollider*>(a)->GetBounds(), rb = static_cast<BoxCollider*>(b)->GetBounds();
            float dx = (ra.width + rb.width) / 2 - std::fabs(pb.x - pa.x);
            float dy = (ra.height + rb.height) / 2 - std::fabs(pb.y - pa.y);
            if (dx < dy) return {{pb.x >= pa.x ? 1.0f : -1.0f, 0}, dx};
            return {{0, pb.y >= pa.y ? 1.0f : -1.0f}, dy};
        }
        bool aBox = a->type == ColliderType::Box;
        auto bounds = static_cast<BoxCollider*>(aBox ? a : b)->GetBounds();
        auto* circle = static_cast<CircleCollider*>(aBox ? b : a);
        auto pos = circle->GetWorldPosition();
        Vector2 delta = {pos.x - Clamp(pos.x, bounds.x, bounds.x + bounds.width),
                         pos.y - Clamp(pos.y, bounds.y, bounds.y + bounds.height)};
        float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        Manifold m;
        if (distance > 0) {
            m = {{delta.x / distance, delta.y / distance}, circle->GetWorldRadius() - distance};
        } else {
            float distances[] = {pos.x - bounds.x, bounds.x + bounds.width - pos.x,
                                 pos.y - bounds.y, bounds.y + bounds.height - pos.y};
            Vector2 normals[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            int closest = 0;
            for (int i = 1; i < 4; ++i) if (distances[i] < distances[closest]) closest = i;
            m = {normals[closest], circle->GetWorldRadius() + distances[closest]};
        }
        if (!aBox) m.normal = {-m.normal.x, -m.normal.y};
        return m;
    }

    static void ResolveCollision(Collider* a, Collider* b) {
        auto ra = a->GetGameObject()->GetComponent<Rigidbody>();
        auto rb = b->GetGameObject()->GetComponent<Rigidbody>();
        float invA = ra ? ra->InverseMass() : 0, invB = rb ? rb->InverseMass() : 0;
        if (invA + invB == 0) return;
        auto m = GetManifold(a, b);
        Vector2 va = ra && ra->IsEnabled() ? ra->velocity : Vector2{0, 0};
        Vector2 vb = rb && rb->IsEnabled() ? rb->velocity : Vector2{0, 0};
        float relative = (vb.x - va.x) * m.normal.x + (vb.y - va.y) * m.normal.y;
        if (relative < 0) {
            float e = Clamp(std::min(ra ? ra->restitution : 1.0f, rb ? rb->restitution : 1.0f), 0.0f, 1.0f);
            float impulse = -(1 + e) * relative / (invA + invB);
            if (invA > 0) { ra->velocity.x -= m.normal.x * impulse * invA; ra->velocity.y -= m.normal.y * impulse * invA; }
            if (invB > 0) { rb->velocity.x += m.normal.x * impulse * invB; rb->velocity.y += m.normal.y * impulse * invB; }
        }
        float correction = std::max(m.depth - 0.001f, 0.0f) / (invA + invB);
        auto move = [&](Collider* c, float amount) {
            auto tf = c->GetGameObject()->transform;
            auto p = tf->GetWorldPosition();
            tf->SetWorldPosition({p.x + m.normal.x * amount, p.y + m.normal.y * amount});
        };
        if (invA > 0) move(a, -correction * invA);
        if (invB > 0) move(b, correction * invB);
    }
};

inline bool Collider::CheckCollision(Collider* other) { return Collide(this, other); }
inline void Collider::Start() { if (IsEnabled()) PhysicsWorld::AddCollider(this); }
inline void Collider::OnDestroy() { PhysicsWorld::RemoveCollider(this); }
inline void Collider::OnEnable() { if (HasStarted()) PhysicsWorld::AddCollider(this); }
inline void Collider::OnDisable() { PhysicsWorld::RemoveCollider(this); }

}
