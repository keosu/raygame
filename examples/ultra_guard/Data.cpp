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
    {"mebius", "梦比优斯奥特曼", "梦比姆射击", "无限光线",
     "化身日比野未来、与 GUYS 并肩作战的光之国新人。左腕的梦比姆手镯画出 ∞ 形光圈，双手组成十字放出梦比姆射击；"
     "与伙伴的羁绊让他燃起烈焰，变为燃烧勇者形态，从胸前的火焰纹章放出梦比姆爆裂。",
     "三级：每第 3 次攻击就是梦比姆爆裂（原为第 4 次），并点燃地面", 170, 42, 1.35f, 3.3f, 1.0f, 0, AttackKind::Infinity,
     {200, 30, 40, 255}, {222, 226, 234, 255}, {255, 190, 80, 255}},
    {"leo", "雷欧奥特曼", "雷欧飞踢", "近身飞踢",
     "来自狮子座 L77 星云的宇宙拳法高手，以凤源的身份守护地球，也是赛罗的师父。高高跃起，右脚燃起火焰般的能量踢向怪兽，"
     "擅长贴身肉搏。",
     "三级：飞踢后再补踢附近第二只怪兽，眩晕延长到 1 秒", 140, 70, 1.9f, 2.0f, 0.7f, 0, AttackKind::Kick,
     {196, 34, 30, 255}, {200, 200, 206, 255}, {255, 140, 50, 255}},
    {"ginga", "银河奥特曼", "银河雷电", "连锁闪电",
     "来自未来的光之巨人，礼堂光用银河火花变身。全身水晶随招式变色：黄色是银河雷电，红色是银河火球，蓝色是银河十字射击。"
     "银河雷电是双臂画圆卷起闪电漩涡再掷出，劈出的闪电在怪兽之间连锁跳跃。",
     "三级：闪电多跳两次，并让命中的怪兽短暂麻痹", 200, 40, 1.5f, 3.2f, 0, 0, AttackKind::Chain,
     {206, 32, 48, 255}, {120, 220, 255, 255}, {255, 236, 90, 255}},
    {"gaia", "盖亚奥特曼", "光子刃", "扇形光刃",
     "地球自己孕育的大地之光，与天才少年高山我梦合为一体。双手交叉蓄能后头部一甩，挥出鞭子一样的光子刃，"
     "红金色的光鞭扫过一片扇形，把前方怪兽一起切开。",
     "三级：光子刃把命中的怪兽击退", 180, 48, 1.7f, 2.7f, 0, 0, AttackKind::Edge,
     {200, 34, 40, 255}, {226, 228, 236, 255}, {255, 120, 70, 255}},
    {"nexus", "奈克瑟斯奥特曼", "梅塔领域", "领域压制",
     "被称为“the next”的银色巨人，光芒在历代适能者之间传递。展开金色的梅塔领域，把怪兽困进不会波及人类的异空间；"
     "红色青年型的终结技是超越光线。",
     "三级：领域内的怪兽受到的伤害提高 20%", 160, 16, 3.2f, 3.0f, 1.25f, 0.35f, AttackKind::Field,
     {150, 156, 170, 255}, {200, 40, 50, 255}, {255, 210, 110, 255}},
    {"dyna", "戴拿奥特曼", "索尔捷特光线", "闪亮 / 强壮",
     "与 SUPER GUTS 的飞鸟信合为一体的光之巨人，迪迦的后辈。闪亮型双手组成十字放出蓝白色的索尔捷特光线；"
     "强壮型浑身燃红，抡出火球般的加尔奈特轰炸；奇迹型则会造出黑洞般的雷沃留姆光波。",
     "三级：每第 2 次攻击就是强壮型的加尔奈特轰炸（原为第 3 次）", 150, 26, 1.05f, 3.6f, 0.9f, 0, AttackKind::Dual,
     {40, 80, 200, 255}, {206, 36, 46, 255}, {130, 200, 255, 255}},
    {"teo", "提欧奥特曼", "提欧修姆光线", "蓄力光线",
     "来自行星 H12 的蓝色巨人，以兽医系大学生光石息吹的身份生活。全身泛起黄色光辉，两臂交叉、左手压住右腕放出提欧修姆光线，"
     "盯住同一只怪兽越打越强。",
     "三级：蓄力上限提高，蓄满时光线贯穿一线", 200, 54, 1.3f, 3.4f, 0, 0, AttackKind::Focus,
     {40, 110, 220, 255}, {226, 230, 240, 255}, {255, 214, 90, 255}},
    {"taiga", "泰迦奥特曼", "斯特利姆爆裂", "灼烧光线",
     "泰罗之子，与泰塔斯、风马组成三人小队，附身工藤优幸战斗。双臂摆成 T 字，从泰迦火花放出红蓝交织的斯特利姆爆裂，"
     "命中的怪兽会持续燃烧。",
     "三级：每第 3 次攻击放出三重斯特利姆爆裂，贯穿一线并灼烧", 160, 44, 1.3f, 3.2f, 0, 0, AttackKind::Strium,
     {206, 38, 46, 255}, {222, 226, 236, 255}, {255, 140, 90, 255}},
    {"x", "艾克斯奥特曼", "扎纳迪姆光线", "数据解析",
     "与 Xio 队员大空大地一体化的电子生命体巨人。手臂交叉成 X 字放出扎纳迪姆光线，把怪兽的数据解析出来，"
     "被解析的怪兽受到所有英雄的伤害都会提高。",
     "三级：每第 4 次攻击放出 X 字斩击，解析落点周围所有怪兽", 150, 36, 1.1f, 3.4f, 1.0f, 0, AttackKind::Xanadium,
     {200, 40, 52, 255}, {214, 220, 230, 255}, {255, 226, 120, 255}},
    {"hikari", "希卡利奥特曼", "骑士光剑", "无视护甲",
     "光之国宇宙科学技术局的天才科学家，蓝色的骑士。右腕的骑士手镯伸出光剑，连续斩击时无视护甲，"
     "还会顺势劈中旁边的怪兽，专克金古桥这样的硬壳敌人。",
     "三级：每第 5 次斩击后放出骑士射击，贯穿一线", 180, 22, 0.62f, 2.3f, 0.8f, 0, AttackKind::Knight,
     {38, 82, 196, 255}, {210, 214, 226, 255}, {120, 196, 255, 255}},
    {"cosmos", "高斯奥特曼", "满月光波", "平息控制",
     "温柔的蓝色巨人，与春野武藏一同守护怪兽与人类的共存。月神形态放出满月光波，让狂暴的怪兽平静下来，"
     "范围内的普通怪兽会暂时停步。",
     "三级：每第 4 次攻击切换科罗娜形态，放出纳伊巴斯塔光线重击", 160, 22, 2.0f, 3.0f, 1.0f, 0.3f, AttackKind::Luna,
     {52, 110, 220, 255}, {214, 222, 236, 255}, {160, 214, 255, 255}},
    {"victory", "维克特利奥特曼", "维克特利姆光线", "大地冲击",
     "守护地底世界的维克特利安战士，额头与胸前的 V 字水晶闪着黄光。借用 EX 雷德王的力量砸向地面，"
     "冲击波把一片怪兽震得站不稳。",
     "三级：每第 3 次攻击放出维克特利姆光线，贯穿一线", 170, 46, 1.8f, 2.6f, 1.0f, 0, AttackKind::Quake,
     {40, 36, 46, 255}, {226, 60, 54, 255}, {255, 196, 70, 255}},
    {"rosso", "罗索奥特曼", "烈焰风暴", "火焰溅射",
     "湊家长子湊克己变身的火之巨人，与弟弟布鲁并称罗布兄弟。火焰形态把烈焰凝成火球投出，落点一片燃烧；"
     "布鲁同在战场时，兄弟同心，伤害提高 15%。",
     "三级：火球范围扩大，灼烧伤害提高", 140, 30, 1.4f, 2.9f, 0.9f, 0, AttackKind::Flame,
     {206, 40, 40, 255}, {216, 218, 226, 255}, {255, 120, 50, 255}},
    {"blu", "布鲁奥特曼", "水流冲击", "水流减速",
     "湊家次子湊旭变身的水之巨人。水流形态双手推出高压水柱，贯穿一线并让怪兽减速、后退；"
     "罗索同在战场时，兄弟同心，伤害提高 15%。",
     "三级：水柱更粗，减速与击退增强", 140, 22, 1.2f, 3.2f, 0, 0.35f, AttackKind::Aqua,
     {40, 92, 210, 255}, {214, 218, 230, 255}, {90, 200, 255, 255}},
    {"omega", "奥美迦奥特曼", "雷特克鲁特光线", "头镖弹射",
     "失去记忆坠落地球、以大木田宙人身份生活的宇宙人。头上的奥美迦头镖投出后在怪兽之间来回弹射；"
     "双臂画弧汇成五边形光圈，交叠成十字放出亮蓝色的雷特克鲁特光线，每第 4 次攻击就是一次重击。",
     "三级：头镖多弹射一次，雷特克鲁特光线改为每第 3 次", 210, 36, 1.1f, 3.3f, 0, 0, AttackKind::Omega,
     {200, 36, 44, 255}, {218, 222, 232, 255}, {80, 170, 255, 255}},
};
static_assert(sizeof(kHeroes) / sizeof(kHeroes[0]) == kHeroCount, "one HeroDef per HeroId");

