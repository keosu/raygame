#pragma once
#include "Physics2D.h"
#include <queue>
#include <set>

namespace Goo {
using namespace RayGame;
inline constexpr float LinkRange = 155, Radius = 15;
struct Node {
    Body2D body;
    bool anchor = false, alive = true;
    float age = 0;
};
// Short-lived feedback the renderer draws; it has no effect on the simulation.
enum class Effect { Snap, Rescue, Splash, Recycle };
struct Burst {
    Effect kind;
    Vector2 position;
    float age = 0;
};
struct Link {
    int a, b;
    Joint2D joint;
    float rest, stress = 0, overload = 0;
};
struct Traveler {
    int from = 0, to = -1;
    float progress = 0;
    Vector2 position{};
    Body2D falling;
    bool done = false;
};
struct Level {
    const char* name;
    Vector2 left, lower, start, exit;
    std::vector<Rectangle> terrain;
    int budget, seeds, needed;
    float wind;
};
inline Level Stage(int index) {
    if (index == 1)
        return {"Up to the Lantern",   {560, 585}, {700, 585}, {630, 505}, {630, 205},
                {{435, 610, 410, 80}}, 15,         7,          6,          0};
    if (index == 2)
        return {"The Windmill Pass",
                {245, 490},
                {245, 590},
                {320, 540},
                {1040, 370},
                {{60, 470, 165, 220}, {575, 455, 95, 235}, {1045, 400, 185, 290}},
                23,
                8,
                6,
                1.4f};
    return {"Across the Quiet Brook",
            {255, 490},
            {255, 590},
            {330, 540},
            {1030, 490},
            {{60, 470, 175, 220}, {1045, 515, 185, 175}},
            20,
            7,
            5,
            0};
}
class Model {
public:
    Physics2DWorld& world;
    Level level = Stage(0);
    std::vector<Node> nodes;
    std::vector<Link> links;
    std::vector<Traveler> travelers;
    std::vector<Burst> bursts;
    int reserve = 0, waiting = 0, rescued = 0, lost = 0, placed = 0, broken = 0, outcome = 0;
    float elapsed = 0, launchClock = 0, readyTime = 0;
    bool sending = false;
    int dragged = -1;
    Joint2D drag;
    explicit Model(Physics2DWorld& physics) : world(physics) {
        Reset(0);
    }
    Vector2 Pos(int i, float alpha = 1) const {
        return world.Position(nodes.at(i).body, alpha);
    }
    void Reset(int stage) {
        world.Clear();
        nodes.clear();
        links.clear();
        travelers.clear();
        bursts.clear();
        level = Stage(std::clamp(stage, 0, 2));
        reserve = level.budget;
        waiting = level.seeds;
        rescued = lost = placed = broken = outcome = 0;
        elapsed = launchClock = readyTime = 0;
        sending = false;
        dragged = -1;
        drag = {};
        world.SetGravity({0, 550});
        Body2DOptions ground;
        ground.motion = Motion2D::Static;
        for (auto r : level.terrain)
            world.Box({r.x + r.width / 2, r.y + r.height / 2}, {r.width, r.height}, ground);
        AddNode(level.left, true);
        AddNode(level.lower, true);
        AddNode(level.start, false);
        Connect(0, 2);
        Connect(1, 2);
        // The exit's two fixed sockets let a completed bridge/tower dock and carry its load.
        AddNode({level.exit.x, level.exit.y - 35}, true);
        AddNode({level.exit.x, level.exit.y + 35}, true);
    }
    int AddNode(Vector2 p, bool anchor) {
        Body2DOptions opt;
        opt.mass = 0.16f;
        opt.damping = 0.5f;
        opt.motion = anchor ? Motion2D::Static : Motion2D::Dynamic;
        opt.friction = 0.8f;
        opt.category = anchor ? 2 : 4;
        opt.mask = anchor ? 0 : 1;
        nodes.push_back({world.Circle(p, Radius, opt), anchor, true});
        return static_cast<int>(nodes.size()) - 1;
    }
    void Connect(int a, int b) {
        Spring2DOptions spring;
        spring.frequency = 14;
        spring.dampingRatio = 0.8f;
        float rest = Distance(Pos(a), Pos(b));
        links.push_back({a, b, world.DistanceJoint(nodes[a].body, nodes[b].body, spring), rest});
    }
    bool ClearSegment(Vector2 a, Vector2 b) const {
        for (auto r : level.terrain) {
            // Fixed sockets sit just outside the platform surface. Connecting strips
            // must not tunnel through a cliff, even though they have no rod fixture.
            r = {r.x + 3, r.y + 3, r.width - 6, r.height - 6};
            Vector2 collision;
            if (CheckCollisionPointRec(a, r) || CheckCollisionPointRec(b, r) ||
                CheckCollisionLines(a, b, {r.x, r.y}, {r.x + r.width, r.y}, &collision) ||
                CheckCollisionLines(a, b, {r.x, r.y}, {r.x, r.y + r.height}, &collision) ||
                CheckCollisionLines(a, b, {r.x + r.width, r.y}, {r.x + r.width, r.y + r.height},
                                    &collision) ||
                CheckCollisionLines(a, b, {r.x, r.y + r.height}, {r.x + r.width, r.y + r.height},
                                    &collision))
                return false;
        }
        return true;
    }
    bool FreeSpot(Vector2 p) const {
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || p.x < 80 || p.x > 1200 || p.y < 175 ||
            p.y > 615)
            return false;
        for (auto r : level.terrain)
            if (CheckCollisionCircleRec(p, Radius + 2, r))
                return false;
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
            if (nodes[i].alive && Distance(p, Pos(i)) < Radius * 2 + 4)
                return false;
        return true;
    }
    std::vector<int> Candidates(Vector2 p) const {
        std::vector<int> result;
        if (!FreeSpot(p))
            return result;
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
            if (nodes[i].alive && Distance(p, Pos(i)) <= LinkRange && ClearSegment(p, Pos(i)))
                result.push_back(i);
        std::stable_sort(result.begin(), result.end(),
                         [&](int a, int b) { return Distance(p, Pos(a)) < Distance(p, Pos(b)); });
        if (result.size() > 3)
            result.resize(3);
        return result;
    }
    bool Place(Vector2 p) {
        if (!CanPlace(p))
            return false;
        auto near = Candidates(p);
        int node = AddNode(p, false);
        for (int other : near)
            Connect(node, other);
        --reserve;
        ++placed;
        return true;
    }
    bool CanPlace(Vector2 p) const {
        if (outcome || reserve <= 0 || dragged >= 0)
            return false;
        auto near = Candidates(p);
        if (near.size() < 2)
            return false;
        // Reject almost collinear support: it cannot form a useful building triangle.
        bool triangle = false;
        for (size_t i = 0; i < near.size(); ++i)
            for (size_t j = i + 1; j < near.size(); ++j) {
                auto a = Vector2Subtract(Pos(near[i]), p), b = Vector2Subtract(Pos(near[j]), p);
                if (std::fabs(a.x * b.y - a.y * b.x) > 600)
                    triangle = true;
            }
        return triangle;
    }
    int Pick(Vector2 p) const {
        int result = -1;
        float closest = 27;
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
            if (nodes[i].alive) {
                float d = Distance(p, Pos(i));
                if (d < closest) {
                    closest = d;
                    result = i;
                }
            }
        return result;
    }
    bool Remove(int node, bool refund = true) {
        if (node < 0 || node >= static_cast<int>(nodes.size()) || !nodes[node].alive ||
            nodes[node].anchor)
            return false;
        if (dragged == node)
            EndDrag();
        Vector2 at = Pos(node);
        bursts.push_back({at.y > 660 ? Effect::Splash : Effect::Recycle, at});
        world.Destroy(nodes[node].body);
        nodes[node].alive = false;
        links.erase(std::remove_if(links.begin(), links.end(),
                                   [&](const Link& l) { return l.a == node || l.b == node; }),
                    links.end());
        if (refund)
            ++reserve;
        return true;
    }
    bool BeginDrag(int node, Vector2 p) {
        if (outcome || node < 0 || node >= static_cast<int>(nodes.size()) || !nodes[node].alive ||
            nodes[node].anchor)
            return false;
        EndDrag();
        dragged = node;
        drag = world.DragJoint(nodes[node].body, p, 600, 4);
        return true;
    }
    void MoveDrag(Vector2 p) {
        if (world.Valid(drag))
            world.SetDragTarget(drag, p);
    }
    void EndDrag() {
        world.Destroy(drag);
        drag = {};
        dragged = -1;
    }
    bool Linked(int a, int b) const {
        for (auto& l : links)
            if ((l.a == a && l.b == b) || (l.b == a && l.a == b))
                return true;
        return false;
    }
    std::vector<int> Route(int from = 0) const {
        if (from < 0 || from >= static_cast<int>(nodes.size()) || !nodes[from].alive)
            return {};
        std::vector<int> previous(nodes.size(), -1);
        std::queue<int> todo;
        previous[from] = from;
        todo.push(from);
        int goal = -1;
        while (!todo.empty()) {
            int n = todo.front();
            todo.pop();
            if (n == 3 || n == 4) {
                goal = n;
                break;
            }
            for (const auto& l : links) {
                int other = l.a == n ? l.b : l.b == n ? l.a : -1;
                if (other >= 0 && previous[other] < 0 && nodes[other].alive) {
                    previous[other] = n;
                    todo.push(other);
                }
            }
        }
        if (goal < 0)
            return {};
        std::vector<int> route;
        for (int n = goal;; n = previous[n]) {
            route.push_back(n);
            if (n == from)
                break;
        }
        std::reverse(route.begin(), route.end());
        return route;
    }
    bool Launch() {
        if (outcome || waiting <= 0 || Route().empty())
            return false;
        sending = true;
        return true;
    }
    void BeforeStep() {
        if (outcome)
            return;
        for (const auto& n : nodes)
            if (n.alive && !n.anchor && level.wind != 0)
                world.ApplyForce(n.body, {level.wind * (0.5f + std::sin(elapsed * 1.3f)), 0});
        for (const auto& t : travelers)
            if (!t.done && !t.falling && t.to >= 0) {
                if (nodes[t.from].alive)
                    world.ApplyForce(nodes[t.from].body, {0, 1.8f * (1 - t.progress)});
                if (nodes[t.to].alive)
                    world.ApplyForce(nodes[t.to].body, {0, 1.8f * t.progress});
            }
    }
    void AfterStep(float dt) {
        if (outcome)
            return;
        elapsed += dt;
        for (auto& n : nodes)
            n.age += dt;
        for (auto& b : bursts)
            b.age += dt;
        bursts.erase(std::remove_if(bursts.begin(), bursts.end(), [](const Burst& b) { return b.age > 0.8f; }),
                     bursts.end());
        for (auto it = links.begin(); it != links.end();) {
            float extension = std::fabs(Distance(Pos(it->a), Pos(it->b)) / it->rest - 1);
            float force = world.JointForce(it->joint);
            it->stress = std::max(extension / 0.3f, force / 220.0f);
            it->overload =
                it->stress > 1 ? it->overload + dt : std::max(0.0f, it->overload - dt * 2);
            if (it->overload > 0.28f) {
                bursts.push_back({Effect::Snap, RayGame::Lerp(Pos(it->a), Pos(it->b), 0.5f)});
                world.Destroy(it->joint);
                it = links.erase(it);
                ++broken;
            } else
                ++it;
        }
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
            if (nodes[i].alive && !nodes[i].anchor &&
                (Pos(i).y > 690 || Pos(i).x < 0 || Pos(i).x > 1280))
                Remove(i, false);
        auto path = Route();
        readyTime = path.empty() ? 0 : readyTime + dt;
        launchClock -= dt;
        if (sending && waiting > 0 && launchClock <= 0 && !path.empty()) {
            Traveler t;
            t.from = 0;
            t.position = Pos(0);
            travelers.push_back(t);
            --waiting;
            launchClock = 0.75f;
        }
        for (auto& t : travelers) {
            if (t.done)
                continue;
            if (t.falling) {
                t.position = world.Position(t.falling);
                if (t.position.y > 665 || elapsed > 240) {
                    if (t.position.y > 665)
                        bursts.push_back({Effect::Splash, t.position});
                    world.Destroy(t.falling);
                    t.done = true;
                    ++lost;
                }
                continue;
            }
            if (!nodes[t.from].alive ||
                (t.to >= 0 && (!nodes[t.to].alive || !Linked(t.from, t.to)))) {
                Body2DOptions opt;
                opt.mass = 0.18f;
                opt.mask = 0;
                t.falling = world.Circle(t.position, 8, opt);
                continue;
            }
            if (t.to < 0) {
                auto route = Route(t.from);
                if (route.size() == 1) {
                    t.done = true;
                    ++rescued;
                    bursts.push_back({Effect::Rescue, level.exit});
                    continue;
                }
                if (route.empty())
                    continue;
                t.to = route[1];
                t.progress = 0;
            }
            float length = std::max(1.0f, Distance(Pos(t.from), Pos(t.to)));
            t.progress = std::min(1.0f, t.progress + dt * 110 / length);
            t.position = RayGame::Lerp(Pos(t.from), Pos(t.to), t.progress);
            if (t.progress >= 1) {
                t.from = t.to;
                t.to = -1;
            }
        }
        if (rescued >= level.needed) {
            outcome = 1;
            EndDrag();
        } else if (level.seeds - lost < level.needed || elapsed >= 240) {
            outcome = -1;
            EndDrag();
        }
    }
    void Step(float dt = 1.0f / 60) {
        BeforeStep();
        world.Step(dt);
        AfterStep(dt);
    }
    int Score() const {
        return std::max(0, rescued * 100 + reserve * 25 - broken * 10 +
                               (outcome == 1 ? 240 - static_cast<int>(elapsed) : 0));
    }
};
} // namespace Goo
