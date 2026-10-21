#pragma once
#include "Core.h"
#include <cstdint>

namespace Marble {
inline constexpr float Spacing = 36;
struct Ball {
    float distance;
    int color;
    float lag = 0;    // drawn this far behind its slot along the path, easing to zero
    float settle = 1; // 0..1 while a newly inserted ball slides from where the shot struck
    Vector2 from{};
};
// Points rising from a cleared group.
struct Popup {
    Vector2 position;
    int points, combo;
    float age = 0;
};
struct Shot {
    Vector2 position, velocity;
    int color;
    bool blast = false;
};
struct Spark {
    Vector2 position;
    float age;
    int color;
};
struct Path {
    std::vector<Vector2> points;
    std::vector<float> lengths;
    float length = 0;
    void Set(int stage) {
        points.clear();
        lengths.clear();
        length = 0;
        std::vector<Vector2> knots;
        if (stage == 0)
            knots = {{74, 178},  {352, 156}, {757, 147}, {1152, 173}, {1170, 354}, {1134, 526},
                     {922, 579}, {709, 559}, {533, 515}, {306, 529},  {130, 597}};
        if (stage == 1)
            knots = {{80, 560},   {139, 350},  {92, 184},  {375, 149}, {834, 161},
                     {1158, 240}, {1118, 453}, {873, 576}, {410, 598}, {352, 476},
                     {388, 323},  {809, 287},  {941, 364}, {872, 456}};
        if (stage == 2)
            knots = {{67, 160},  {563, 160}, {1147, 170}, {1180, 525}, {826, 585},
                     {316, 579}, {111, 451}, {159, 281},  {424, 239},  {817, 241},
                     {968, 335}, {961, 466}, {730, 526},  {384, 507},  {309, 403}};
        // Catmull-Rom sampled into an arc-length polyline; physics and drawing use the same path.
        for (size_t i = 0; i + 1 < knots.size(); ++i) {
            auto a = knots[i ? i - 1 : i], b = knots[i], c = knots[i + 1],
                 d = knots[std::min(i + 2, knots.size() - 1)];
            for (int step = 0; step < 24; ++step) {
                float t = step / 24.0f, t2 = t * t, t3 = t2 * t;
                auto value = [=](float p0, float p1, float p2, float p3) {
                    return 0.5f *
                           ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 +
                            (-p0 + 3 * p1 - 3 * p2 + p3) * t3);
                };
                points.push_back({value(a.x, b.x, c.x, d.x), value(a.y, b.y, c.y, d.y)});
            }
        }
        points.push_back(knots.back());
        lengths.push_back(0);
        for (size_t i = 1; i < points.size(); ++i) {
            float x = points[i].x - points[i - 1].x, y = points[i].y - points[i - 1].y;
            length += std::sqrt(x * x + y * y);
            lengths.push_back(length);
        }
    }
    Vector2 At(float distance) const {
        distance = std::clamp(distance, 0.0f, length);
        auto it = std::lower_bound(lengths.begin(), lengths.end(), distance);
        size_t i = std::max<size_t>(1, static_cast<size_t>(it - lengths.begin()));
        float t = (distance - lengths[i - 1]) / std::max(0.01f, lengths[i] - lengths[i - 1]);
        return {points[i - 1].x + (points[i].x - points[i - 1].x) * t,
                points[i - 1].y + (points[i].y - points[i - 1].y) * t};
    }
    Vector2 Tangent(float distance) const {
        auto a = At(distance - 3), b = At(distance + 3);
        float x = b.x - a.x, y = b.y - a.y, norm = std::sqrt(x * x + y * y);
        return norm > 0.01f ? Vector2{x / norm, y / norm} : Vector2{1, 0};
    }
};
class Model {
public:
    Path path;
    std::vector<Ball> balls;
    std::vector<Shot> shots;
    std::vector<Spark> sparks;
    std::vector<Popup> popups;
    Vector2 launcher{640, 390};
    int stage = 0, remaining = 0, score = 0, combo = 0, shotsFired = 0, removed = 0, outcome = 0;
    int current = 0, next = 1, colors = 4;
    float cooldown = 0, slow = 0, elapsed = 0;
    int soundShot = 0, soundClear = 0;
    std::uint32_t random = 1;
    unsigned Random() {
        random = random * 1664525u + 1013904223u;
        return random;
    }
    int AvailableColor() {
        std::vector<int> available;
        for (const auto& b : balls)
            if (std::find(available.begin(), available.end(), b.color) == available.end())
                available.push_back(b.color);
        return available.empty() ? static_cast<int>(Random() % colors)
                                 : available[Random() % available.size()];
    }
    void Reset(int level) {
        *this = Model{};
        stage = std::clamp(level, 0, 2);
        random = 4101 + stage * 173;
        colors = stage == 2 ? 5 : 4;
        path.Set(stage);
        remaining = 45 + stage * 12;
        launcher = stage == 1 ? Vector2{655, 417} : Vector2{640, 378};
        int initial = 22 + stage * 2;
        for (int i = 0; i < initial; ++i)
            Spawn((initial - i - 1) * Spacing + 100);
        current = AvailableColor();
        next = AvailableColor();
    }
    void Spawn(float distance) {
        int c = static_cast<int>(Random() % colors);
        if (balls.size() >= 2 && balls.back().color == c && balls[balls.size() - 2].color == c)
            c = (c + 1) % colors;
        balls.push_back({distance, c});
        --remaining;
    }
    void Swap() {
        std::swap(current, next);
    }
    bool Fire(Vector2 aim) {
        if (!std::isfinite(aim.x) || !std::isfinite(aim.y))
            return false;
        float dx = aim.x - launcher.x, dy = aim.y - launcher.y,
              length = std::sqrt(dx * dx + dy * dy);
        if (outcome || cooldown > 0 || shots.size() >= 6 || length < 30)
            return false;
        shots.push_back(
            {launcher, {dx / length * 920, dy / length * 920}, current, (shotsFired + 1) % 9 == 0});
        ++shotsFired;
        ++soundShot;
        current = next;
        next = AvailableColor();
        cooldown = 0.20f;
        return true;
    }
    // Where a ball is drawn: behind its slot by its lag, or on its way in from the impact.
    Vector2 Visible(const Ball& b) const {
        Vector2 slot = path.At(b.distance - b.lag);
        if (b.settle >= 1)
            return slot;
        float e = 1 - (1 - b.settle) * (1 - b.settle);
        return {b.from.x + (slot.x - b.from.x) * e, b.from.y + (slot.y - b.from.y) * e};
    }
    void Erase(size_t first, size_t last, bool chain) {
        int count = static_cast<int>(last - first);
        combo = chain ? combo + 1 : 1;
        removed += count;
        score += count * 100 * combo;
        slow = std::max(slow, 1.2f);
        ++soundClear;
        Vector2 middle{};
        for (size_t i = first; i < last; ++i) {
            auto at = Visible(balls[i]);
            sparks.push_back({at, 0, balls[i].color});
            middle = {middle.x + at.x / count, middle.y + at.y / count};
        }
        popups.push_back({middle, count * 100 * combo, combo});
        balls.erase(balls.begin() + first, balls.begin() + last);
    }
    bool Match(size_t at, bool chain) {
        if (at >= balls.size())
            return false;
        size_t first = at, last = at + 1;
        while (first > 0 && balls[first - 1].color == balls[at].color &&
               balls[first - 1].distance - balls[first].distance <= Spacing + 1)
            --first;
        while (last < balls.size() && balls[last].color == balls[at].color &&
               balls[last - 1].distance - balls[last].distance <= Spacing + 1)
            ++last;
        if (last - first < 3)
            return false;
        Erase(first, last, chain);
        return true;
    }
    // Hit side is projected on the path tangent; prefix shifts leave room for the new ball.
    void Insert(size_t hit, int color, Vector2 impact, bool blast = false) {
        if (hit >= balls.size())
            return;
        if (blast) {
            Erase(hit > 1 ? hit - 2 : 0, std::min(hit + 3, balls.size()), false);
            return;
        }
        auto center = path.At(balls[hit].distance), tangent = path.Tangent(balls[hit].distance);
        bool front = (impact.x - center.x) * tangent.x + (impact.y - center.y) * tangent.y > 0;
        size_t at = front ? hit : hit + 1;
        float distance = balls[hit].distance + (front ? Spacing : 0);
        // Balls ahead make room at once but glide there (lag) instead of jumping.
        for (size_t i = 0; i < at; ++i) {
            balls[i].distance += Spacing;
            balls[i].lag += Spacing;
        }
        if (front)
            distance = balls[hit].distance + Spacing;
        balls.insert(balls.begin() + at, {distance, color, 0, 0, impact});
        // Restore strict order if the hit was beside a closing gap.
        for (size_t i = balls.size() - 1; i > 0; --i) {
            float ordered = std::max(balls[i - 1].distance, balls[i].distance + Spacing);
            balls[i - 1].lag += ordered - balls[i - 1].distance;
            balls[i - 1].distance = ordered;
        }
        if (!Match(at, false))
            combo = 0;
    }
    void Update(float dt) {
        if (outcome || !std::isfinite(dt) || dt <= 0)
            return;
        dt = std::min(dt, 0.05f);
        elapsed += dt;
        cooldown = std::max(0.0f, cooldown - dt);
        slow = std::max(0.0f, slow - dt);
        float speed = (13 + stage * 3.5f) * (slow > 0 ? 0.28f : 1);
        for (auto& b : balls) {
            b.distance += speed * dt;
            b.lag = b.lag > 0.05f ? b.lag * std::exp(-14 * dt) : 0;
            b.settle = std::min(1.0f, b.settle + dt / 0.12f);
        }
        if (remaining > 0 && (balls.empty() || balls.back().distance >= Spacing))
            Spawn(balls.empty() ? 0 : balls.back().distance - Spacing);
        for (size_t i = 0; i + 1 < balls.size(); ++i) {
            float gap = balls[i].distance - balls[i + 1].distance - Spacing;
            if (gap > 0.01f) {
                float shift = std::min(gap, 190 * dt);
                for (size_t j = 0; j <= i; ++j)
                    balls[j].distance -= shift;
                if (gap <= shift + 0.01f) {
                    if (Match(i, true))
                        break;
                }
                break;
            }
        }
        // Swept segment/circle intersection prevents bullets tunnelling at low FPS.
        for (auto it = shots.begin(); it != shots.end();) {
            auto start = it->position;
            Vector2 end{start.x + it->velocity.x * dt, start.y + it->velocity.y * dt};
            float vx = end.x - start.x, vy = end.y - start.y, a = vx * vx + vy * vy, earliest = 2;
            size_t hit = balls.size();
            for (size_t i = 0; i < balls.size(); ++i) {
                auto p = path.At(balls[i].distance);
                float x = start.x - p.x, y = start.y - p.y;
                float b = 2 * (x * vx + y * vy), c = x * x + y * y - 31 * 31,
                      det = b * b - 4 * a * c;
                if (det < 0 || a < 0.01f)
                    continue;
                float t = c <= 0 ? 0 : (-b - std::sqrt(det)) / (2 * a);
                if (t >= 0 && t <= 1 && t < earliest) {
                    earliest = t;
                    hit = i;
                }
            }
            if (hit < balls.size()) {
                Insert(hit, it->color, {start.x + vx * earliest, start.y + vy * earliest},
                       it->blast);
                it = shots.erase(it);
            } else {
                it->position = end;
                if (end.x < 0 || end.x > 1280 || end.y < 80 || end.y > 680)
                    it = shots.erase(it);
                else
                    ++it;
            }
        }
        for (auto& s : sparks)
            s.age += dt;
        sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
                                    [](const Spark& s) { return s.age > 0.7f; }),
                     sparks.end());
        for (auto& p : popups)
            p.age += dt;
        popups.erase(std::remove_if(popups.begin(), popups.end(),
                                    [](const Popup& p) { return p.age > 1.0f; }),
                     popups.end());
        if (balls.empty() && remaining == 0) {
            outcome = 1;
            score += 1000 + std::max(0, 240 - static_cast<int>(elapsed)) * 10;
        } else if (!balls.empty() && balls.front().distance >= path.length - 20)
            outcome = -1;
    }
};
} // namespace Marble
