#include "../examples/marble_temple/Model.h"
#include "../examples/garden_guard/Model.h"
#include <cstdio>
#include <limits>

int passed = 0, failed = 0;
static void Check(bool ok, const char* name) {
    if (ok)
        ++passed;
    else {
        ++failed;
        std::printf("FAIL %s\n", name);
    }
}
static void MarbleRules() {
    Marble::Model m;
    m.Reset(0);
    m.remaining = 0;
    m.balls = {{200, 1}, {164, 1}, {128, 2}};
    auto p = m.path.At(164), t = m.path.Tangent(164);
    m.Insert(1, 1, {p.x - t.x * 17, p.y - t.y * 17});
    Check(m.removed == 3 && m.balls.size() == 1 && m.score == 300,
          "insert completes three and preserves other colour");
    m.Reset(0);
    m.remaining = 0;
    m.balls = {{400, 2}, {364, 2}, {328, 1}, {292, 1}, {256, 1}, {220, 2}};
    Check(m.Match(3, false), "initial run erased");
    for (int i = 0; i < 90; ++i)
        m.Update(1.0f / 60);
    Check(m.removed == 6 && m.combo == 2 && m.outcome == 1, "closing gap triggers chain and win");
    m.Reset(0);
    int old = m.current, next = m.next;
    m.Swap();
    Check(m.current == next && m.next == old, "swap real ammunition");
    Check(!m.Fire(m.launcher), "reject zero length aiming");
    Check(!m.Fire({std::numeric_limits<float>::quiet_NaN(), 0}), "invalid aim is ignored");
    Check(m.Fire({640, 100}) && !m.Fire({640, 100}), "shot cooldown prevents duplicate input");
    m.Reset(0);
    m.remaining = 0;
    m.balls = {{200, 0}};
    m.shots = {{{m.path.At(200).x, m.path.At(200).y + 70}, {0, -3000}, 0, true}};
    m.Update(0.05f);
    Check(m.removed == 1, "swept projectile hits across circle");
    m.Reset(0);
    m.balls.front().distance = m.path.length;
    m.Update(1.0f / 60);
    Check(m.outcome == -1, "gate loss");
    m.Reset(0);
    m.balls.clear();
    m.remaining = 1;
    m.Update(1.0f / 60);
    Check(m.balls.size() == 1 && m.outcome == 0, "remaining supply prevents early win");
    m.Reset(0);
    m.remaining = 0;
    m.balls = {{200, 0}, {164, 1}, {128, 2}};
    p = m.path.At(164);
    t = m.path.Tangent(164);
    m.Insert(1, 3, {p.x + t.x * 17, p.y + t.y * 17});
    Check(m.balls.size() == 4 && m.balls[1].color == 3 && m.balls[2].color == 1 &&
              m.balls[0].distance > m.balls[1].distance &&
              m.balls[1].distance > m.balls[2].distance,
          "front insertion preserves path order");
    for (int level = 0; level < 3; ++level) {
        m.Reset(level);
        Check(m.path.points.size() > 200 && m.path.length > 2000,
              "arc length sampled campaign track");
        // Ordinary aiming, swapping and firing only. No direct insertion/teleport/score edits.
        for (int step = 0; step < 60000 && !m.outcome; ++step) {
            if (m.cooldown <= 0) {
                size_t target = 0;
                float best = -1e9f;
                bool swap = false;
                for (size_t i = 0; i < m.balls.size(); ++i) {
                    auto p = m.path.At(m.balls[i].distance);
                    float dx = p.x - m.launcher.x, dy = p.y - m.launcher.y;
                    int pair = (i > 0 && m.balls[i - 1].color == m.balls[i].color) ||
                               (i + 1 < m.balls.size() && m.balls[i + 1].color == m.balls[i].color);
                    bool usable = m.balls[i].color == m.current || m.balls[i].color == m.next;
                    float priority = (usable ? 5000.0f : 0) + pair * 2000 +
                                     m.balls[i].distance * 0.5f - std::sqrt(dx * dx + dy * dy);
                    if (priority > best) {
                        best = priority;
                        target = i;
                        swap = m.balls[i].color != m.current && m.balls[i].color == m.next;
                    }
                }
                if (!m.balls.empty()) {
                    if (swap)
                        m.Swap();
                    m.Fire(m.path.At(m.balls[target].distance));
                }
            }
            m.Update(1.0f / 60);
        }
        std::printf("Marble stage %d: outcome=%d time=%.1f score=%d shots=%d left=%zu\n", level + 1,
                    m.outcome, m.elapsed, m.score, m.shotsFired, m.balls.size());
        Check(m.outcome == 1 && m.removed > 0 && m.shotsFired > 0,
              "campaign won with ordinary shot commands");
    }
}
static void GardenRules() {
    Garden::Model m;
    m.Reset(0);
    Check(!m.PlantSeed(-1, 0, 0) && !m.PlantSeed(0, 9, 0) && !m.PlantSeed(0, 0, 6),
          "invalid planting boundaries");
    Check(m.PlantSeed(0, 0, 0) && m.sun == 200, "seed spends sun");
    Check(!m.PlantSeed(1, 0, 0) && !m.PlantSeed(0, 0, 1), "cooldown and occupied cell");
    Check(m.Shovel(0, 0) && !m.Shovel(0, 0), "shovel removes once");
    m.suns.push_back({100, {300, 300}});
    Check(m.Collect({310, 300}) && m.sun == 225 && !m.Collect({310, 300}), "sun collects once");
    m.Reset(0);
    m.sun = 0;
    Check(!m.PlantSeed(0, 0, 1), "insufficient sun");
    m.Reset(0);
    m.schedule = {{100, 0, 0}};
    m.SpawnZombie(0, 0, 900);
    m.peas.push_back({1, 850, 100, false});
    m.Update(0.05f);
    Check(m.zombies[0].hp == 110, "projectile cannot hit different row");
    m.peas.push_back({0, 850, 25, true});
    for (int i = 0; i < 5; ++i)
        m.Update(0.05f);
    Check(m.zombies[0].hp == 85 && m.zombies[0].slow > 0, "pea sweep and ice slow");
    m.Reset(0);
    m.schedule = {{100, 0, 0}};
    m.PlantSeed(2, 3, 4);
    m.SpawnZombie(2, 1, Garden::Center(2, 3).x);
    m.SpawnZombie(3, 0, Garden::Center(2, 3).x + 100);
    m.SpawnZombie(0, 0, 900);
    for (int i = 0; i < 60; ++i)
        m.Update(1.0f / 60);
    Check(m.zombies.size() == 1 && m.zombies[0].row == 0 && m.PlantAt(2, 3) < 0,
          "cherry area and self destruction");
    m.Reset(0);
    m.schedule = {{100, 0, 0}};
    m.SpawnZombie(0, 0, Garden::Left - 45);
    for (int i = 0; i < 120; ++i)
        m.Update(1.0f / 60);
    Check(!m.mowerReady[0] && m.zombies.empty() && !m.outcome, "first breach consumes mower");
    m.SpawnZombie(0, 0, Garden::Left - 45);
    m.Update(1.0f / 60);
    Check(m.outcome == -1, "second breach loses");
    int oldSun = m.sun;
    m.suns.push_back({100, {300, 300}});
    Check(!m.Click({300, 300}) && m.sun == oldSun, "finished round rejects gameplay input");
    m.Reset(0);
    m.schedule = {{100, 0, 0}};
    m.PlantSeed(0, 0, 0);
    for (int i = 0; i < 450; ++i)
        m.Update(1.0f / 60);
    Check(std::any_of(
              m.suns.begin(), m.suns.end(),
              [](const Garden::Sun& s) { return s.position.x == Garden::Center(0, 0).x + 15; }),
          "sunflower generates collectible income");
    m.Reset(0);
    m.schedule = {{100, 0, 0}};
    m.PlantSeed(0, 3, 2);
    m.SpawnZombie(0, 0, Garden::Center(0, 3).x + 20);
    for (int i = 0; i < 240; ++i)
        m.Update(1.0f / 60);
    Check(m.PlantAt(0, 3) >= 0 && m.plants[0].hp < 520 && m.zombies[0].x > Garden::Center(0, 3).x,
          "wallnut blocks while enemy bites");
    m.Reset(0);
    m.schedule = {{100, 0, 0}};
    m.PlantSeed(0, 1, 5);
    m.SpawnZombie(0, 1, 1000);
    for (int i = 0; i < 30; ++i)
        m.Update(1.0f / 60);
    Check(m.peas.size() == 2, "repeater emits two projectiles");
    m.Reset(1);
    Check(std::any_of(m.schedule.begin(), m.schedule.end(),
                      [](const Garden::Spawn& s) { return s.type == 1; }),
          "armoured wave composition");
    m.Reset(2);
    Check(std::any_of(m.schedule.begin(), m.schedule.end(),
                      [](const Garden::Spawn& s) { return s.type == 2; }),
          "runner wave composition");
    for (int level = 0; level < 3; ++level) {
        m.Reset(level);
        // Collect, plant and defend through normal actions with real costs and cooldowns.
        for (int step = 0; step < 24000 && !m.outcome; ++step) {
            while (!m.suns.empty())
                m.Collect(m.suns.back().position);
            int producers = static_cast<int>(
                std::count_if(m.plants.begin(), m.plants.end(), [](const Garden::Plant& p) {
                    return p.kind == Garden::Kind::Sunflower;
                }));
            for (int row = 0; row < 5; ++row)
                if (producers < 5 && m.PlantAt(row, 0) < 0) {
                    if (m.PlantSeed(row, 0, 0))
                        ++producers;
                }
            for (int row = 0; row < 5; ++row)
                if (m.PlantAt(row, 1) < 0)
                    m.PlantSeed(row, 1, 1);
            for (int row = 0; row < 5; ++row)
                if (m.PlantAt(row, 2) < 0)
                    m.PlantSeed(row, 2, level == 2 ? 5 : 1);
            for (int row = 0; row < 5; ++row)
                if (m.PlantAt(row, 5) < 0)
                    m.PlantSeed(row, 5, 2);
            for (int row = 0; row < 5; ++row)
                if (m.PlantAt(row, 3) < 0)
                    m.PlantSeed(row, 3, 3);
            m.Update(1.0f / 60);
        }
        std::printf("Garden stage %d: outcome=%d time=%.1f score=%d defeated=%d/%zu plants=%zu\n",
                    level + 1, m.outcome, m.elapsed, m.score, m.defeated, m.schedule.size(),
                    m.plants.size());
        Check(m.outcome == 1 && m.defeated == static_cast<int>(m.schedule.size()),
              "campaign defended with normal planting and collection");
    }
}
int main() {
    MarbleRules();
    GardenRules();
    std::printf("Campaign tests: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
