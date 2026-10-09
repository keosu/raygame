#pragma once

#include "Core.h"

namespace RayGame {

class Collider;

class RAYGAME_API Component {
public:
    Component() : gameObject(nullptr), enabled(true), started(false) {}
    virtual ~Component() = default;

    // Lifecycle
    virtual void Awake() {}
    virtual void Start() {}
    virtual void Update() {}
    virtual void FixedUpdate() {}
    virtual void Render() {}
    virtual void OnDestroy() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}

    // Physics callbacks (fired by PhysicsWorld)
    virtual void OnCollisionEnter(Collider* other) {}
    virtual void OnCollisionStay(Collider* other) {}
    virtual void OnCollisionExit(Collider* other) {}
    virtual void OnTriggerEnter(Collider* other) {}
    virtual void OnTriggerStay(Collider* other) {}
    virtual void OnTriggerExit(Collider* other) {}

    // Determines draw order; lower draws first. Renderers override.
    virtual int RenderLayer() const { return renderOrder; }

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool value) {
        if (enabled == value) return;
        enabled = value;
        if (enabled) OnEnable();
        else OnDisable();
    }

    bool HasStarted() const { return started; }

    GameObject* GetGameObject() const { return gameObject; }

    int renderOrder = 0;

protected:
    friend class GameObject;
    GameObject* gameObject;
    bool enabled;
    bool started;
};

class RAYGAME_API Transform : public Component {
public:
    Vector2 position = {0, 0};   // local position (world if no parent)
    float rotation = 0.0f;       // local rotation in degrees
    Vector2 scale = {1, 1};      // local scale

    Transform() = default;
    Transform(Vector2 pos) : position(pos) {}
    Transform(Vector2 pos, float rot, Vector2 scl)
        : position(pos), rotation(rot), scale(scl) {}

    void Translate(Vector2 offset) {
        position.x += offset.x;
        position.y += offset.y;
    }

    void Rotate(float degrees) {
        rotation += degrees;
    }

    void ScaleBy(Vector2 factor) {
        scale.x *= factor.x;
        scale.y *= factor.y;
    }

    void Scale(Vector2 factor) { ScaleBy(factor); }

    Vector2 Forward() const {
        float rad = GetWorldRotation() * DEG2RAD;
        return {std::cos(rad), std::sin(rad)};
    }

    Vector2 Right() const {
        float rad = (GetWorldRotation() + 90) * DEG2RAD;
        return {std::cos(rad), std::sin(rad)};
    }

    // ---- Hierarchy ----
    Transform* GetParent() const { return parent; }
    const std::vector<Transform*>& GetChildren() const { return children; }

    void SetParent(Transform* newParent, bool keepWorld = true) {
        if (newParent == this || newParent == parent) return;
        for (auto* ancestor = newParent; ancestor; ancestor = ancestor->parent) {
            if (ancestor == this) return;
        }

        Vector2 worldPos = GetWorldPosition();
        float worldRot = GetWorldRotation();
        Vector2 worldScl = GetWorldScale();

        if (parent) {
            auto& sib = parent->children;
            sib.erase(std::remove(sib.begin(), sib.end(), this), sib.end());
        }
        parent = newParent;
        if (parent) parent->children.push_back(this);

        if (keepWorld) {
            SetWorldPosition(worldPos);
            SetWorldRotation(worldRot);
            SetWorldScale(worldScl);
        }
    }

    Vector2 GetWorldPosition() const {
        return parent ? parent->TransformPoint(position) : position;
    }

    Vector2 TransformPoint(Vector2 local) const {
        float rad = rotation * DEG2RAD;
        Vector2 scaled = {local.x * scale.x, local.y * scale.y};
        Vector2 point = {position.x + scaled.x * std::cos(rad) - scaled.y * std::sin(rad),
                         position.y + scaled.x * std::sin(rad) + scaled.y * std::cos(rad)};
        return parent ? parent->TransformPoint(point) : point;
    }

    Vector2 InverseTransformPoint(Vector2 world) const {
        Vector2 point = parent ? parent->InverseTransformPoint(world) : world;
        point = {point.x - position.x, point.y - position.y};
        float rad = -rotation * DEG2RAD;
        Vector2 rotated = {point.x * std::cos(rad) - point.y * std::sin(rad),
                           point.x * std::sin(rad) + point.y * std::cos(rad)};
        return {scale.x != 0 ? rotated.x / scale.x : 0,
                scale.y != 0 ? rotated.y / scale.y : 0};
    }

    float GetWorldRotation() const {
        return parent ? parent->GetWorldRotation() + rotation : rotation;
    }

    Vector2 GetWorldScale() const {
        if (!parent) return scale;
        Vector2 pscl = parent->GetWorldScale();
        return {pscl.x * scale.x, pscl.y * scale.y};
    }

    void SetWorldPosition(Vector2 world) {
        position = parent ? parent->InverseTransformPoint(world) : world;
    }

    void SetWorldRotation(float world) {
        rotation = parent ? world - parent->GetWorldRotation() : world;
    }

    void SetWorldScale(Vector2 world) {
        if (!parent) { scale = world; return; }
        Vector2 ps = parent->GetWorldScale();
        scale = {ps.x != 0 ? world.x / ps.x : 0, ps.y != 0 ? world.y / ps.y : 0};
    }

    void DetachHierarchy() {
        while (!children.empty()) children.back()->SetParent(nullptr);
        SetParent(nullptr);
    }

    ~Transform() override { DetachHierarchy(); }

private:
    Transform* parent = nullptr;
    std::vector<Transform*> children;
};

}
