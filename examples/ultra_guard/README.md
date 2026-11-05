# 光之守护者 / Ultra Guard

> **同人作品声明**：本示例是爱好者制作的非官方同人塔防游戏，与圆谷制作（Tsuburaya
> Productions）及任何版权方无关，也未获其授权或认可。“奥特曼”及相关角色名称、形象
> 归其权利人所有。英雄头像、怪兽贴图和应用图标由奥特曼百科（ultra.fandom.com）上的
> 角色立绘裁剪生成，版权属于原权利人；音效与音乐由脚本合成。若权利人提出要求，将移除本示例。

**v2.0**：12 个关卡加守城生存。24 位光之巨人可以在出战准备页自由组队（每关上阵六位），
抵御 16 种怪兽与宇宙人（含五种 BOSS）。关卡分四篇：守护地球、乐园奇遇、怪兽岛、宇宙决战，
难度逐关升高。本示例同时展示 raygame 的 GameObject/Component 架构在完整游戏中的用法。

v2.0 的变化：英雄名单定为 24 位（新增泰迦、艾克斯、希卡利、高斯、维克特利、罗索、布鲁、奥美迦，
初代退出名单）；新增贝蒙斯坦、加波拉、哥尔赞、达达星人、金潘顿和首领黑暗扎基；怪兽一律使用写实贴图；
首页改为左侧菜单加右侧英雄轮播；数据库三列、图鉴网格布局；应用图标改用真人头像合成。

## 运行

```powershell
xmake build ultra_guard
xmake run ultra_guard
# Android（ARM64）：
xmake android --game=ultra_guard
```

存档与日志位于 `raygame.ultra_guard` 用户数据目录。开发参数：`--capture <目录>`
在隐藏窗口里自动巡览所有页面并导出截图（引擎 `Automation`，可加 `--skin <皮肤目录>`）；`--balance <文件>` 无窗口运行
平衡机器人（每关 3 次，同一版本结果逐字节一致）；`--export-skin <目录>` 把内置画面导出为
皮肤模板。

## 玩法与操作

- 点击右侧英雄卡片（或数字键 1-6），再点击道路旁的空地放置；按住 Shift 连续放置。
- 点击已放置的英雄：升级（最高三级，三级解锁特殊能力）、出售（返还 70%）、切换目标（最前/最强/最近）。
- 空格开始/提前召唤下一波（提前召唤有金币奖励），F 切换 1x-3x 倍速，P/Esc 暂停。
- Q 合体光线（全场伤害），W 空中支援（选区轰炸并眩晕）。
- E 究极合体：六位奥特曼都放置在场上后先蓄能约 25 秒；发动时时间停止，阵容逐个切入、光柱汇成能量球，约 3 秒后砸下，对全场造成很高的真实伤害并眩晕，冷却 75 秒。
- 右键或“取消”按钮退出放置/瞄准；Android 返回键 = Esc，切到后台自动暂停。
- 怪兽特性：巴尔坦被击倒后分裂、艾雷王抗减速、雷德王高护甲、芝顿周期护盾并瞬移、
  贝利亚召唤分身并在半血后暴怒加速、金古桥超高护甲、梅特隆星人让英雄陷入幻觉、托雷基亚穿越异次元并让怪兽狂暴、
  加坦杰厄外壳半血才碎、会石化英雄并用黑雾拖慢攻击；贝蒙斯坦张开吸收口时受到的伤害一半转为回复、
  加波拉钻地潜行时无法攻击、哥尔赞护甲高且光线射程远、达达星人闪烁瞬移、金潘顿火球烧伤一片英雄、
  黑暗扎基用黑暗屏障护住身边怪兽并在半血后狂暴。
- 英雄特色（v2.0 新增）：泰迦光线灼烧；艾克斯解析怪兽使其受到的伤害提高 20%；希卡利光剑无视护甲；
  高斯满月光波让普通怪兽停步；维克特利砸地震晕一片；罗索火球溅射灼烧、布鲁水柱减速击退，两人同场伤害各提高 15%；
  奥美迦头镖在怪兽间弹射，每第 4 次放雷特克鲁特光线。
- 后三关有专属地标：长隆的摩天轮、动物园和鲸鲨池，奥特曼乐园的巨型贝塔魔棒和喷气甲虫号展台，环球影城的城堡、恐龙园和摄影棚，终点是 Universal 地球；
  地标占用离道路最远的格子。

## 关卡配置（assets/levels/）

每关一个 `.level` 文本文件，`levels.txt` 决定顺序（Android 无法列出 APK 内目录）。
新增关卡：复制一个文件改数值并加入索引，格式说明在 `01_tokyo.level` 文件头：

```text
theme    city            # city / mountain / nebula / park / plaza / studio
heroes   z orb trigger geed arc zero   # 可选，最多 6 位，按卡片顺序；省略时为前六位
gold     340
base     17 9            # 基地格子（20 列 × 12 行）
path     -1,2 4,2 4,8 9,8 9,3 14,3 14,9 17,9
wave     redking x4 every 1.8 | baltan x10 every 0.7 after 2 | zetton x1 after 8
```

