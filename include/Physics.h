#pragma once

#include "Core.h"
#include "Component.h"
#include "GameObject.h"
#include "GameTime.h"
#include <limits>
#include <map>
#include <unordered_set>

namespace RayGame {

class PhysicsWorld;

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
    void OnDisable() override {} // Keep the binding; inactive contacts exit on the next step.
    PhysicsWorld* GetPhysicsWorld() const { return world; }

private:
    friend class PhysicsWorld;
    PhysicsWorld* world = nullptr;

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
    bool interpolate = true;
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

    void Start() override { if (gameObject) gameObject->transform->SetInterpolationEnabled(interpolate); }

    // Called by the scene after all pre-physics FixedUpdate callbacks.
    void Integrate(float dt) {
        if (!gameObject || InverseMass() == 0) { acceleration = {0, 0}; return; }
        if (!std::isfinite(dt) || dt <= 0) return;
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

// Result of a segment or ray query. `fraction` is 0 at the start and 1 at the end.
struct SegmentHit {
    Collider* collider = nullptr;
    float fraction = 0;
    Vector2 point = {0, 0};
};

class RAYGAME_API PhysicsWorld {
public:
    explicit PhysicsWorld(Scene* owner = nullptr) : owner(owner) {}
    ~PhysicsWorld() { Clear(); }
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void AddCollider(Collider* collider);
    void RemoveCollider(Collider* collider);

    // Worlds with at least this many colliders prune pairs by their bounds before the
    // narrow test. Pairs are still processed in registration order, so callbacks run
    // in the same order; a pair that only starts overlapping because another contact
    // was resolved earlier in the same step is detected on the next step.
    void SetBroadphaseThreshold(size_t colliderCount) { broadphaseThreshold = colliderCount; }
    size_t GetBroadphaseThreshold() const { return broadphaseThreshold; }

    // Detection/resolution runs after each fixed-timestep integration.
    void Step() {
        if (stepping) return;
        stepping = true;
        struct StepScope { PhysicsWorld& world; ~StepScope() { world.stepping = false; } } scope{*this};
        const auto snapshot = colliders;
        const auto previous = activePairs;
        const auto generation = clearGeneration;
        std::map<Key, Contact, KeyLess> current;
        // Returns false when a callback cleared the world and the step must stop.
        auto visit = [&](size_t i, size_t j) {
            auto a = snapshot[i];
            auto b = snapshot[j];
            if (!IsActive(a.get()) || !IsActive(b.get())) return true;
            if (a->GetGameObject() == b->GetGameObject()) return true;
            if (a->layer < 0 || a->layer >= 32 || b->layer < 0 || b->layer >= 32) return true;
            if (((a->mask >> b->layer) & 1u) == 0 || ((b->mask >> a->layer) & 1u) == 0) return true;
            if (!Collide(a.get(), b.get())) return true;
            auto key = MakeKey(a.get(), b.get());
            Contact contact{a, b, a->isTrigger || b->isTrigger};
            auto old = previous.find(key);
            bool existed = old != previous.end() && old->second.trigger == contact.trigger;
            if (old != previous.end() && !existed) Dispatch(old->second, Phase::Exit);
            if (generation != clearGeneration) return false;
            if (!IsActive(a.get()) || !IsActive(b.get())) return true;
            if (!contact.trigger) ResolveCollision(a.get(), b.get());
            current.emplace(key, contact);
            Dispatch(contact, existed ? Phase::Stay : Phase::Enter);
            return generation == clearGeneration;
        };
        if (snapshot.size() < broadphaseThreshold) {
            for (size_t i = 0; i < snapshot.size(); ++i)
                for (size_t j = i + 1; j < snapshot.size(); ++j)
                    if (!visit(i, j)) { stepping = false; return; }
        } else {
            for (const auto& pair : CandidatePairs(snapshot))
                if (!visit(pair.first, pair.second)) { stepping = false; return; }
        }
        for (const auto& entry : previous) {
            if (!current.count(entry.first)) Dispatch(entry.second, Phase::Exit);
            if (generation != clearGeneration) { stepping = false; return; }
        }
        activePairs.swap(current);
        stepping = false;
    }

    void CheckCollisions() { Step(); }
    void DebugDraw() { for (const auto& c : colliders) if (IsActive(c.get())) c->DebugDraw(); }
    void Clear() {
        ++clearGeneration;
        for (const auto& collider : colliders) if (collider->world == this) collider->world = nullptr;
        registered.clear();
        colliders.clear();
        activePairs.clear();
    }
    int GetColliderCount() const {
        return (int)std::count_if(colliders.begin(), colliders.end(),
            [](const Ref<Collider>& c) { return c->IsEnabled(); });
    }
    size_t GetContactCount() const { return activePairs.size(); }

    // Queries read current poses and never dispatch callbacks or resolve contacts.
    // `layerMask` selects collider layers (bit n = layer n). Results are in registration
    // order; the pointers stay valid until the colliders are removed or destroyed.
    std::vector<Collider*> OverlapCircle(Vector2 center, float radius, unsigned int layerMask = 0xFFFFFFFFu,
                                         bool includeTriggers = true) const {
        std::vector<Collider*> result;
        if (!std::isfinite(radius) || radius < 0) return result;
        for (const auto& c : colliders) {
            if (!Selectable(c.get(), layerMask, includeTriggers)) continue;
            bool hit = c->type == ColliderType::Circle
                ? CheckCollisionCircles(center, radius, c->GetWorldPosition(),
                                        static_cast<CircleCollider*>(c.get())->GetWorldRadius())
                : CheckCollisionCircleRec(center, radius, static_cast<BoxCollider*>(c.get())->GetBounds());
            if (hit) result.push_back(c.get());
        }
        return result;
    }

    std::vector<Collider*> OverlapBox(Rectangle area, unsigned int layerMask = 0xFFFFFFFFu,
                                      bool includeTriggers = true) const {
        std::vector<Collider*> result;
        for (const auto& c : colliders) {
            if (!Selectable(c.get(), layerMask, includeTriggers)) continue;
            bool hit = c->type == ColliderType::Box
                ? CheckCollisionRecs(area, static_cast<BoxCollider*>(c.get())->GetBounds())
                : CheckCollisionCircleRec(c->GetWorldPosition(),
                                          static_cast<CircleCollider*>(c.get())->GetWorldRadius(), area);
            if (hit) result.push_back(c.get());
        }
        return result;
    }

    // Sweeps a circle of `radius` (0 = thin line) from `from` to `to`; hits are sorted
    // nearest first. Boxes are expanded by the radius with square corners.
    std::vector<SegmentHit> SegmentCast(Vector2 from, Vector2 to, float radius = 0,
                                        unsigned int layerMask = 0xFFFFFFFFu, bool includeTriggers = true) const {
        std::vector<SegmentHit> result;
        if (!std::isfinite(radius) || radius < 0) return result;
        Vector2 d = {to.x - from.x, to.y - from.y};
        for (const auto& c : colliders) {
            if (!Selectable(c.get(), layerMask, includeTriggers)) continue;
            float t = -1;
            if (c->type == ColliderType::Circle) {
                t = SegmentCircle(from, d, c->GetWorldPosition(),
                                  static_cast<CircleCollider*>(c.get())->GetWorldRadius() + radius);
            } else {
                Rectangle b = static_cast<BoxCollider*>(c.get())->GetBounds();
                t = SegmentBox(from, d, {b.x - radius, b.y - radius, b.width + radius * 2, b.height + radius * 2});
            }
            if (t >= 0) result.push_back({c.get(), t, {from.x + d.x * t, from.y + d.y * t}});
        }
        std::stable_sort(result.begin(), result.end(),
                         [](const SegmentHit& a, const SegmentHit& b) { return a.fraction < b.fraction; });
        return result;
    }

    // Nearest hit along `direction` within `maxDistance`; the fraction is relative to maxDistance.
    bool Raycast(Vector2 origin, Vector2 direction, float maxDistance, SegmentHit* hit = nullptr,
                 unsigned int layerMask = 0xFFFFFFFFu, bool includeTriggers = true) const {
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (!(length > 0) || !std::isfinite(maxDistance) || maxDistance < 0) return false;
        Vector2 to = {origin.x + direction.x / length * maxDistance, origin.y + direction.y / length * maxDistance};
        auto hits = SegmentCast(origin, to, 0, layerMask, includeTriggers);
        if (hits.empty()) return false;
        if (hit) *hit = hits.front();
        return true;
    }

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
    std::vector<Ref<Collider>> colliders;
    std::unordered_set<Collider*> registered;
    std::map<Key, Contact, KeyLess> activePairs;
    unsigned long clearGeneration = 0;
    bool stepping = false;
    size_t broadphaseThreshold = 48;
    Scene* owner = nullptr;

    static Key MakeKey(Collider* a, Collider* b) {
        return std::less<Collider*>{}(a, b) ? Key{a, b} : Key{b, a};
    }
    bool IsActive(Collider* c) const {
        return registered.count(c) && c->IsEnabled() && c->GetGameObject()
            && c->GetGameObject()->active && !c->GetGameObject()->IsDestroyed();
    }
    bool Selectable(Collider* c, unsigned int layerMask, bool includeTriggers) const {
        return IsActive(c) && c->layer >= 0 && c->layer < 32 && ((layerMask >> c->layer) & 1u)
            && (includeTriggers || !c->isTrigger);
    }

    static Rectangle Bounds(Collider* c) {
        if (c->type == ColliderType::Box) return static_cast<BoxCollider*>(c)->GetBounds();
        Vector2 p = c->GetWorldPosition();
        float r = static_cast<CircleCollider*>(c)->GetWorldRadius();
        return {p.x - r, p.y - r, r * 2, r * 2};
    }

    // Sweep and prune on x, then restore registration order for deterministic dispatch.
    std::vector<std::pair<size_t, size_t>> CandidatePairs(const std::vector<Ref<Collider>>& snapshot) const {
        struct Entry { size_t index; float minX, maxX, minY, maxY; };
        std::vector<Entry> entries;
        entries.reserve(snapshot.size());
        for (size_t i = 0; i < snapshot.size(); ++i) {
            if (!IsActive(snapshot[i].get())) continue;
            Rectangle b = Bounds(snapshot[i].get());
            if (!std::isfinite(b.x) || !std::isfinite(b.y) || !std::isfinite(b.width) || !std::isfinite(b.height)) {
                // Degenerate bounds cannot be pruned safely; test them against everything.
                const float inf = std::numeric_limits<float>::infinity();
                entries.push_back({i, -inf, inf, -inf, inf});
                continue;
            }
            entries.push_back({i, b.x, b.x + b.width, b.y, b.y + b.height});
        }
        std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
            return a.minX != b.minX ? a.minX < b.minX : a.index < b.index;
        });
        std::vector<std::pair<size_t, size_t>> pairs;
        for (size_t i = 0; i < entries.size(); ++i) {
            for (size_t j = i + 1; j < entries.size() && entries[j].minX <= entries[i].maxX; ++j) {
                if (entries[j].minY > entries[i].maxY || entries[i].minY > entries[j].maxY) continue;
                size_t a = entries[i].index, b = entries[j].index;
                pairs.push_back(a < b ? std::make_pair(a, b) : std::make_pair(b, a));
            }
        }
        std::sort(pairs.begin(), pairs.end());
        return pairs;
    }

