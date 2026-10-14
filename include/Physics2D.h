#pragma once
#include "Scene.h"

#ifdef RAYGAME_WITH_BOX2D
namespace RayGame {
// Opaque, non-recycled IDs. Stale and foreign-world handles cannot alias live objects.
struct Body2D {
    uint64_t id = 0;
    explicit operator bool() const {
        return id != 0;
    }
};
struct Joint2D {
    uint64_t id = 0;
    explicit operator bool() const {
        return id != 0;
    }
};
enum class Motion2D { Static, Dynamic, Kinematic };
struct Body2DOptions {
    Motion2D motion = Motion2D::Dynamic;
    float mass = 1, friction = 0.6f, restitution = 0.05f, damping = 0.15f;
    bool fixedRotation = false, bullet = false;
    uint16_t category = 1, mask = 0xffff;
};
struct Spring2DOptions {
    float length = 0; // pixels; zero selects the current distance
    float frequency = 5, dampingRatio = 0.7f;
    bool collideConnected = false;
};

// Pixel positions/lengths/velocities, radians, kilograms, and SI forces/impulses.
// One simulation owns each body. No Rigidbody/Collider is needed for these handles.
class RAYGAME_API Physics2DWorld {
public:
    explicit Physics2DWorld(Vector2 gravity = {0, 600}, float pixelsPerMeter = 50);
    ~Physics2DWorld();
    Physics2DWorld(const Physics2DWorld&) = delete;
    Physics2DWorld& operator=(const Physics2DWorld&) = delete;
    Body2D Circle(Vector2 position, float radius, const Body2DOptions& options = {});
    Body2D Box(Vector2 position, Vector2 size, const Body2DOptions& options = {}, float angle = 0);
    Joint2D DistanceJoint(Body2D a, Body2D b, const Spring2DOptions& options = {});
    Joint2D DragJoint(Body2D body, Vector2 target, float maxForce = 1000, float frequency = 5);
    void SetDragTarget(Joint2D joint, Vector2 target);
    bool Destroy(Body2D body);
    bool Destroy(Joint2D joint);
    bool Valid(Body2D body) const;
    bool Valid(Joint2D joint) const;
    void Clear();
    void SetGravity(Vector2 gravity);
    void Step(float dt, int substeps = 4);
    Vector2 Position(Body2D body, float alpha = 1) const;
    float Angle(Body2D body, float alpha = 1) const;
    Vector2 Velocity(Body2D body) const;
    void SetVelocity(Body2D body, Vector2 velocity);
    void Teleport(Body2D body, Vector2 position, float angle = 0);
    void ApplyForce(Body2D body, Vector2 newtons);
    void ApplyImpulse(Body2D body, Vector2 newtonSeconds);
    float JointForce(Joint2D joint) const;
    size_t BodyCount() const;
    size_t JointCount() const;
    void DebugDraw() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

// Scene-owned optional backend, stepped between FixedUpdate and PostFixedUpdate.
// Destruction/unload releases every body and joint even when components keep handles.
class RAYGAME_API PhysicsScene2D : public Scene {
public:
    Physics2DWorld dynamics;
    explicit PhysicsScene2D(const std::string& name, Vector2 gravity = {0, 600})
        : Scene(name), dynamics(gravity) {}
    ~PhysicsScene2D() override {
        Clear();
    }
    void IntegratePhysics(float dt) override {
        Scene::IntegratePhysics(dt);
        dynamics.Step(dt);
    }
    void Clear() override {
        dynamics.Clear();
        Scene::Clear();
    }
};
} // namespace RayGame
#endif
