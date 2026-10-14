# 光之守护者 / Ultra Guard

> **同人作品声明**：本示例是爱好者制作的非官方同人塔防游戏，与圆谷制作（Tsuburaya
> Productions）及任何版权方无关，也未获其授权或认可。“奥特曼”及相关角色名称、形象
> 归其权利人所有。示例不含任何官方图片、音频或字体：英雄与怪兽均为代码绘制的
> 示意形象，音效与音乐由脚本合成。若权利人提出要求，将移除本示例。

三关塔防：在道路旁放置六位光之巨人，抵御七种怪兽（含两种 BOSS），撑过所有波次。
本示例同时展示 raygame 的 GameObject/Component 架构在完整游戏中的用法。

## 运行

```powershell
xmake build ultra_guard
xmake run ultra_guard
# Android（ARM64）：
pwsh -NoProfile -File scripts/build-android.ps1 -Game ultra_guard
```

存档与日志位于 `raygame.ultra_guard` 用户数据目录。开发参数：`--capture <目录>`
自动巡览所有页面并导出截图（可加 `--skin <皮肤目录>`）；`--balance <文件>` 无窗口运行
平衡机器人（每关 3 次，同一版本结果逐字节一致）；`--export-skin <目录>` 把内置画面导出为
皮肤模板。

## 玩法与操作

- 点击右侧英雄卡片（或数字键 1-6），再点击道路旁的空地放置；按住 Shift 连续放置。
- 点击已放置的英雄：升级（最高三级，三级解锁特殊能力）、出售（返还 70%）、切换目标（最前/最强/最近）。
- 空格开始/提前召唤下一波（提前召唤有金币奖励），F 切换 1x-3x 倍速，P/Esc 暂停。
- Q 合体光线（全场伤害），W 空中支援（选区轰炸并眩晕）。
- 右键或“取消”按钮退出放置/瞄准；Android 返回键 = Esc，切到后台自动暂停。
- 怪兽特性：巴尔坦被击倒后分裂、艾雷王抗减速、红王高护甲、杰顿周期护盾并瞬移、
  贝利亚召唤分身并在半血后暴怒加速。

## 关卡配置（assets/levels/）

每关一个 `.level` 文本文件，`levels.txt` 决定顺序（Android 无法列出 APK 内目录）。
新增关卡：复制一个文件改数值并加入索引，格式说明在 `01_tokyo.level` 文件头：

```text
theme    city            # city / mountain / nebula
gold     320
base     17 9            # 基地格子（20 列 × 12 行）
path     -1,2 4,2 4,8 9,8 9,3 14,3 14,9 17,9
wave     redking x6 every 1.5 | baltan x16 every 0.5 after 2 | zetton x1 after 10
```

多条 `path` 时可用 `path 2` 指定某组走第几条路，否则轮流出怪；含 zetton/belial 的波次
自动显示 BOSS 警告。出错的关卡会被跳过，`文件:行号` 错误写入日志并显示在关卡页底部。
关卡页每页 3 关可翻页；星级按关卡 id 存档。

## 皮肤（assets/skins/）

`skins.txt` 列出皮肤目录，标题页“皮肤”按钮即时切换（“经典”为内置画面）。目录内容
均可选：`skin.txt`（名称、界面调色板、怪兽帧条参数、是否在替换地面上画内置道路/装饰）、
`avatars/<英雄>.png`、`kaiju/<怪兽>.png`（横向动画帧条）、`maps/<关卡 id 或主题>.png`。
`--export-skin` 导出的模板带完整注释，可直接复制修改。示例皮肤“霓虹夜”由
`tools/make_sample_skin.py` 从模板生成（夜色地面、色相旋转的怪兽、霓虹配色）。
音效与音乐暂不在皮肤范围内（`AudioManager` 尚无卸载/替换接口）。

## 用到的引擎功能

| 功能 | 用法 |
| --- | --- |
| GameObject/Component、`ySort` | 怪兽、英雄、尸体、弹道是组件，按 y 排序遮挡；`DestroyAfter` 清理尸体 |
| `PhysicsWorld` 查询 | 怪兽身上挂 trigger `CircleCollider`（mask=0 不产生接触），索敌/爆炸用 `OverlapCircle`，穿透光线用 `SegmentCast` |
| `Projectile` | 头镖（回旋、穿透）、火球与双头镖（追踪）、轰炸（抛物） |
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
- 图标：`tools/make_icon.py` 用 `--capture` 导出的头像生成 Windows ICO 与 Android 启动图标。
- 头像可替换：把正方形 PNG 放到皮肤目录或游戏资源目录的 `avatars/<key>.png`
  （key 为 z、orb、tiga、geed、arc、zero）即可覆盖代码绘制的头像。
- 现成的真人头像：`assets/avatars/` 由 `tools/make_avatars.py` 从立绘抠图裁剪生成
  （正方形、头部居中偏上、透明背景），源图可来自任意全身抠图；怪兽贴图见
  `assets/skins/real/`，由 `tools/make_kaiju.py` 生成（紧贴内容边界、静止帧，脚底贴图片底边）。
