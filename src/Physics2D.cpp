#include "Physics2D.h"
#ifdef RAYGAME_WITH_BOX2D
#include <box2d/box2d.h>
#include <atomic>
#include <map>

namespace RayGame {
namespace {
std::atomic<uint64_t> nextId{1};
bool Finite(Vector2 v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}
void Require(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
} // namespace
struct Physics2DWorld::Impl {
    struct Body {
        b2Body* value;
        Vector2 previous;
        float angle;
    };
    struct Joint {
        b2Joint* value;
        uint64_t a, b;
    };
    b2World world;
    float scale, inverseStep = 0;
    b2Body* ground;
    std::map<uint64_t, Body> bodies;
    std::map<uint64_t, Joint> joints;
    Impl(Vector2 gravity, float pixels)
        : world({gravity.x / pixels, gravity.y / pixels}), scale(pixels) {
        b2BodyDef def;
        ground = world.CreateBody(&def);
    }
    b2Vec2 To(Vector2 v) const {
        return {v.x / scale, v.y / scale};
    }
    Vector2 From(b2Vec2 v) const {
        return {v.x * scale, v.y * scale};
    }
    Body& Get(Body2D h) {
        auto found = bodies.find(h.id);
        if (found == bodies.end())
            throw std::invalid_argument("Invalid or foreign Body2D");
        return found->second;
    }
    Joint& Get(Joint2D h) {
        auto found = joints.find(h.id);
        if (found == joints.end())
            throw std::invalid_argument("Invalid or foreign Joint2D");
        return found->second;
    }
    Body2D Create(Vector2 p, float angle, const b2Shape& shape, float area,
                  const Body2DOptions& opt) {
        Require(Finite(p) && std::isfinite(angle), "Non-finite body pose");
        Require(std::isfinite(opt.mass) && opt.mass > 0 && std::isfinite(opt.friction) &&
                    opt.friction >= 0 && std::isfinite(opt.restitution) && opt.restitution >= 0 &&
                    opt.restitution <= 1 && std::isfinite(opt.damping) && opt.damping >= 0,
                "Invalid body material");
        b2BodyDef def;
        def.type = opt.motion == Motion2D::Static      ? b2_staticBody
                   : opt.motion == Motion2D::Kinematic ? b2_kinematicBody
                                                       : b2_dynamicBody;
        def.position = To(p);
        def.angle = angle;
        def.linearDamping = opt.damping;
        def.angularDamping = opt.damping;
        def.fixedRotation = opt.fixedRotation;
        def.bullet = opt.bullet;
        auto* body = world.CreateBody(&def);
        b2FixtureDef fixture;
        fixture.shape = &shape;
        fixture.density = opt.mass / area;
        fixture.friction = opt.friction;
        fixture.restitution = opt.restitution;
        fixture.filter.categoryBits = opt.category;
        fixture.filter.maskBits = opt.mask;
        body->CreateFixture(&fixture);
        uint64_t id = nextId++;
        bodies.emplace(id, Body{body, p, angle});
        return {id};
    }
};
Physics2DWorld::Physics2DWorld(Vector2 gravity, float scale) {
    Require(Finite(gravity) && std::isfinite(scale) && scale > 0,
            "Invalid physics world units/gravity");
    impl = std::make_unique<Impl>(gravity, scale);
}
Physics2DWorld::~Physics2DWorld() = default;
Body2D Physics2DWorld::Circle(Vector2 p, float radius, const Body2DOptions& opt) {
    Require(std::isfinite(radius) && radius > 0, "Invalid circle radius");
    b2CircleShape shape;
    shape.m_radius = radius / impl->scale;
    return impl->Create(p, 0, shape, b2_pi * shape.m_radius * shape.m_radius, opt);
}
Body2D Physics2DWorld::Box(Vector2 p, Vector2 size, const Body2DOptions& opt, float angle) {
    Require(Finite(size) && size.x > 0 && size.y > 0, "Invalid box size");
    b2PolygonShape shape;
    shape.SetAsBox(size.x / (2 * impl->scale), size.y / (2 * impl->scale));
    return impl->Create(p, angle, shape, size.x * size.y / (impl->scale * impl->scale), opt);
}
Joint2D Physics2DWorld::DistanceJoint(Body2D a, Body2D b, const Spring2DOptions& opt) {
    auto* ba = impl->Get(a).value;
    auto* bb = impl->Get(b).value;
    Require(a.id != b.id && (ba->GetType() == b2_dynamicBody || bb->GetType() == b2_dynamicBody),
            "Joint needs distinct bodies and a dynamic body");
    Require(std::isfinite(opt.length) && opt.length >= 0 && std::isfinite(opt.frequency) &&
                opt.frequency >= 0 && std::isfinite(opt.dampingRatio) && opt.dampingRatio >= 0,
            "Invalid spring parameters");
    b2DistanceJointDef def;
    def.Initialize(ba, bb, ba->GetPosition(), bb->GetPosition());
    if (opt.length > 0)
        def.length = std::max(b2_linearSlop, opt.length / impl->scale);
    if (opt.frequency > 0) {
        def.minLength = b2_linearSlop;
        def.maxLength = FLT_MAX;
        b2LinearStiffness(def.stiffness, def.damping, opt.frequency, opt.dampingRatio, ba, bb);
    } else
        def.minLength = def.maxLength = def.length;
    def.collideConnected = opt.collideConnected;
    uint64_t id = nextId++;
    impl->joints.emplace(id, Impl::Joint{impl->world.CreateJoint(&def), a.id, b.id});
    return {id};
}
Joint2D Physics2DWorld::DragJoint(Body2D body, Vector2 target, float maxForce, float frequency) {
    auto* b = impl->Get(body).value;
    Require(b->GetType() == b2_dynamicBody && Finite(target) && std::isfinite(maxForce) &&
                maxForce > 0 && std::isfinite(frequency) && frequency > 0,
            "Invalid drag constraint");
    b2MouseJointDef def;
    def.bodyA = impl->ground;
    def.bodyB = b;
    def.target = impl->To(target);
    def.maxForce = maxForce;
    b2LinearStiffness(def.stiffness, def.damping, frequency, 0.85f, def.bodyA, b);
    uint64_t id = nextId++;
    impl->joints.emplace(id, Impl::Joint{impl->world.CreateJoint(&def), 0, body.id});
    b->SetAwake(true);
    return {id};
}
void Physics2DWorld::SetDragTarget(Joint2D h, Vector2 p) {
    auto* j = impl->Get(h).value;
    Require(j->GetType() == e_mouseJoint && Finite(p), "Invalid drag target or joint kind");
    static_cast<b2MouseJoint*>(j)->SetTarget(impl->To(p));
}
bool Physics2DWorld::Destroy(Joint2D h) {
    auto it = impl->joints.find(h.id);
    if (it == impl->joints.end())
        return false;
    auto* joint = it->second.value;
    joint->GetBodyA()->SetAwake(true);
    joint->GetBodyB()->SetAwake(true);
    impl->world.DestroyJoint(joint);
    impl->joints.erase(it);
    return true;
}
bool Physics2DWorld::Destroy(Body2D h) {
    auto it = impl->bodies.find(h.id);
    if (it == impl->bodies.end())
        return false;
    for (auto j = impl->joints.begin(); j != impl->joints.end();) {
        if (j->second.a == h.id || j->second.b == h.id) {
            auto doomed = j++;
            Destroy(Joint2D{doomed->first});
        } else
            ++j;
    }
    impl->world.DestroyBody(it->second.value);
    impl->bodies.erase(it);
    return true;
}
bool Physics2DWorld::Valid(Body2D h) const {
    return impl->bodies.count(h.id) != 0;
}
bool Physics2DWorld::Valid(Joint2D h) const {
    return impl->joints.count(h.id) != 0;
}
void Physics2DWorld::Clear() {
    while (!impl->bodies.empty())
        Destroy(Body2D{impl->bodies.begin()->first});
    impl->inverseStep = 0;
}
void Physics2DWorld::SetGravity(Vector2 g) {
    Require(Finite(g), "Invalid gravity");
    impl->world.SetGravity(impl->To(g));
    for (auto& entry : impl->bodies)
        entry.second.value->SetAwake(true);
}
void Physics2DWorld::Step(float dt, int substeps) {
    Require(std::isfinite(dt) && dt > 0 && dt <= 0.1f && substeps >= 1 && substeps <= 32,
            "Invalid physics timestep");
    for (auto& entry : impl->bodies) {
        entry.second.previous = impl->From(entry.second.value->GetPosition());
        entry.second.angle = entry.second.value->GetAngle();
    }
    impl->world.SetAutoClearForces(false);
    for (int i = 0; i < substeps; ++i)
        impl->world.Step(dt / substeps, 12, 6);
    impl->world.ClearForces();
    impl->inverseStep = substeps / dt;
}
Vector2 Physics2DWorld::Position(Body2D h, float alpha) const {
    auto& b = impl->Get(h);
    Require(std::isfinite(alpha), "Invalid interpolation");
    return RayGame::Lerp(b.previous, impl->From(b.value->GetPosition()), Clamp(alpha, 0.0f, 1.0f));
}
float Physics2DWorld::Angle(Body2D h, float alpha) const {
    auto& b = impl->Get(h);
    Require(std::isfinite(alpha), "Invalid interpolation");
    return ::Lerp(b.angle, b.value->GetAngle(), Clamp(alpha, 0.0f, 1.0f));
}
Vector2 Physics2DWorld::Velocity(Body2D h) const {
    return impl->From(impl->Get(h).value->GetLinearVelocity());
}
void Physics2DWorld::SetVelocity(Body2D h, Vector2 v) {
    Require(Finite(v), "Invalid velocity");
    impl->Get(h).value->SetLinearVelocity(impl->To(v));
}
void Physics2DWorld::Teleport(Body2D h, Vector2 p, float angle) {
    Require(Finite(p) && std::isfinite(angle), "Invalid pose");
    auto& b = impl->Get(h);
    b.value->SetTransform(impl->To(p), angle);
    b.value->SetAwake(true);
    b.previous = p;
    b.angle = angle;
}
void Physics2DWorld::ApplyForce(Body2D h, Vector2 f) {
    Require(Finite(f), "Invalid force");
    impl->Get(h).value->ApplyForceToCenter({f.x, f.y}, true);
}
void Physics2DWorld::ApplyImpulse(Body2D h, Vector2 f) {
    Require(Finite(f), "Invalid impulse");
    impl->Get(h).value->ApplyLinearImpulseToCenter({f.x, f.y}, true);
}
float Physics2DWorld::JointForce(Joint2D h) const {
    return impl->Get(h).value->GetReactionForce(impl->inverseStep).Length();
}
size_t Physics2DWorld::BodyCount() const {
    return impl->bodies.size();
}
size_t Physics2DWorld::JointCount() const {
    return impl->joints.size();
}
void Physics2DWorld::DebugDraw() const {
    for (const auto& entry : impl->joints) {
        auto* j = entry.second.value;
        DrawLineEx(impl->From(j->GetAnchorA()), impl->From(j->GetAnchorB()), 2, ORANGE);
    }
    for (const auto& entry : impl->bodies) {
        auto* b = entry.second.value;
        for (auto* f = b->GetFixtureList(); f; f = f->GetNext()) {
            if (f->GetType() == b2Shape::e_circle) {
                auto* c = static_cast<const b2CircleShape*>(f->GetShape());
                Vector2 p = impl->From(b->GetWorldPoint(c->m_p));
                DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y),
                                c->m_radius * impl->scale, GREEN);
            } else if (f->GetType() == b2Shape::e_polygon) {
                auto* p = static_cast<const b2PolygonShape*>(f->GetShape());
                for (int i = 0; i < p->m_count; ++i)
                    DrawLineEx(impl->From(b->GetWorldPoint(p->m_vertices[i])),
                               impl->From(b->GetWorldPoint(p->m_vertices[(i + 1) % p->m_count])), 2,
                               GREEN);
            }
        }
    }
}
} // namespace RayGame
#endif