const EnemyDef kEnemies[] = {
    {"baltan", "巴尔坦星人", "宇宙忍者", "身手敏捷的外星人。被击倒后会分裂成两个分身继续前进。", 70, 1.45f, 0, 8, 1, 0.92f, 0, false,
     {70, 78, 120, 255}, "剪刀光线", 7, 2.0f, 3.2f, KaijuStrike::Beam},
    {"baltan_clone", "巴尔坦分身", "幻影", "巴尔坦星人的分身，脆弱但移动迅速。", 28, 1.8f, 0, 3, 1, 0.66f, 0, false, {90, 160, 220, 255},
     "", 0, 0, 0, KaijuStrike::None},
    {"gomora", "哥莫拉", "古代怪兽", "生活在地底的远古怪兽，皮糙肉厚，鼻角能撞碎大楼。", 140, 1.0f, 2, 12, 1, 1.0f, 0, false,
     {140, 92, 58, 255}, "超振动波", 13, 1.6f, 3.6f, KaijuStrike::Shock},
    {"eleking", "艾雷王", "宇宙怪兽", "全身带电的宇宙怪兽，速度极快，对减速有 50% 抗性。", 95, 1.95f, 0, 11, 1, 0.95f, 0.5f, false,
     {236, 236, 228, 255}, "放电", 6, 2.2f, 2.6f, KaijuStrike::Bolt},
    {"redking", "雷德王", "怪力怪兽", "力大无穷的暴躁怪兽，厚重的皮肤提供高额护甲。", 420, 0.72f, 5, 26, 2, 1.25f, 0.15f, false,
     {212, 170, 96, 255}, "投掷岩石", 24, 2.8f, 5.0f, KaijuStrike::Rock},
    {"zetton", "芝顿", "宇宙恐龙", "曾击败奥特曼的宇宙恐龙。会短时间展开光波护盾，并向前瞬移一段距离。", 1500, 0.52f, 3, 140, 4, 1.7f,
     0.35f, true, {36, 36, 44, 255}, "一兆度火球", 42, 3.3f, 4.4f, KaijuStrike::Fireball},
    {"belial", "贝利亚", "黑暗奥特曼", "堕入黑暗的光之国叛徒。会召出巴尔坦分身，血量低于一半时暴怒加速。", 4200, 0.4f, 4, 420, 8, 1.9f,
     0.4f, true, {28, 22, 30, 255}, "死亡光线", 48, 3.5f, 4.0f, KaijuStrike::Beam},
    {"kingjoe", "金古桥", "宇宙机器人",
     "佩丹星人送来的超金属机器人，赛文的头镖与艾梅利姆光线都伤不了它。护甲极高，伤害低的攻击几乎无效，需要破甲或重击。", 520, 0.62f,
     14, 45, 3, 1.3f, 0.3f, false, {200, 190, 160, 255}, "破坏光线", 18, 2.6f, 3.6f, KaijuStrike::Beam},
    {"metron", "梅特隆星人", "幻觉宇宙人",
     "曾用掺了红色结晶的香烟让人类陷入幻觉、自相残杀，还和诸星弹隔着矮桌对坐交谈。会让附近一位英雄陷入幻觉，暂时停手。", 170, 1.1f, 1,
     22, 2, 1.0f, 0, false, {226, 74, 40, 255}, "眼光线", 8, 2.4f, 3.0f, KaijuStrike::Beam},
    {"tregear", "托雷基亚", "黑暗奥特曼",
     "泰罗的儿时挚友，曾任宇宙科学技术局副局长，后来离开光之国、坠入混沌。会打开异次元之门消失再出现，还会放出光线让身边怪兽狂暴。",
     2800, 0.5f, 4, 380, 6, 1.8f, 0.4f, true, {40, 60, 150, 255}, "托雷基亚光线", 40, 3.2f, 4.2f, KaijuStrike::Beam},
    {"gatanothor", "加坦杰厄", "邪神",
     "《迪迦奥特曼》最终话的超古代邪神，鹦鹉螺般的外壳挡住了迪迦的光线。外壳在半血前护甲极高；会用石化光线定住英雄，并放出黑雾拖慢全场攻击。",
     7600, 0.3f, 12, 700, 14, 2.3f, 0.7f, true, {70, 66, 58, 255}, "触手横扫", 60, 3.6f, 5.5f, KaijuStrike::Shock},
    {"bemstar", "贝蒙斯坦", "宇宙大怪兽",
     "腹部的五角形吸收口能吞下光线和炮火。每隔一段时间张开吸收口，期间受到的伤害有一半转为回复，避开那几秒再集火。", 380, 0.75f, 3,
     30, 2, 1.25f, 0.2f, false, {150, 160, 170, 255}, "吸收反射光线", 20, 2.6f, 4.0f, KaijuStrike::Beam},
    {"gabora", "加波拉", "放射能怪兽",
     "爱吃铀矿的地底怪兽，头部像花瓣一样张开。会钻进地下快速潜行一段，潜行时无法被攻击。", 200, 1.05f, 2, 18, 1, 1.05f, 0, false,
     {120, 110, 100, 255}, "放射能光线", 10, 2.0f, 3.4f, KaijuStrike::Beam},
    {"golza", "哥尔赞", "超古代怪兽",
     "沉睡在地底的超古代怪兽，额头的器官放出超音波光线。全身覆盖坚硬的鳞甲，光线射程很远。", 520, 0.7f, 6, 32, 2, 1.3f, 0.2f, false,
     {110, 80, 60, 255}, "超音波光线", 22, 3.6f, 4.6f, KaijuStrike::Beam},
    {"dada", "达达星人", "三面怪人",
     "有三张面孔的宇宙人，曾用微缩器把人类缩小带走。经常闪烁瞬移，一眨眼就向前跳出一大段。", 110, 1.15f, 0, 15, 1, 0.95f, 0, false,
     {230, 230, 230, 255}, "缩小光线", 8, 2.4f, 3.0f, KaijuStrike::Bolt},
    {"kingpandon", "金潘顿", "双头怪兽",
     "长着两个脑袋的凶暴怪兽，两张嘴同时喷出火焰。体格巨大、生命很高，火球会烧伤一片英雄。", 900, 0.6f, 4, 60, 3, 1.45f, 0.3f, false,
     {200, 70, 50, 255}, "双头火焰", 30, 3.0f, 4.2f, KaijuStrike::Fireball},
    {"zagi", "黑暗扎基", "黑暗巨人",
     "模仿奈克瑟斯造出的黑暗巨人。会张开黑暗屏障护住身边的怪兽，血量低于一半后更加狂暴，屏障也更频繁。", 5200, 0.42f, 5, 480, 9,
     1.9f, 0.45f, true, {40, 20, 44, 255}, "暗黑雷击光线", 50, 3.6f, 3.8f, KaijuStrike::Beam},
};
static_assert(sizeof(kEnemies) / sizeof(kEnemies[0]) == kEnemyCount, "one EnemyDef per EnemyId");
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
float HeroMaxHp(HeroId id, int level) { return (90.0f + HeroInfo(id).cost * 0.4f) * (1.0f + 0.35f * level); }

int UpgradeCost(HeroId id, int level) {
    if (level >= 2) return 0;
    float factor = level == 0 ? 0.8f : 1.25f;
    return (int)(std::round(HeroInfo(id).cost * factor / 5.0f) * 5);
}

// Survival grows exponentially: a field full of max-level heroes must eventually fall.
float WaveHpScale(const LevelDef& level, int wave) {
    if (level.endless) return level.hpScale * std::pow(1.15f, (float)wave);
    return level.hpScale * (1.0f + 0.10f * wave + 0.004f * wave * wave);
}

float BossHpScale(const LevelDef& level, int wave) {
    if (level.endless) return level.hpScale * std::pow(1.12f, (float)wave);
    return level.hpScale * (1.0f + 0.04f * wave);
}

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
