#pragma once

#include "Common.h"

namespace UG {

enum class HeroId { Ultraman, Seven, Tiga, Cosmos, Taro, Zero, Count };
enum class AttackKind { PierceBeam, Boomerang, Snipe, Pulse, Splash, Twin };
constexpr int kHeroCount = (int)HeroId::Count;

struct HeroDef {
    const char* key;   // also the optional avatar override: assets/avatars/<key>.png
    const char* name;
    const char* move;  // signature technique
    const char* role;
    const char* desc;
    const char* perk;  // unlocked at level 3
    int cost;
    float damage;
    float cooldown;
    float range; // tiles
    float splash; // tiles
    float slow;
    AttackKind kind;
    Color primary;
    Color accent;
    Color beam;
};

const HeroDef& HeroInfo(HeroId id);

enum class EnemyId { Baltan, BaltanClone, Gomora, Eleking, RedKing, Zetton, Belial, Count };
constexpr int kEnemyCount = (int)EnemyId::Count;

struct EnemyDef {
    const char* name;
    const char* title;
    const char* desc;
    float hp;
    float speed; // tiles per second
    float armor;
    int reward;
    int leak; // lives lost when reaching the base
    float size; // 1 = ordinary kaiju sprite
    float slowResist;
    bool boss;
    Color color;
};

const EnemyDef& EnemyInfo(EnemyId id);

struct GridPoint {
    int col, row;
};

struct SpawnGroup {
    EnemyId enemy;
    int count;
    float interval;
    float delay;
    int path; // -1 alternates between all paths
};

struct WaveDef {
    std::vector<SpawnGroup> groups;
    bool boss = false;
};

enum class Theme { City, Mountain, Nebula };

struct LevelDef {
    const char* name;
    const char* subtitle;
    const char* desc;
    Theme theme;
    std::vector<std::vector<GridPoint>> paths; // waypoints, first point may be off-grid
    GridPoint base;
    int gold;
    int lives;
    float hpScale;
    std::vector<WaveDef> waves;
    std::uint32_t seed;
};

const std::vector<LevelDef>& Levels();

// Stats for a hero at upgrade level 0..2.
float HeroDamage(HeroId id, int level);
float HeroCooldown(HeroId id, int level);
float HeroRange(HeroId id, int level); // pixels
int UpgradeCost(HeroId id, int level); // cost to go from `level` to level + 1
float WaveHpScale(const LevelDef& level, int wave);
float BossHpScale(const LevelDef& level, int wave);

} // namespace UG
