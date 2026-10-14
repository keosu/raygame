#include "../examples/goo_journey/Model.h"
#include "GameEngine.h"
#include <iostream>
#include <limits>
using namespace RayGame;
int failures = 0, checks = 0;
void Check(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
template <class F> void Reject(F f, const char* message) {
    bool rejected = false;
    try {
        f();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected, message);
}
int main(int argc, char** argv) {
    int buildTicks = argc > 1 ? std::clamp(std::atoi(argv[1]), 1, 300) : 60;
    Physics2DWorld world({0, 500}), other;
    Body2DOptions fixed;
    fixed.motion = Motion2D::Static;
    auto anchor = world.Circle({0, 0}, 10, fixed);
    auto ball = world.Circle({100, 0}, 10);
    auto joint = world.DistanceJoint(anchor, ball);
    for (int i = 0; i < 600; ++i)
        world.Step(1.0f / 60);
    Check(Distance(world.Position(ball), world.Position(anchor)) < 120,
          "spring supports a hanging mass");
    Check(world.JointForce(joint) > 5 && world.JointForce(joint) < 15,
          "reaction force uses substep duration and SI units");
    Reject([&] { other.Position(ball); }, "foreign body rejected");
    Reject([&] { world.DistanceJoint(ball, ball); }, "self joint rejected");
    Reject([&] { world.Circle({0, 0}, -1); }, "negative radius rejected");
    Reject([&] { world.Step(std::numeric_limits<float>::quiet_NaN()); },
           "non-finite timestep rejected");
    auto drag = world.DragJoint(ball, world.Position(ball));
    world.SetDragTarget(drag, {50, 50});
    for (int i = 0; i < 120; ++i)
        world.Step(1.0f / 60);
    Check(Distance(world.Position(ball), {50, 50}) < 30, "drag joint moves a constrained mass");
    world.Destroy(ball);
    Check(!world.Valid(drag) && !world.Valid(joint) && world.JointCount() == 0,
          "body deletion removes attached joints");
    auto replacement = world.Circle({0, 0}, 10);
    Check(!world.Valid(ball) && replacement.id != ball.id && !world.Destroy(ball),
          "stale handles never alias replacements");
    world.Clear();
    Check(world.BodyCount() == 0 && !world.Valid(anchor), "world clear invalidates all bodies");
    PhysicsScene2D scene("lifetime");
    auto sceneBall = scene.dynamics.Circle({10, 10}, 5);
    scene.Clear();
    Check(!scene.dynamics.Valid(sceneBall), "Scene Clear releases the optional world");
    Vector2 cadenceBaseline{};
    for (int fps : {30, 60, 144}) {
        struct HeadlessEngine : GameEngine {
            HeadlessEngine() : GameEngine("physics cadence", 1280, 720) {}
            using GameEngine::Tick;
        } engine;
        engine.SetDefaultShortcutsEnabled(false);
        auto current = CreateRef<PhysicsScene2D>("cadence", Vector2{0, 500});
        auto body = current->dynamics.Circle({0, 0}, 10);
        engine.LoadScene(current);
        for (int frame = 0; frame < fps * 2; ++frame) {
            Input::SetFrame({});
            engine.Tick(1.0f / fps);
        }
        auto p = current->dynamics.Position(body);
        if (fps == 30)
            cadenceBaseline = p;
        Check(Distance(p, cadenceBaseline) < 0.01f,
              "30/60/144 render cadence retains the same fixed simulation");
        engine.SetPaused(true);
        engine.Tick(0.1f);
        Check(Distance(p, current->dynamics.Position(body)) == 0,
              "pause freezes the optional physics world");
        auto another = CreateRef<PhysicsScene2D>("replacement");
        auto retained = another->dynamics.Circle({20, 20}, 5);
        engine.LoadScene(another);
        engine.Tick(0);
        Check(!current->dynamics.Valid(body) && another->dynamics.Valid(retained),
              "scene switch clears only the outgoing world");
    }
    Body2DOptions material;
    material.restitution = 0;
    material.friction = 0.9f;
    world.Box({0, 200}, {600, 30}, fixed);
    auto resting = world.Box({0, 0}, {30, 30}, material, 0.2f);
    world.SetVelocity(resting, {130, 0});
    for (int i = 0; i < 900; ++i)
        world.Step(1.0f / 60);
    Check(world.Position(resting).y < 186 && std::fabs(world.Velocity(resting).x) < 1,
          "rotated box lands with friction and sleeps");
    {
        Physics2DWorld physics;
        Goo::Model model(physics);
        model.BeginDrag(2, model.Pos(2));
        model.MoveDrag({1100, 200});
        for (int i = 0; i < 180; ++i)
            model.Step();
        Check(model.broken > 0, "sustained player drag can break overloaded connections");
        model.EndDrag();
        model.Reset(0);
        model.BeginDrag(2, model.Pos(2));
        model.MoveDrag({1100, 200});
        model.Step();
        model.EndDrag();
        Check(model.broken == 0,
              "a one-tick stress spike does not immediately fracture a connection");
        model.Reset(2);
        Check(!model.ClearSegment({520, 520}, {740, 520}),
              "construction rejects connections through the central cliff");
        Check(!model.Place({610, 500}), "nodes cannot be placed inside terrain");
    }
    for (int stage = 0; stage < 3; ++stage) {
        Physics2DWorld physics;
        Goo::Model model(physics);
        model.Reset(stage);
        Check(!model.Place({0, 0}) && !model.Place(model.Pos(2)),
              "invalid and overlapping placement rejected");
        Check(!model.Launch() && !model.Remove(0),
              "disconnected launch and anchor recycling rejected");
        std::vector<Vector2> solution;
        if (stage == 0)
            for (int i = 0; i < 11; ++i)
                solution.push_back({400.0f + i * 60, i % 2 == 0 ? 490.0f : 560.0f});
        if (stage == 1)
            solution = {{550, 460}, {690, 450}, {615, 380}, {685, 320}, {600, 260}};
        if (stage == 2)
            solution = {{380, 480}, {400, 580}, {470, 480}, {450, 395}, {535, 355}, {530, 440},
                        {610, 365}, {680, 420}, {740, 345}, {810, 420}, {870, 370}, {950, 420}};
        for (auto p : solution) {
            bool placed = model.Place(p);
            if (!placed)
                std::cerr << "stage " << stage << " placement " << p.x << "," << p.y
                          << " candidates=" << model.Candidates(p).size() << '\n';
            Check(placed, "solution uses ordinary placement commands");
            for (int i = 0; i < buildTicks; ++i)
                model.Step();
        }
        std::cout << "stage " << stage << " route=" << model.Route().size()
                  << " links=" << model.links.size() << " broken=" << model.broken << '\n';
        Check(model.Launch(), "constructed level has a traversable route");
        for (int i = 0; i < 7200 && !model.outcome; ++i)
            model.Step();
        std::cout << "stage " << stage << " rescued=" << model.rescued << " broken=" << model.broken
                  << " outcome=" << model.outcome << '\n';
        Check(model.outcome == 1 && model.rescued >= model.level.needed,
              "real weighted travelers complete the level");
        if (stage == 0) {
            model.Reset(0);
            for (auto p : solution)
                model.Place(p);
            Check(model.Launch(), "reconstructed route launches after reset");
            for (int i = 0; i < 150; ++i)
                model.Step();
            for (int n = 0; n < static_cast<int>(model.nodes.size()); ++n)
                model.Remove(n, false);
            for (int i = 0; i < 300 && !model.outcome; ++i)
                model.Step();
            Check(model.lost >= 3 && model.outcome == -1,
                  "severing occupied paths drops travelers and can lose the game");
        }
        model.Reset(stage);
        Check(model.reserve == model.level.budget && model.rescued == 0 && model.links.size() == 2,
              "retry restores fresh world and resource budget");
    }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