    // Earliest t in [0, 1] where from + d*t is within `radius` of `center`, or -1.
    static float SegmentCircle(Vector2 from, Vector2 d, Vector2 center, float radius) {
        Vector2 f = {from.x - center.x, from.y - center.y};
        float c = f.x * f.x + f.y * f.y - radius * radius;
        if (c <= 0) return 0;
        float a = d.x * d.x + d.y * d.y;
        if (a <= 0) return -1;
        float b = 2 * (f.x * d.x + f.y * d.y);
        float disc = b * b - 4 * a * c;
        if (disc < 0) return -1;
        float t = (-b - std::sqrt(disc)) / (2 * a);
        return t >= 0 && t <= 1 ? t : -1;
    }

    static float SegmentBox(Vector2 from, Vector2 d, Rectangle box) {
        float tMin = 0, tMax = 1;
        const float origin[2] = {from.x, from.y}, delta[2] = {d.x, d.y};
        const float lo[2] = {box.x, box.y}, hi[2] = {box.x + box.width, box.y + box.height};
        for (int axis = 0; axis < 2; ++axis) {
            if (std::fabs(delta[axis]) < 1e-12f) {
                if (origin[axis] < lo[axis] || origin[axis] > hi[axis]) return -1;
                continue;
            }
            float t1 = (lo[axis] - origin[axis]) / delta[axis], t2 = (hi[axis] - origin[axis]) / delta[axis];
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) return -1;
        }
        return tMin;
    }
    void Dispatch(const Contact& c, Phase phase) {
        const auto generation = clearGeneration;
        NotifyAll(c.a.get(), c.b.get(), c.trigger, phase);
        if (generation == clearGeneration) NotifyAll(c.b.get(), c.a.get(), c.trigger, phase);
    }
    void NotifyAll(Collider* self, Collider* other, bool trigger, Phase phase) {
        if (self->world && self->world != this) return;
        auto* obj = self->GetGameObject();
        if (!obj || !obj->active || obj->IsDestroyed() || !self->IsEnabled()) return;
        const auto components = obj->GetAllComponents();
        const auto generation = clearGeneration;
        for (const auto& comp : components) {
            if (generation != clearGeneration || !self->GetGameObject()) break;
            if (!obj->active || obj->IsDestroyed() || !self->IsEnabled()) break;
            if (!comp->IsEnabled() || !comp->HasStarted()) continue;
            ScopeActivation scope(comp->GetCallbackScope());
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

}
