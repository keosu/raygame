#include "Assets.h"
#include "Data.h"
#include <sstream>

namespace UG {

namespace {
const HeroDef kHeroes[] = {
    {"z", "泽塔奥特曼", "泽斯帝姆光线", "穿透光线",
     "自称赛罗弟子的新生代战士。摆出 Z 字蓄能，双手组成十字后从右手放出蓝白色的泽斯帝姆光线，可贯穿整排怪兽。",
     "三级：光线残留灼烧，2 秒内持续造成伤害", 100, 30, 1.15f, 2.9f, 0, 0, AttackKind::PierceBeam,
     {214, 40, 48, 255}, {226, 232, 240, 255}, {120, 220, 255, 255}},
    {"orb", "欧布奥特曼", "欧布圣剑", "回旋穿刺",
     "红凯恢复本来面貌后的欧布原生形态。挥出欧布圣剑的光刃，光刃回旋往返，两次切开路径上的敌人。",
     "三级：圣剑破甲 3 秒，护甲降低 60%", 130, 24, 1.45f, 3.1f, 0, 0, AttackKind::Boomerang,
     {206, 34, 44, 255}, {214, 214, 220, 255}, {255, 206, 80, 255}},
    {"tiga", "迪迦奥特曼", "哉佩利敖光线", "终结狙击",
     "超古代的光之巨人，复合型。双腕交叉后向两侧展开聚能，再组成 L 字放出白色的哉佩利敖光线，专克高体力强敌。",
     "三级：光线临界，25% 几率造成 2.5 倍伤害", 180, 120, 2.5f, 4.6f, 0, 0, AttackKind::Snipe,
     {130, 60, 190, 255}, {214, 40, 56, 255}, {255, 220, 140, 255}},
    {"geed", "捷德奥特曼", "毁灭爆裂", "力场迟缓",
     "贝利亚之子，原始形态。蓄能时溢出红黑色闪电，强烈的力场让周围怪兽变慢，随后放出缠绕红黑闪电的蓝白色光线。",
     "三级：减速提升至 60%，并使怪兽受到伤害提高 20%", 90, 9, 1.1f, 2.3f, 0, 0.42f, AttackKind::Pulse,
     {168, 28, 42, 255}, {230, 232, 238, 255}, {150, 205, 255, 255}},
    {"arc", "亚刻奥特曼", "亚刻泰拉光弹", "彩虹爆破",
     "飞世优马的想象之力化成的巨人。投出亚刻泰拉光弹在敌群中爆开，带出亚刻最终升华光线那样的彩虹色光晕。",
     "三级：爆炸点燃地面，范围内敌人持续灼烧", 150, 42, 1.65f, 2.8f, 1.15f, 0, AttackKind::Splash,
     {210, 36, 48, 255}, {220, 230, 240, 255}, {255, 236, 170, 255}},
    {"zero", "赛罗奥特曼", "赛罗头镖", "高速连击",
     "赛文之子。两把赛罗头镖由脑波遥控连续飞斩，熟练后从额头光束灯射出翡翠色的艾梅利姆切割。",
     "三级：每第 5 次攻击放出艾梅利姆切割，贯穿一线敌人", 240, 17, 0.42f, 3.0f, 0, 0, AttackKind::Twin,
     {36, 88, 210, 255}, {214, 36, 52, 255}, {90, 255, 170, 255}},
    {"trigger", "特利迦奥特曼", "哉佩利敖光线", "三型轮换",
     "真中剑悟变身的光之巨人。轮流切换三型：复合型的哉佩利敖光线贯穿一线，强力型的迪拉修姆光流重击破甲，空中型的兰帕尔特光弹快速连射。",
     "三级：空中型连射由 3 发增至 5 发", 190, 40, 1.25f, 3.5f, 0, 0, AttackKind::Multi,
     {124, 52, 190, 255}, {206, 38, 52, 255}, {236, 226, 255, 255}},
    {"decker", "德凯奥特曼", "雷亚留特光波", "黑洞牵引",
     "与明日见奏大融合的光之巨人，奇迹型。把空间能量压进右手投出雷亚留特光波，落点形成小型黑洞，把周围怪兽往回拽。",
     "三级：黑洞更大，拉回距离增加 50%", 170, 34, 2.1f, 3.0f, 0.95f, 0, AttackKind::Vortex,
     {40, 96, 210, 255}, {210, 40, 52, 255}, {110, 170, 255, 255}},
    {"blazar", "布莱泽奥特曼", "螺旋光矛", "贯穿光矛",
     "与比留间弦人一同战斗的野性巨人。在掌心生成虫洞，抽出红蓝双螺旋的光之长矛投出，笔直贯穿一线怪兽。",
     "三级：光矛命中第一只怪兽时从内部炸开", 220, 64, 1.7f, 3.6f, 0.9f, 0, AttackKind::Spear,
     {70, 74, 92, 255}, {220, 40, 52, 255}, {90, 170, 255, 255}},
};
static_assert(sizeof(kHeroes) / sizeof(kHeroes[0]) == kHeroCount, "one HeroDef per HeroId");

const EnemyDef kEnemies[] = {
    {"baltan",
     "巴尔坦星人",
     "宇宙忍者",
     "身手敏捷的外星人。被击倒后会分裂成两个分身继续前进。",
     70,
     1.45f,
     0,
     8,
     1,
     0.92f,
     0,
     false,
     {70, 78, 120, 255}},
    {"baltan_clone", "巴尔坦分身", "幻影", "巴尔坦星人的分身，脆弱但移动迅速。", 28, 1.8f, 0, 3, 1, 0.66f, 0, false, {90, 160, 220, 255}},
    {"gomora",
     "哥莫拉",
     "古代怪兽",
     "生活在地底的远古怪兽，皮糙肉厚，鼻角能撞碎大楼。",
     140,
     1.0f,
     2,
     12,
     1,
     1.0f,
     0,
     false,
     {140, 92, 58, 255}},
    {"eleking",
     "艾雷王",
     "宇宙怪兽",
     "全身带电的宇宙怪兽，速度极快，对减速有 50% 抗性。",
     95,
     1.95f,
     0,
     11,
     1,
     0.95f,
     0.5f,
     false,
     {236, 236, 228, 255}},
    {"redking",
     "红王",
     "怪力怪兽",
     "力大无穷的暴躁怪兽，厚重的皮肤提供高额护甲。",
     420,
     0.72f,
     5,
     26,
     2,
     1.25f,
     0.15f,
     false,
     {212, 170, 96, 255}},
    {"zetton",
     "芝顿",
     "宇宙恐龙",
     "曾击败奥特曼的宇宙恐龙。会短时间展开光波护盾，并向前瞬移一段距离。",
     1500,
     0.52f,
     3,
     140,
     4,
     1.7f,
     0.35f,
     true,
     {36, 36, 44, 255}},
    {"belial",
     "贝利亚",
     "黑暗奥特曼",
     "堕入黑暗的光之国叛徒。会召出巴尔坦分身，血量低于一半时暴怒加速。",
     4200,
     0.4f,
     4,
     420,
     8,
     1.9f,
     0.4f,
     true,
     {28, 22, 30, 255}},
};

} // namespace

const HeroDef& HeroInfo(HeroId id) { return kHeroes[(int)id]; }

bool HeroByKey(const std::string& key, HeroId& out) {
    for (int i = 0; i < kHeroCount; ++i)
        if (key == kHeroes[i].key) {
            out = (HeroId)i;
            return true;
        }
    return false;
}
const EnemyDef& EnemyInfo(EnemyId id) { return kEnemies[(int)id]; }

float HeroDamage(HeroId id, int level) { return HeroInfo(id).damage * (1.0f + 0.65f * level); }
float HeroCooldown(HeroId id, int level) { return HeroInfo(id).cooldown * (1.0f - 0.1f * level); }
float HeroRange(HeroId id, int level) { return HeroInfo(id).range * kTile * (1.0f + 0.1f * level); }

int UpgradeCost(HeroId id, int level) {
    if (level >= 2) return 0;
    float factor = level == 0 ? 0.8f : 1.25f;
    return (int)(std::round(HeroInfo(id).cost * factor / 5.0f) * 5);
}

float WaveHpScale(const LevelDef& level, int wave) { return level.hpScale * (1.0f + 0.10f * wave + 0.004f * wave * wave); }

float BossHpScale(const LevelDef& level, int wave) { return level.hpScale * (1.0f + 0.04f * wave); }

namespace {
struct CodexState {
    std::vector<UltraEntry> entries;
    std::vector<std::string> errors;
};

std::string CodexTrim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}

CodexState LoadCodex() {
    CodexState state;
    std::string text;
    if (!FileSystem::ReadAssetText(AssetPath("ultras/codex.txt"), text)) {
        state.errors.push_back("ultras/codex.txt: 无法读取奥特曼数据库");
        return state;
    }
    UltraEntry cur;
    bool any = false;
    auto flush = [&]() {
        if (!any) return;
        if (cur.id.empty() || cur.name.empty() || cur.move.empty())
            state.errors.push_back("ultras/codex.txt: 记录缺少 id、name 或 move（" + cur.id + "）");
        else state.entries.push_back(cur);
        cur = {};
        any = false;
    };
    std::istringstream lines(text);
    int number = 0;
    for (std::string raw; std::getline(lines, raw);) {
        ++number;
        if (number == 1 && raw.rfind("\xEF\xBB\xBF", 0) == 0) raw = raw.substr(3);
        std::string line = CodexTrim(raw);
        if (line.empty() || line[0] == '#') continue;
        if (line == "---") {
            flush();
            continue;
        }
        size_t split = line.find_first_of(" \t");
        std::string key = line.substr(0, split);
        std::string value = split == std::string::npos ? "" : CodexTrim(line.substr(split));
        if (value.empty()) {
            state.errors.push_back("ultras/codex.txt:" + std::to_string(number) + ": " + key + " 不能为空");
            continue;
        }
        any = true;
        if (key == "id") cur.id = value;
        else if (key == "name") cur.name = value;
        else if (key == "title") cur.title = value;
        else if (key == "move") cur.move = value;
        else if (key == "desc") cur.desc = value;
        else if (key == "note") cur.note = value;
        else state.errors.push_back("ultras/codex.txt:" + std::to_string(number) + ": 未知字段“" + key + "”");
    }
    flush();
    if (state.entries.empty() && state.errors.empty()) state.errors.push_back("ultras/codex.txt: 没有记录");
    for (const auto& e : state.errors) Diagnostics::Write(LogLevel::Warning, "Ultra: " + e);
    Diagnostics::Write(LogLevel::Info, "Loaded " + std::to_string(state.entries.size()) + " ultras");
    return state;
}

CodexState& Codex() {
    static CodexState state = LoadCodex();
    return state;
}
} // namespace

const std::vector<UltraEntry>& UltraCodex() { return Codex().entries; }
const std::vector<std::string>& UltraErrors() { return Codex().errors; }

} // namespace UG
