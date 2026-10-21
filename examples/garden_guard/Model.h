#pragma once
#include "Core.h"
#include <cstdint>
#include <array>

namespace Garden {
inline constexpr int Rows = 5, Columns = 9;
inline constexpr float Left = 225, Top = 206, CellW = 100, CellH = 85;
enum class Kind { Sunflower, Peashooter, Wallnut, Iceflower, Cherry, Repeater };
inline constexpr int Costs[] = {50, 100, 75, 150, 125, 200};
inline constexpr float Recharges[] = {4, 4, 8, 7, 16, 9};
inline const char* Names[] = {"SUNFLOWER", "PEASHOOTER", "WALLNUT",
                              "ICEFLOWER", "CHERRY",     "REPEATER"};
struct Plant {
    int row, col;
    Kind kind;
    float hp = 100, timer = 0, flash = 0, age = 0;
};
struct Zombie {
    int id, row, type;
    float x, hp, slow = 0, bite = 0, flash = 0;
};
struct Pea {
    int row;
    float x, damage;
    bool ice;
};
struct Sun {
    int id;
    Vector2 position; // where it comes to rest
    float age = 0;
    Vector2 origin{}; // where its entrance starts
    float travel = 0; // entrance length in seconds; 0 means already resting
    bool hop = false; // tossed in an arc by a sunflower instead of drifting down from the sky
    float Lifetime() const { return travel + 10; }
    // Where the token is drawn and clicked.
    Vector2 At() const {
        if (travel <= 0 || age >= travel)
            return position;
        float t = age / travel, e = hop ? t : 1 - (1 - t) * (1 - t);
        Vector2 p{origin.x + (position.x - origin.x) * e, origin.y + (position.y - origin.y) * e};
        if (hop)
            p.y -= std::sin(t * PI) * 46;
        else
            p.x += std::sin(age * 2.4f + id) * 10 * (1 - t);
        return p;
    }
};
// A defeated visitor topples over before it disappears.
struct Fallen {
    Zombie zombie;
    float age = 0;
};
struct Blast {
    Vector2 position;
    float age = 0;
};
struct Spawn {
    float time;
    int row, type;
};
inline Vector2 Center(int row, int col) {
    return {Left + (col + 0.5f) * CellW, Top + (row + 0.5f) * CellH};
}
class Model {
public:
    std::vector<Plant> plants;
    std::vector<Zombie> zombies;
    std::vector<Pea> peas;
    std::vector<Sun> suns;
    std::vector<Blast> blasts;
    std::vector<Fallen> fallen;
    std::vector<Spawn> schedule;
    std::array<float, 6> cooldown{};
    std::array<bool, 5> mowerReady{true, true, true, true, true};
    std::array<float, 5> mowerX{-1, -1, -1, -1, -1};
    int stage = 0, sun = 200, selected = 1, score = 0, defeated = 0, spawned = 0, outcome = 0;
    int nextId = 1, soundShot = 0, soundHit = 0, soundPlant = 0, collected = 0;
    Vector2 lastPickup{}; // where the latest collected token was, for its flight to the counter
    float elapsed = 0, skyTimer = 3, waveFlash = 0;
    std::uint32_t random = 1;
    unsigned Random() {
        random = random * 1664525u + 1013904223u;
        return random;
    }
    void Reset(int level) {
        *this = Model{};
        stage = std::clamp(level, 0, 2);
        random = 317 + stage * 41;
        sun = 250;
        int waves = 3;
        for (int wave = 0; wave < waves; ++wave) {
            int count = 6 + stage * 3 + wave * 2;
            for (int i = 0; i < count; ++i) {
                int type = 0;
                if (stage >= 1 && (i + wave) % 3 == 1)
                    type = 1; // armour
                if (stage >= 2 && i % 4 == 2)
                    type = 2; // runner
                schedule.push_back(
                    {22.0f + wave * 38 + i * 2.3f, (i * 3 + wave + stage) % 5, type});
            }
        }
    }
    int PlantAt(int row, int col) const {
        for (size_t i = 0; i < plants.size(); ++i)
            if (plants[i].row == row && plants[i].col == col)
                return static_cast<int>(i);
        return -1;
    }
    bool PlantSeed(int row, int col, int seed) {
        if (outcome || row < 0 || row >= Rows || col < 0 || col >= Columns || seed < 0 ||
            seed >= 6 || PlantAt(row, col) >= 0 || cooldown[seed] > 0 || sun < Costs[seed])
            return false;
        sun -= Costs[seed];
        cooldown[seed] = Recharges[seed];
        plants.push_back({row, col, static_cast<Kind>(seed), seed == 2 ? 520.0f : 110.0f,
                          seed == 0   ? 7.0f
                          : seed == 4 ? 0.85f
                                      : 0.45f});
        ++soundPlant;
        return true;
    }
    bool Shovel(int row, int col) {
        if (outcome)
            return false;
        int i = PlantAt(row, col);
        if (i < 0)
            return false;
        plants.erase(plants.begin() + i);
        ++soundPlant;
        return true;
    }
    bool Collect(Vector2 point) {
        if (outcome || !std::isfinite(point.x) || !std::isfinite(point.y))
            return false;
        for (auto it = suns.rbegin(); it != suns.rend(); ++it) {
            Vector2 at = it->At();
            float x = point.x - at.x, y = point.y - at.y;
            if (x * x + y * y < 34 * 34) {
                sun += 25;
                score += 25;
                lastPickup = at;
                ++collected;
                suns.erase(std::next(it).base());
                return true;
            }
        }
        return false;
    }
    bool Click(Vector2 point) {
        if (outcome || !std::isfinite(point.x) || !std::isfinite(point.y))
            return false;
        if (Collect(point))
            return true;
        int col = static_cast<int>(std::floor((point.x - Left) / CellW)),
            row = static_cast<int>(std::floor((point.y - Top) / CellH));
        return selected == 6 ? Shovel(row, col) : PlantSeed(row, col, selected);
    }
    void SpawnZombie(int row, int type, float x = 1190) {
        zombies.push_back({nextId++, row, type, x,
                           type == 1   ? 230.0f
                           : type == 2 ? 90.0f
                                       : 110.0f});
    }
    void Damage(Zombie& z, float amount) {
        z.hp -= amount;
        z.flash = 0.12f;
        ++soundHit;
    }
    void Update(float dt) {
        if (outcome || !std::isfinite(dt) || dt <= 0)
            return;
        dt = std::min(dt, 0.05f);
        elapsed += dt;
        waveFlash = std::max(0.0f, waveFlash - dt);
        for (auto& c : cooldown)
            c = std::max(0.0f, c - dt);
        while (spawned < static_cast<int>(schedule.size()) && schedule[spawned].time <= elapsed) {
            auto spawn = schedule[spawned++];
            SpawnZombie(spawn.row, spawn.type);
            waveFlash = 2;
        }
        skyTimer -= dt;
        if (skyTimer <= 0) {
            skyTimer += 6;
            Vector2 rest{Left + 40 + static_cast<float>(Random() % 830),
                         Top + 20 + static_cast<float>(Random() % 370)};
            Vector2 from{rest.x, Top - 130};
            suns.push_back({nextId++, rest, 0, from, 1.5f + (rest.y - from.y) / 160});
        }
        for (auto& p : plants) {
            p.timer -= dt;
            p.age += dt;
            p.flash = std::max(0.0f, p.flash - dt);
            if (p.kind == Kind::Sunflower && p.timer <= 0) {
                p.timer += 8;
                auto center = Center(p.row, p.col);
                suns.push_back({nextId++, {center.x + 15, center.y + 5}, 0, {center.x, center.y - 10},
                                0.55f, true});
            } else if (p.kind == Kind::Cherry && p.timer <= 0) {
                auto center = Center(p.row, p.col);
                blasts.push_back({center});
                for (auto& z : zombies)
                    if (std::abs(z.row - p.row) <= 1 && std::fabs(z.x - center.x) <= CellW * 1.55f)
                        Damage(z, 700);
                p.hp = 0;
            } else if (p.kind == Kind::Peashooter || p.kind == Kind::Iceflower ||
                       p.kind == Kind::Repeater) {
                float x = Center(p.row, p.col).x;
                bool target = std::any_of(zombies.begin(), zombies.end(), [&](const Zombie& z) {
                    return z.hp > 0 && z.row == p.row && z.x > x;
                });
                if (p.timer <= 0 && target) {
                    p.timer = p.kind == Kind::Repeater ? 1.0f : 1.25f;
                    p.flash = 0.15f;
                    peas.push_back({p.row, x + 32, 25, p.kind == Kind::Iceflower});
                    if (p.kind == Kind::Repeater)
                        peas.push_back({p.row, x + 10, 25, false});
                    ++soundShot;
                } else if (!target)
                    p.timer = std::max(0.0f, p.timer);
            }
        }
        for (auto it = peas.begin(); it != peas.end();) {
            float previous = it->x;
            it->x += 460 * dt;
            Zombie* nearest = nullptr;
            for (auto& z : zombies)
                if (z.hp > 0 && z.row == it->row && z.x + 22 >= previous && z.x - 22 <= it->x &&
                    (!nearest || z.x < nearest->x))
                    nearest = &z;
            if (nearest) {
                Damage(*nearest, it->damage);
                if (it->ice)
                    nearest->slow = 3;
                it = peas.erase(it);
            } else if (it->x > 1250)
                it = peas.erase(it);
            else
                ++it;
        }
        for (auto& z : zombies) {
            if (z.hp <= 0)
                continue;
            z.slow = std::max(0.0f, z.slow - dt);
            z.flash = std::max(0.0f, z.flash - dt);
            Plant* target = nullptr;
            for (auto& p : plants) {
                float x = Center(p.row, p.col).x;
                if (p.hp > 0 && p.row == z.row && z.x >= x - 10 && z.x <= x + 48 &&
                    (!target || p.col > target->col))
                    target = &p;
            }
            if (target) {
                target->hp -= 36 * dt;
                z.bite += dt;
            } else {
                z.x -= (z.type == 2 ? 29.0f : 16.0f + stage * 1.2f) * (z.slow > 0 ? 0.48f : 1) * dt;
                z.bite = 0;
            }
            if (z.x < Left - 40) {
                if (mowerReady[z.row]) {
                    mowerReady[z.row] = false;
                    mowerX[z.row] = Left - 55;
                } else if (mowerX[z.row] < 0)
                    outcome = -1;
            }
        }
        for (int row = 0; row < Rows; ++row)
            if (mowerX[row] >= 0) {
                float prev = mowerX[row];
                mowerX[row] += 740 * dt;
                for (auto& z : zombies)
                    if (z.row == row && z.hp > 0 && z.x >= prev - 65 && z.x <= mowerX[row] + 65)
                        Damage(z, 10000);
                if (mowerX[row] > 1260)
                    mowerX[row] = -1;
            }
        for (auto& f : fallen)
            f.age += dt;
        fallen.erase(std::remove_if(fallen.begin(), fallen.end(),
                                    [](const Fallen& f) { return f.age > 0.8f; }),
                     fallen.end());
        for (const auto& z : zombies)
            if (z.hp <= 0) {
                ++defeated;
                score += z.type == 1 ? 150 : 100;
                fallen.push_back({z});
            }
        zombies.erase(std::remove_if(zombies.begin(), zombies.end(),
                                     [](const Zombie& z) { return z.hp <= 0; }),
                      zombies.end());
        plants.erase(
            std::remove_if(plants.begin(), plants.end(), [](const Plant& p) { return p.hp <= 0; }),
            plants.end());
        for (auto& s : suns)
            s.age += dt;
        suns.erase(
            std::remove_if(suns.begin(), suns.end(), [](const Sun& s) { return s.age > s.Lifetime(); }),
            suns.end());
        for (auto& b : blasts)
            b.age += dt;
        blasts.erase(std::remove_if(blasts.begin(), blasts.end(),
                                    [](const Blast& b) { return b.age > 0.6f; }),
                     blasts.end());
        if (outcome == 0 && spawned == static_cast<int>(schedule.size()) && zombies.empty()) {
            outcome = 1;
            score += sun +
                     500 * static_cast<int>(std::count(mowerReady.begin(), mowerReady.end(), true));
        }
    }
};
} // namespace Garden
