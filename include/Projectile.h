#pragma once

#include "Component.h"
#include "GameObject.h"
#include "GameTime.h"
#include "Physics.h"
#include "Scene.h"

namespace RayGame {

enum class ProjectileMotion {
    Straight,  // constant `velocity`
    Homing,    // chases `follow` (or flies to `target` once it is gone)
    Boomerang, // out to `target`, then back to where it was launched
    Lob        // falls from the launch point onto `target` over `flightTime`
};

// A moving hit-tester for bullets, thrown weapons and bombs. Hits use the scene's
// PhysicsWorld queries (no Rigidbody or contacts). Without `pierce` the first hit ends
// the flight; with it, every collider is hit at most once per leg. The object destroys
// itself on arrival, on a non-piercing hit, or after `maxLife` seconds.
class RAYGAME_API Projectile : public Component {
public:
    ProjectileMotion motion = ProjectileMotion::Straight;
    Vector2 velocity = {0, 0};
    float speed = 400;           // Homing and Boomerang
    Vector2 target = {0, 0};     // destination; Homing updates it from `follow`
    std::weak_ptr<GameObject> follow;
    Vector2 followOffset = {0, 0};
    float flightTime = 0.5f;     // Lob
    float hitRadius = 0;         // 0 disables overlap hits (e.g. arrive-only bombs)
    unsigned int layerMask = 0xFFFFFFFFu;
    bool includeTriggers = true;
    bool pierce = false;
    float maxLife = 5;

    std::function<void(Projectile&, Collider*)> onHit;
    std::function<void(Projectile&, Vector2)> onArrive;
    std::function<void(Projectile&)> onStep; // after each fixed-step move, e.g. to emit a trail
    std::function<void(const Projectile&)> onDraw;

    float Age() const { return age; }
    bool Returning() const { return returning; }
    Vector2 Origin() const { return origin; }
    Vector2 Position() const { return gameObject->transform->position; }
    Vector2 RenderPosition() const { return gameObject->transform->GetRenderPosition(); }

    void Start() override {
        origin = Position();
        gameObject->transform->SetInterpolationEnabled(true);
    }

    void FixedUpdate() override {
        if (finished) return;
        float dt = GameTime::DeltaTime();
        age += dt;
        Vector2 p = Position();
        switch (motion) {
        case ProjectileMotion::Straight:
            p = {p.x + velocity.x * dt, p.y + velocity.y * dt};
            break;
        case ProjectileMotion::Homing: {
            if (auto chased = follow.lock(); chased && !chased->IsDestroyed() && chased->active)
                target = {chased->transform->position.x + followOffset.x,
                          chased->transform->position.y + followOffset.y};
            if (MoveToward(p, target, speed * dt)) {
                Place(p);
                Arrive(p);
                return;
            }
            break;
        }
        case ProjectileMotion::Boomerang: {
            Vector2 goal = returning ? origin : target;
            if (MoveToward(p, goal, speed * dt)) {
                if (returning) {
                    Place(p);
                    Arrive(p);
                    return;
                }
                returning = true;
                hits.clear(); // the return leg can hit everything again
            }
            break;
        }
        case ProjectileMotion::Lob: {
            float t = flightTime > 0 ? Clamp(age / flightTime, 0.0f, 1.0f) : 1.0f;
            p = Lerp(origin, target, t * t);
            if (t >= 1) {
                Place(p);
                Arrive(p);
                return;
            }
            break;
        }
        }
        Place(p);
        if (hitRadius > 0) TestHits(p);
        if (!finished && onStep) onStep(*this);
        if (!finished && age >= maxLife) Finish();
    }

    void Render() override {
        if (onDraw) onDraw(*this);
    }

    // Ends the flight without an arrival callback, e.g. when a hit handler absorbs it.
    void Finish() {
        if (finished) return;
        finished = true;
        if (gameObject) gameObject->MarkDestroyed();
    }
    bool Finished() const { return finished; }

private:
    // A destroyed collider's address can be reused by a new one, so hits are remembered
    // through the owning component reference; a raw pointer is only kept for unowned colliders.
    struct Hit {
        std::weak_ptr<Component> owner;
        const Collider* collider;
        bool owned;
    };
    Vector2 origin = {0, 0};
    float age = 0;
    bool returning = false, finished = false;
    std::vector<Hit> hits;

    bool AlreadyHit(const Collider* c) const {
        for (const auto& h : hits) {
            if (h.collider != c) continue;
            if (!h.owned) return true;
            auto alive = h.owner.lock();
            if (alive && alive.get() == static_cast<const Component*>(c)) return true;
        }
        return false;
    }

    static Hit Remember(Collider* c) {
        if (GameObject* owner = c->GetGameObject())
            for (const auto& comp : owner->GetAllComponents())
                if (comp.get() == static_cast<Component*>(c)) return {comp, c, true};
        return {{}, c, false};
    }

    void Place(Vector2 p) { gameObject->transform->position = p; }

    static bool MoveToward(Vector2& p, Vector2 goal, float step) {
        float dx = goal.x - p.x, dy = goal.y - p.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (!std::isfinite(dist) || dist <= step) {
            p = goal;
            return true;
        }
        p = {p.x + dx / dist * step, p.y + dy / dist * step};
        return false;
    }

    void Arrive(Vector2 p) {
        if (hitRadius > 0) TestHits(p);
        if (finished) return;
        if (onArrive) onArrive(*this, p);
        Finish();
    }

    void TestHits(Vector2 p) {
        if (!gameObject || !gameObject->scene) return;
        for (Collider* c : gameObject->scene->GetPhysicsWorld().OverlapCircle(p, hitRadius, layerMask, includeTriggers)) {
            if (c->GetGameObject() == gameObject) continue;
            if (AlreadyHit(c)) continue;
            hits.push_back(Remember(c));
            if (onHit) onHit(*this, c);
            if (finished) return;
            if (!pierce) {
                Finish();
                return;
            }
        }
    }
};

} // namespace RayGame
