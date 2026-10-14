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
自动巡览所有页面并导出截图；`--balance <文件>` 无窗口运行平衡机器人（每关 3 次）。

## 玩法与操作

- 点击右侧英雄卡片（或数字键 1-6），再点击道路旁的空地放置；按住 Shift 连续放置。
- 点击已放置的英雄：升级（最高三级，三级解锁特殊能力）、出售（返还 70%）、切换目标（最前/最强/最近）。
- 空格开始/提前召唤下一波（提前召唤有金币奖励），F 切换 1x-3x 倍速，P/Esc 暂停。
- Q 合体光线（全场伤害），W 空中支援（选区轰炸并眩晕）。
- 右键或“取消”按钮退出放置/瞄准；Android 返回键 = Esc，切到后台自动暂停。
- 怪兽特性：巴尔坦被击倒后分裂、艾雷王抗减速、红王高护甲、杰顿周期护盾并瞬移、
  贝利亚召唤分身并在半血后暴怒加速。

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
| `Display` | 地图/背景预渲染后 `PresentOpaque` 绘制 |

## 资源与工具

- 字体：`assets/fonts/UltraSC-*.ttf` 是 Noto Sans SC（SIL OFL，许可见同目录）的子集，
  修改界面文字后运行 `python examples/ultra_guard/tools/make_font.py` 重新生成。
- 音频：`tools/make_audio.py` 合成全部音效与两首 BGM（需要 numpy）。
- 图标：`tools/make_icon.py` 用 `--capture` 导出的头像生成 Windows ICO 与 Android 启动图标。
- 头像可替换：把正方形 PNG 放到游戏资源目录 `avatars/<key>.png`
  （key 为 ultraman、seven、tiga、cosmos、taro、zero）即可覆盖代码绘制的头像。
