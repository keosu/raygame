#pragma once

#include "Common.h"

namespace UG {

// The first six ids are the original roster (keys z orb tiga geed arc zero); levels pick up to six.
enum class HeroId { Ultraman, Seven, Tiga, Cosmos, Taro, Zero, Trigger, Decker, Blazar, Mebius, Leo, Ginga, Gaia, Nexus, Dyna, Man, Teo, Count };
enum class AttackKind { PierceBeam, Boomerang, Snipe, Pulse, Splash, Twin, Multi, Vortex, Spear, Slash, Infinity, Kick, Chain, Edge, Field, Dual, Focus };
constexpr int kHeroCount = (int)HeroId::Count;
constexpr int kRosterMax = 6; // hero cards that fit the battle panel

struct HeroDef {
    const char* key; // also the optional avatar override: assets/avatars/<key>.png
    const char* name;
    const char* move; // signature technique
    const char* role;
    const char* desc;
    const char* perk; // unlocked at level 3
    int cost;
    float damage;
    float cooldown;
    float range;  // tiles
    float splash; // tiles
    float slow;
    AttackKind kind;
    Color primary;
    Color accent;
    Color beam;
};

const HeroDef& HeroInfo(HeroId id);
bool HeroByKey(const std::string& key, HeroId& out);

enum class EnemyId { Baltan, BaltanClone, Gomora, Eleking, RedKing, Zetton, Belial, KingJoe, Metron, Tregear, Gatanothor, Count };
constexpr int kEnemyCount = (int)EnemyId::Count;

// How a kaiju hits heroes in levels with `assault` above zero.
enum class KaijuStrike { None, Shock, Beam, Bolt, Rock, Fireball };

struct EnemyDef {
    const char* key; // used in level files
    const char* name;
    const char* title;
    const char* desc;
    float hp;
    float speed; // tiles per second
    float armor;
    int reward;
    int leak;   // lives lost when reaching the base
    float size; // 1 = ordinary kaiju sprite
    float slowResist;
    bool boss;
    Color color;
    const char* strikeName;
    float strikeDamage; // per hit on a hero, before the level's assault scale; 0 never attacks
    float strikeRange;  // tiles
    float strikeCooldown;
    KaijuStrike strike;
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

enum class Theme { City, Mountain, Nebula, Park, Plaza, Studio, Volcano, Moon, Citadel };
constexpr const char* kThemeKeys[] = {"city", "mountain", "nebula", "park", "plaza", "studio", "volcano", "moon", "citadel"};

inline Color ThemeAccent(Theme theme) {
    switch (theme) {
    case Theme::Citadel:
        return {120, 210, 255, 255};
    case Theme::Volcano:
        return {255, 96, 40, 255};
    case Theme::Moon:
        return {150, 200, 255, 255};
    case Theme::City:
        return {255, 160, 70, 255};
    case Theme::Mountain:
        return {120, 220, 130, 255};
    case Theme::Nebula:
        return {170, 140, 255, 255};
    case Theme::Park:
        return {255, 136, 48, 255};
    case Theme::Plaza:
        return {255, 64, 84, 255};
    case Theme::Studio:
        return {70, 168, 255, 255};
    }
    return {255, 160, 70, 255};
}

struct LevelDef {
    std::string id;   // save key; defaults to the file name
    std::string file; // source, for error messages
    std::string name;
    std::string subtitle;
    std::string arc; // story arc the level page groups by
    std::string tag; // "arc-number" such as 2-3, assigned from the order in levels.txt
    std::string desc;
    Theme theme;
    std::vector<std::vector<GridPoint>> paths; // waypoints, first point may be off-grid
    GridPoint base;
    int gold;
    int lives;
    float hpScale;
    float assault = 0; // kaiju attack heroes when > 0; scales their hit damage
    float siege = 0;   // > 0: the base is a tower with this much durability; kaiju stop at its wall and batter it
    // Castle walls run along this rectangle's border cells (inclusive); a path crossing a wall makes a gate.
    GridPoint castleMin{-1, -1}, castleMax{-1, -1};
    bool HasCastle() const { return castleMax.col > castleMin.col; }
    bool endless = false; // survival: waves are generated without end
    std::vector<WaveDef> waves;
    std::uint32_t seed;
    std::vector<HeroId> heroes; // recommended battle cards in order; defaults to the original six
};

// Lore catalog in assets/ultras/codex.txt. Playable fighters stay in HeroInfo;
// extra entries are the roster future levels can promote onto the field.
struct UltraEntry {
    std::string id;
    std::string name;
    std::string title;
    std::string move;
    std::string desc;
    std::string note;
};

const std::vector<UltraEntry>& UltraCodex();
const std::vector<std::string>& UltraErrors();

// Loaded once from assets/levels (levels.txt lists the files in order).
const std::vector<LevelDef>& Levels();
// Problems found while loading, as "file:line: message"; invalid levels are skipped.
const std::vector<std::string>& LevelErrors();
// Parses one level file's text; returns false and appends to errors when invalid.
bool ParseLevel(const std::string& text, const std::string& file, LevelDef& out, std::vector<std::string>& errors);
// levels/survival.level (not in levels.txt); null when missing or invalid.
const LevelDef* SurvivalLevel();
constexpr int kSurvivalIndex = -1; // level index that stands for the survival map
// Survival wave `wave` (0-based): the kaiju pool widens and a boss arrives every fifth wave.
WaveDef SurvivalWave(int wave);
float SurvivalAssault(const LevelDef& level, int wave);
// Skill showcase: one straight road past a single hero, kaiju walking by without end.
LevelDef DemoLevel(HeroId hero);

// Stats for a hero at upgrade level 0..2.
float HeroDamage(HeroId id, int level);
float HeroCooldown(HeroId id, int level);
float HeroRange(HeroId id, int level); // pixels
float HeroMaxHp(HeroId id, int level);  // energy kaiju strikes drain; at zero the hero is exhausted
int UpgradeCost(HeroId id, int level); // cost to go from `level` to level + 1
float WaveHpScale(const LevelDef& level, int wave);
float BossHpScale(const LevelDef& level, int wave);

} // namespace UG
