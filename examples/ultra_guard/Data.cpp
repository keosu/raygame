#include "Data.h"

namespace UG {

namespace {
const HeroDef kHeroes[] = {
    {"z", "泽塔奥特曼", "泽斯帝姆光线", "穿透光线",
     "宇宙警备队的新人战士，赛罗的弟子。双臂交叉射出的泽斯帝姆光线可贯穿一整排怪兽。",
     "三级：光线附带灼烧，2 秒内持续造成伤害", 100, 30, 1.15f, 2.9f, 0, 0, AttackKind::PierceBeam,
     {214, 40, 48, 255}, {226, 232, 240, 255}, {150, 230, 255, 255}},
    {"orb", "欧布奥特曼", "欧布圣剑", "回旋穿刺",
     "融合两位奥特战士之力的流浪战士。掷出的欧布圣剑去而复返，往返两次切割路径上的敌人。",
     "三级：欧布圣剑击中后破甲 3 秒，护甲降低 60%", 130, 24, 1.45f, 3.1f, 0, 0, AttackKind::Boomerang,
     {206, 34, 44, 255}, {200, 208, 220, 255}, {240, 248, 255, 255}},
    {"tiga", "迪迦奥特曼", "哉佩利敖光线", "远程狙击",
     "超古代的光之巨人。蓄力释放的哉佩利敖光线射程极远，专门对付高血量的强敌。",
     "三级：25% 几率暴击，造成 2.5 倍伤害", 180, 120, 2.5f, 4.6f, 0, 0, AttackKind::Snipe,
     {130, 60, 190, 255}, {214, 40, 56, 255}, {255, 214, 120, 255}},
    {"geed", "捷德奥特曼", "捷德光波", "范围减速",
     "贝利亚之子，背负黑暗血统的战士。释放的捷德光波让周围怪兽变得迟缓，为队友争取输出时间。",
     "三级：减速提升至 60%，并使怪兽受到伤害提高 20%", 90, 9, 1.1f, 2.3f, 0, 0.42f, AttackKind::Pulse,
     {60, 150, 230, 255}, {226, 232, 240, 255}, {140, 210, 255, 255}},
    {"arc", "亚刻奥特曼", "亚刻终结", "爆炸溅射",
     "由想象之力诞生的新生代巨人。炽热的终结光弹在怪兽群中爆开，对密集的敌人效果拔群。",
     "三级：爆炸点燃地面，范围内敌人持续灼烧", 150, 42, 1.65f, 2.8f, 1.15f, 0, AttackKind::Splash,
     {216, 36, 40, 255}, {255, 196, 60, 255}, {255, 150, 60, 255}},
    {"zero", "赛罗奥特曼", "双头镖", "高速连击",
     "赛文之子，年轻的天才战士。双头镖高速连击，单体输出无人能及。",
     "三级：每第 5 次攻击释放集束光线，贯穿一线敌人", 240, 17, 0.42f, 3.0f, 0, 0, AttackKind::Twin,
     {40, 92, 210, 255}, {214, 40, 56, 255}, {120, 255, 220, 255}},
};

const EnemyDef kEnemies[] = {
    {"baltan", "巴尔坦星人", "宇宙忍者", "身手敏捷的外星人。被击倒后会分裂成两个分身继续前进。", 70, 1.45f, 0, 8, 1, 0.92f,
     0, false, {70, 78, 120, 255}},
    {"baltan_clone", "巴尔坦分身", "幻影", "巴尔坦星人的分身，脆弱但移动迅速。", 28, 1.8f, 0, 3, 1, 0.66f, 0, false,
     {90, 160, 220, 255}},
    {"gomora", "哥莫拉", "古代怪兽", "生活在地底的远古怪兽，皮糙肉厚，鼻角能撞碎大楼。", 140, 1.0f, 2, 12, 1, 1.0f, 0,
     false, {140, 92, 58, 255}},
    {"eleking", "艾雷王", "宇宙怪兽", "全身带电的宇宙怪兽，速度极快，对减速有 50% 抗性。", 95, 1.95f, 0, 11, 1, 0.95f, 0.5f,
     false, {236, 236, 228, 255}},
    {"redking", "红王", "怪力怪兽", "力大无穷的暴躁怪兽，厚重的皮肤提供高额护甲。", 480, 0.72f, 9, 26, 2, 1.25f, 0.2f,
     false, {212, 170, 96, 255}},
    {"zetton", "芝顿", "宇宙恐龙", "曾击败奥特曼的最强怪兽。会周期性展开光波护盾并瞬间移动。", 2600, 0.55f, 5, 160, 5,
     1.7f, 0.5f, true, {36, 36, 44, 255}},
    {"belial", "贝利亚", "黑暗奥特曼", "堕入黑暗的光之国叛徒。持续召唤巴尔坦军团，血量低于一半时会暴怒加速。", 9000,
     0.42f, 10, 600, 20, 1.9f, 0.6f, true, {28, 22, 30, 255}},
};

} // namespace

const HeroDef& HeroInfo(HeroId id) { return kHeroes[(int)id]; }
const EnemyDef& EnemyInfo(EnemyId id) { return kEnemies[(int)id]; }


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