多条 `path` 时可用 `path 2` 指定某组走第几条路，否则轮流出怪；含 zetton/belial 的波次
自动显示 BOSS 警告。出错的关卡会被跳过，`文件:行号` 错误写入日志并显示在关卡页底部。
关卡页只显示缩略图、名字和星级；未解锁的关卡是灰色并带锁。每页最多 6 关，星级按关卡 id 存档。

## 奥特曼数据库（assets/ultras/codex.txt）

标题页「奥特数据库」三列列出 24 位可上阵的英雄，选中后头像上循环播放招式特效。`codex.txt` 是每位英雄的档案
文本（称号、招式、设定与游戏中的改编说明），招式名按中文常用译名。新增英雄要在 `Data.h` 的 `HeroId`、
`Data.cpp` 的 `kHeroes` 里补战斗数据，在 `Battle.cpp` 的 `FireSignature` 写招式，放 `avatars/<key>.png`，
再补一条档案。

## 怪兽贴图与皮肤

怪兽一律使用 `assets/kaiju/<key>.png` 写实贴图（`kaiju.txt` 设定绘制高度与脚底位置），代码绘制的形象只在
缺图时兜底。皮肤（`assets/skins/`）只给改图的玩家用，用 `--skin <目录>` 启用，不再出现在菜单里。目录内容
均可选：`skin.txt`（名称、界面调色板、怪兽帧条参数、是否在替换地面上画内置道路/装饰）、
`avatars/<英雄>.png`、`kaiju/<怪兽>.png`（横向动画帧条）、`maps/<关卡 id 或主题>.png`。
`--export-skin` 导出的模板带完整注释，可直接复制修改。示例皮肤“霓虹夜”由
`tools/make_sample_skin.py` 从模板生成。音效与音乐暂不在皮肤范围内。

## 用到的引擎功能

| 功能 | 用法 |
| --- | --- |
| GameObject/Component、`ySort` | 怪兽、英雄、尸体、弹道是组件，按 y 排序遮挡；`DestroyAfter` 清理尸体 |
| `PhysicsWorld` 查询 | 怪兽身上挂 trigger `CircleCollider`（mask=0 不产生接触），索敌/爆炸用 `OverlapCircle`，穿透光线用 `SegmentCast` |
| `Projectile` | 圣剑光刃（回旋、穿透）、光弹与赛罗头镖（追踪）、轰炸（抛物） |
| `EffectLayer` | 光线、冲击环、伤害飘字 |
| `ParticleEmitter` | 四个共享发射器：径向爆发、拖尾火花（`alignToVelocity`）、烟雾、余烬 |
| `GameEngine::SetTimeScale` | 1x-3x 倍速；暂停即时间缩放为 0 |
| `UI` | 主题按钮 `ButtonStyle`、`UI::Text` 描边文字、`IsMouseOverUI` 防止点击 HUD 误放置 |
| `AudioManager` | 多声部音效、冷却节流、音效/音乐音量通道 |
| `ResourceManager` | `CodepointsFromText` + `SetFontOptions` 加载中文字体子集；`CreateTextureFromImage` 生成柔光与烘焙头像 |
| `Display` | 地图/背景预渲染后 `PresentOpaque` 绘制；窗口比例不符时 `MarginStyle::BlurredCanvas` 用模糊画面填充四周 |
| `FileSystem` | `ReadAssetText`/`AssetExists` 读取关卡与皮肤（桌面目录与 APK 内通用） |

## 资源与工具

- 字体：`assets/fonts/UltraSC-*.ttf` 是 Noto Sans SC（SIL OFL，许可见同目录）的子集，
  修改界面、关卡或皮肤里的中文后运行 `python examples/ultra_guard/tools/make_font.py` 重新生成。
  脚本默认用 `C:\Windows\Fonts\NotoSansSC-VF.ttf`（可变字体，Medium/Bold 两个实例）；
  未安装时回退本机微软雅黑（`msyh.ttc`/`msyhbd.ttc`，静态字体，不再做 weight 实例化），
  并把字体名改为 UltraGuard SC。运行时只加载 `charset.txt` 里的字，所以漏了这步
  新中文会显示成方块。
- 音频：`tools/make_audio.py` 合成全部音效（WAV）与两首 BGM（Ogg Vorbis，需要 numpy、soundfile）。
- 图标：`tools/make_icon.py` 用真人头像（默认迪迦）合成圆角图标：夜空渐变、金色光环与放射光芒，
  输出窗口图标 `assets/icon.png`、Windows ICO 和 Android 启动图标（含自适应与单色图层）。
- 头像：`assets/avatars/<key>.png` 由 `tools/make_avatars.py` 从全身立绘裁剪生成
  （正方形、头部居中偏上、透明背景）；皮肤目录里的同名文件可以覆盖。
- 怪兽贴图：`assets/kaiju/` 由 `tools/make_kaiju.py` 生成（紧贴内容边界、静止帧，脚底贴图片底边，
  可切掉立绘自带的展示底座）。
