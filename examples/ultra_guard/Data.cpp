#include "Data.h"

namespace UG {

namespace {
const HeroDef kHeroes[] = {
    {"ultraman", "奥特曼", "斯派修姆光线", "穿透光线",
     "来自 M78 星云的初代光之巨人。双臂交叉射出的斯派修姆光线可贯穿一整排怪兽。",
     "三级：光线附带灼烧，2 秒内持续造成伤害", 100, 30, 1.15f, 2.9f, 0, 0, AttackKind::PierceBeam,
     {214, 40, 48, 255}, {226, 232, 240, 255}, {150, 230, 255, 255}},
    {"seven", "赛文奥特曼", "头镖", "回旋穿刺",
     "宇宙警备队的精英战士。掷出的头镖去而复返，往返两次切割路径上的所有敌人。",
     "三级：头镖击中后破甲 3 秒，护甲降低 60%", 130, 24, 1.45f, 3.1f, 0, 0, AttackKind::Boomerang,
     {206, 34, 44, 255}, {200, 208, 220, 255}, {240, 248, 255, 255}},
    {"tiga", "迪迦奥特曼", "哉佩利敖光线", "远程狙击",
     "超古代的光之巨人。蓄力释放的哉佩利敖光线射程极远，专门对付高血量的强敌。",
     "三级：25% 几率暴击，造成 2.5 倍伤害", 180, 120, 2.5f, 4.6f, 0, 0, AttackKind::Snipe,
     {130, 60, 190, 255}, {214, 40, 56, 255}, {255, 214, 120, 255}},
    {"cosmos", "高斯奥特曼", "月神光波", "范围减速",
     "温柔的慈爱战士。月神模式释放的光波让周围怪兽变得迟缓，为队友创造输出时间。",
     "三级：减速提升至 60%，并使怪兽受到伤害提高 20%", 90, 9, 1.1f, 2.3f, 0, 0.42f, AttackKind::Pulse,
     {60, 150, 230, 255}, {226, 232, 240, 255}, {140, 210, 255, 255}},
    {"taro", "泰罗奥特曼", "斯特利姆光线", "爆炸溅射",
     "奥特之父与奥特之母的儿子。炽热的光弹在怪兽群中爆开，对密集的敌人效果拔群。",
     "三级：爆炸点燃地面，范围内敌人持续灼烧", 150, 42, 1.65f, 2.8f, 1.15f, 0, AttackKind::Splash,
     {216, 36, 40, 255}, {255, 196, 60, 255}, {255, 150, 60, 255}},
    {"zero", "赛罗奥特曼", "双头镖", "高速连击",
     "赛文之子，年轻的天才战士。双头镖高速连击，单体输出无人能及。",
     "三级：每第 5 次攻击释放集束光线，贯穿一线敌人", 240, 17, 0.42f, 3.0f, 0, 0, AttackKind::Twin,
     {40, 92, 210, 255}, {214, 40, 56, 255}, {120, 255, 220, 255}},
};

const EnemyDef kEnemies[] = {
    {"巴尔坦星人", "宇宙忍者", "身手敏捷的外星人。被击倒后会分裂成两个分身继续前进。", 70, 1.45f, 0, 8, 1, 0.92f,
     0, false, {70, 78, 120, 255}},
    {"巴尔坦分身", "幻影", "巴尔坦星人的分身，脆弱但移动迅速。", 28, 1.8f, 0, 3, 1, 0.66f, 0, false,
     {90, 160, 220, 255}},
    {"哥莫拉", "古代怪兽", "生活在地底的远古怪兽，皮糙肉厚，鼻角能撞碎大楼。", 140, 1.0f, 2, 12, 1, 1.0f, 0,
     false, {140, 92, 58, 255}},
    {"艾雷王", "宇宙怪兽", "全身带电的宇宙怪兽，速度极快，对减速有 50% 抗性。", 95, 1.95f, 0, 11, 1, 0.95f, 0.5f,
     false, {236, 236, 228, 255}},
    {"红王", "怪力怪兽", "力大无穷的暴躁怪兽，厚重的皮肤提供高额护甲。", 480, 0.72f, 9, 26, 2, 1.25f, 0.2f,
     false, {212, 170, 96, 255}},
    {"杰顿", "宇宙恐龙", "曾击败奥特曼的最强怪兽。会周期性展开光波护盾并瞬间移动。", 2600, 0.55f, 5, 160, 5,
     1.7f, 0.5f, true, {36, 36, 44, 255}},
    {"贝利亚", "黑暗奥特曼", "堕入黑暗的光之国叛徒。持续召唤巴尔坦军团，血量低于一半时会暴怒加速。", 9000,
     0.42f, 10, 600, 20, 1.9f, 0.6f, true, {28, 22, 30, 255}},
};

WaveDef Wave(std::initializer_list<SpawnGroup> groups, bool boss = false) {
    WaveDef w;
    w.groups = groups;
    w.boss = boss;
    return w;
}

using E = EnemyId;

std::vector<LevelDef> BuildLevels() {
    std::vector<LevelDef> levels;

    LevelDef city;
    city.name = "东京湾防线";
    city.subtitle = "第一章 · 黄昏都市";
    city.desc = "怪兽从海湾登陆，沿着环城公路冲向科学特搜队基地。熟悉英雄的能力，守住城市！";
    city.theme = Theme::City;
    city.paths = {{{-1, 2}, {4, 2}, {4, 8}, {9, 8}, {9, 3}, {14, 3}, {14, 9}, {17, 9}}};
    city.base = {17, 9};
    city.gold = 320;
    city.lives = 20;
    city.hpScale = 1.1f;
    city.seed = 1966;
    city.waves = {
        Wave({{E::Gomora, 6, 1.5f, 0, 0}}),
        Wave({{E::Gomora, 6, 1.3f, 0, 0}, {E::Baltan, 4, 1.1f, 4, 0}}),
        Wave({{E::Baltan, 10, 0.9f, 0, 0}}),
        Wave({{E::Gomora, 8, 1.1f, 0, 0}, {E::Eleking, 5, 0.9f, 5, 0}}),
        Wave({{E::RedKing, 2, 3.0f, 0, 0}, {E::Gomora, 8, 1.0f, 2, 0}}),
        Wave({{E::Eleking, 14, 0.6f, 0, 0}}),
        Wave({{E::Baltan, 12, 0.7f, 0, 0}, {E::RedKing, 3, 2.5f, 4, 0}}),
        Wave({{E::Gomora, 14, 0.8f, 0, 0}, {E::Eleking, 8, 0.7f, 6, 0}}),
        Wave({{E::RedKing, 6, 1.8f, 0, 0}, {E::Baltan, 12, 0.6f, 3, 0}}),
        Wave({{E::Zetton, 1, 1, 0, 0}, {E::Gomora, 10, 1.0f, 2, 0}}, true),
        Wave({{E::Eleking, 18, 0.5f, 0, 0}, {E::RedKing, 5, 1.6f, 3, 0}}),
        Wave({{E::RedKing, 6, 1.5f, 0, 0}, {E::Baltan, 16, 0.5f, 2, 0}, {E::Zetton, 1, 1, 10, 0}}, true),
    };
    levels.push_back(city);

    LevelDef mountain;
    mountain.name = "富士山麓";
    mountain.subtitle = "第二章 · 群山回廊";
    mountain.desc = "蜿蜒的山道让怪兽多次折返，善用拐角布置穿透与溅射英雄，打出最大伤害。";
    mountain.theme = Theme::Mountain;
    mountain.paths = {{{-1, 9}, {3, 9}, {3, 2}, {7, 2}, {7, 9}, {11, 9}, {11, 2}, {15, 2}, {15, 7}, {18, 7}}};
    mountain.base = {18, 7};
    mountain.gold = 380;
    mountain.lives = 20;
    mountain.hpScale = 1.55f;
    mountain.seed = 1967;
    mountain.waves = {
        Wave({{E::Gomora, 8, 1.3f, 0, 0}}),
        Wave({{E::Baltan, 10, 0.9f, 0, 0}}),
        Wave({{E::Gomora, 8, 1.0f, 0, 0}, {E::Eleking, 6, 0.8f, 4, 0}}),
        Wave({{E::RedKing, 3, 2.4f, 0, 0}, {E::Baltan, 8, 0.8f, 3, 0}}),
        Wave({{E::Zetton, 1, 1, 0, 0}, {E::Gomora, 8, 1.0f, 3, 0}}, true),
        Wave({{E::Eleking, 16, 0.5f, 0, 0}}),
        Wave({{E::RedKing, 5, 1.8f, 0, 0}, {E::Gomora, 10, 0.8f, 2, 0}}),
        Wave({{E::Baltan, 20, 0.45f, 0, 0}, {E::Eleking, 8, 0.6f, 5, 0}}),
        Wave({{E::RedKing, 8, 1.3f, 0, 0}, {E::Eleking, 10, 0.6f, 4, 0}}),
        Wave({{E::Zetton, 2, 6.0f, 0, 0}, {E::Baltan, 14, 0.6f, 2, 0}}, true),
        Wave({{E::Gomora, 20, 0.55f, 0, 0}, {E::RedKing, 6, 1.4f, 4, 0}}),
        Wave({{E::Eleking, 24, 0.4f, 0, 0}, {E::Baltan, 14, 0.5f, 3, 0}}),
        Wave({{E::RedKing, 12, 1.0f, 0, 0}, {E::Gomora, 14, 0.6f, 3, 0}}),
        Wave({{E::Baltan, 24, 0.4f, 0, 0}, {E::Eleking, 16, 0.45f, 2, 0}, {E::RedKing, 6, 1.2f, 8, 0}}),
        Wave({{E::Zetton, 3, 4.5f, 0, 0}, {E::RedKing, 8, 1.2f, 2, 0}, {E::Eleking, 14, 0.5f, 6, 0}}, true),
    };
    levels.push_back(mountain);

    LevelDef nebula;
    nebula.name = "光之国保卫战";
    nebula.subtitle = "最终章 · M78 星云";
    nebula.desc = "黑暗奥特曼贝利亚率领怪兽大军从两路进攻光之国！守住等离子火花塔，迎接最终决战。";
    nebula.theme = Theme::Nebula;
    nebula.paths = {
        {{-1, 2}, {6, 2}, {6, 6}, {12, 6}, {12, 9}, {17, 9}, {17, 5}, {18, 5}},
        {{-1, 10}, {6, 10}, {6, 6}, {12, 6}, {12, 9}, {17, 9}, {17, 5}, {18, 5}},
    };
    nebula.base = {18, 5};
    nebula.gold = 450;
    nebula.lives = 20;
    nebula.hpScale = 1.6f;
    nebula.seed = 1968;
    nebula.waves = {
        Wave({{E::Gomora, 10, 1.2f, 0, -1}}),
        Wave({{E::Baltan, 14, 0.7f, 0, -1}}),
        Wave({{E::Eleking, 10, 0.7f, 0, 0}, {E::Gomora, 8, 1.0f, 2, 1}}),
        Wave({{E::RedKing, 4, 2.0f, 0, -1}, {E::Baltan, 12, 0.6f, 3, -1}}),
        Wave({{E::Zetton, 1, 1, 0, 1}, {E::Gomora, 12, 0.8f, 2, -1}}, true),
        Wave({{E::Eleking, 24, 0.4f, 0, -1}}),
        Wave({{E::RedKing, 8, 1.4f, 0, -1}, {E::Baltan, 16, 0.5f, 3, -1}}),
        Wave({{E::Gomora, 24, 0.5f, 0, -1}, {E::Eleking, 12, 0.5f, 4, -1}}),
        Wave({{E::RedKing, 12, 1.0f, 0, -1}, {E::Eleking, 16, 0.45f, 3, -1}}),
        Wave({{E::Zetton, 3, 4.0f, 0, -1}, {E::Baltan, 20, 0.45f, 2, -1}}, true),
        Wave({{E::Baltan, 30, 0.35f, 0, -1}, {E::RedKing, 10, 1.0f, 4, -1}}),
        Wave({{E::Eleking, 30, 0.35f, 0, -1}, {E::Gomora, 20, 0.5f, 3, -1}}),
        Wave({{E::RedKing, 18, 0.8f, 0, -1}, {E::Zetton, 2, 5.0f, 6, -1}}, true),
        Wave({{E::Gomora, 30, 0.4f, 0, -1}, {E::Baltan, 30, 0.35f, 2, -1}, {E::Eleking, 20, 0.4f, 6, -1}}),
        Wave({{E::Belial, 1, 1, 0, 0}, {E::RedKing, 10, 1.2f, 4, 1}, {E::Zetton, 3, 5.0f, 10, -1}}, true),
    };
    levels.push_back(nebula);

    return levels;
}
} // namespace

const HeroDef& HeroInfo(HeroId id) { return kHeroes[(int)id]; }
const EnemyDef& EnemyInfo(EnemyId id) { return kEnemies[(int)id]; }

const std::vector<LevelDef>& Levels() {
    static const std::vector<LevelDef> levels = BuildLevels();
    return levels;
}

float HeroDamage(HeroId id, int level) { return HeroInfo(id).damage * (1.0f + 0.65f * level); }
float HeroCooldown(HeroId id, int level) { return HeroInfo(id).cooldown * (1.0f - 0.1f * level); }
float HeroRange(HeroId id, int level) { return HeroInfo(id).range * kTile * (1.0f + 0.1f * level); }

int UpgradeCost(HeroId id, int level) {
    if (level >= 2) return 0;
    float factor = level == 0 ? 0.8f : 1.25f;
    return (int)(std::round(HeroInfo(id).cost * factor / 5.0f) * 5);
}

float WaveHpScale(const LevelDef& level, int wave) {
    return level.hpScale * (1.0f + 0.15f * wave + 0.008f * wave * wave);
}

float BossHpScale(const LevelDef& level, int wave) { return level.hpScale * (1.0f + 0.08f * wave); }

} // namespace UG
