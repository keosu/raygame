# FORGE / The last delivery

支持根工程构建和独立消费 RayGame SDK 的工业科幻横版平台参考游戏。你是一名携带最后火种的
信使，要穿过铸造厂、能源通道和反应炉核心，收集全部能源电池并重启机器。
三关分别围绕传送带、冰面与气流、坍塌平台与炮塔展开；每关出口需要全部电池。

| 关卡 | 场景与移动手感 | 敌人与应对 |
| --- | --- | --- |
| 铸造厂 | 顺向和逆向传送带会带动脚下的角色，箭头显示方向；跳离传送带即可摆脱输送 | 带尖刺的装甲巡逻车沿地面往返，必须跳过，不能靠冲刺穿过 |
| 冷却管廊 | 冰面起步、转向和刹停更慢；冷却风机形成上升气流，托举角色抵达高处平台 | 空中巡逻无人机与少量周期激光；提前减速，把空中移动和冲刺用于离开气流、避开危险 |
| 反应核心 | 开裂平台踩踏后约 0.65–0.8 秒坍塌，2.5 秒后恢复；红色缩短条显示剩余支撑时间，空框与填充条显示恢复进度 | 炮塔先蓄能闪光并显示虚线射向，再发射水平能量弹；跳跃或冲刺躲避，也可借实体平台掩护 |

普通钢地面保持原手感；冰面的抓地倍率为钢地面的 0.12–0.16，空中操控不受影响。
炮塔每轮开火前有 0.75 秒预警，射程有限，轮次之间留有安全间隔。
三关保留二段跳、墙跳、升降台和检查点，机关提示支持中英文及触屏操作。
原有存档可以继续使用，电池索引和检查点顺序未变；重生时坍塌平台恢复完整。

玩法与完整流程：

- 移动加减速、跳跃宽限和输入缓冲、按住/轻点的可变跳高、空中二段跳、
  墙跳/滑墙、冲刺与冷却；单向平台支持下落，升降台会带着角色移动。
- 主菜单、新游戏确认、继续游戏、暂停、检查点重试、逐关结算和最终通关。
  关卡进度、已收集电池、设置和最佳时间保存到用户目录；保存失败给出提示。
- 暖色铸造车间、蓝色冷却管廊、紫金反应核心三种视差场景；带护甲、发光背包、
  头盔和围巾的信使，关节动画、灯光、粒子、落地反馈和可关闭的
  镜头震动；原创合成音效及 32 秒工业氛围循环配乐。
- 键盘、鼠标和手柄可操作主流程；以 1280×720 为设计基准等比扩展画布，
  宽屏显示更多场景、菜单居中、触屏按钮锚定边缘，避免固定 16:9 留边。
  GUI EXE 不弹控制台，标题栏继承引擎的 Windows 原生主题，诊断保留到文件。
- 支持简体中文 / English 即时切换，默认中文；菜单、HUD、三关提示和结算
  完整翻译。中文使用 Noto Sans SC 的专用字重子集，保持清晰的标题与正文。

## Windows 构建与运行

在 RayGame 仓库根目录使用 Visual Studio 2022 和 xmake 3.1.1：

```powershell
xmake f -p windows -a x64 --toolchain=msvc -m release --build_examples=y --build_tests=y -y
xmake build -y
xmake run forge_runner
xmake build -y forge_tests
xmake run forge_tests
```

默认构建包含 FORGE，也可用 `xmake build forge_runner` 单独构建。EXE 位于
`bin/forge_runner/forge_runner.exe`，其 `assets/`、README 和第三方说明自动同步。
Linux 在根目录配置 `xmake f -p linux -m release -y` 后使用相同的构建/运行命令，
输出文件名不带 `.exe`。`forge_tests` 默认把测试数据放在 build 目录，游戏与
测试分别受根工程 `build_examples` / `build_tests` 选项控制。

独立 SDK 构建使用 PowerShell 7：

```powershell
pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify
./examples/forge_runner/build/windows/x64/release/forge_runner.exe
```

脚本在独立配置中构建/安装 SDK，然后构建游戏，不改 IDE 当前桌面配置。
Debug 使用 `-Mode debug`。`-Verify` 运行逻辑测试和隐藏窗口图形 smoke，
日志和截图位于 `build/forge/<mode>/`。游戏运行需要输出目录中的 `assets/`。
MSVC Release 使用动态 C 运行库，目标 Windows 机器需要 Visual C++ 2015–2022
x64 Redistributable。CI 的独立 Forge 下载包根目录直接包含 EXE、assets 和说明；
Windows 总游戏包中的 `forge_runner/` 也包含完整游戏。Linux 下载包解压后需
运行 `chmod +x forge_runner` 恢复可执行权限。
没有可用 OpenGL 显示环境时可加 `-SkipGraphics`；Windows CI 使用此选项，仅运行
构建、逻辑和内嵌图标检查，不把图形 smoke 标为通过。

也可以把整个 `examples/forge_runner` 复制到另一个目录，使用已安装的 SDK。
先切换到复制后的游戏目录，再运行：

```powershell
xmake f -p windows -a x64 --toolchain=msvc -m release --raygame_sdk=C:/SDK/raygame/windows-x64-release -y
xmake build -y
xmake run forge_runner
```

独立入口只引用安装后的 `<raygame/...>` 头文件和库，不引用引擎源码、
其他示例或仓库测试。根工程入口链接当前 `raygame` 目标，在 build 目录生成
相同命名空间的头文件布局；两者共用 `targets.lua`。字体/关卡/音频/许可均
随本目录保存。Windows/Linux CI 均构建根工程游戏并运行逻辑测试，Windows
还验证独立 SDK 构建。Android 通过根工程构建，独立安装 SDK 的入口仍面向桌面。
Linux 实机运行和发行包签名尚未验收。

## Android APK（实验性）

已加入 Android 8.0+ / API 26、ARM64、OpenGL ES 2.0 的 NativeActivity 目标，
target SDK 35。使用与其他示例相同的隔离构建、签名和校验脚本：

```powershell
pwsh -NoProfile -File scripts/build-android.ps1 -Game forge_runner -Mode release
```

输出为 `bin/android/arm64-v8a/release/forge_runner.apk`，包名 `org.raygame.forge`。
不指定 `-Game` 会生成全部五个游戏；CI 的 `raygame-android-arm64-release`
产物包含 Forge APK 和校验报告。工具安装与签名说明见 [Android 文档](../../docs/ANDROID.md)。

手机采用横屏及反向横屏、1280×720 基准的 Expand 等比画布、系统全屏；
新设置默认 60 FPS。画布填满原生 Surface，刘海/系统手势安全区仍需实机确认。
触屏操作支持同时按住多个按钮：

- 左下左右箭头移动；右下“跳跃”支持按住跳高、再次点击二段跳，“冲刺”发动冲刺。
- 按住左下的下箭头，同时点击跳跃，可穿过单向平台下落。
- 右上暂停按钮打开暂停菜单；系统返回键切换暂停、退出设置或返回主菜单，
  在主菜单再次返回会保存并退出。菜单、制作信息和新游戏确认均可触屏操作。
- 设置行左半侧减少、右半侧增加；显示模式固定全屏。保留外接键盘/手柄操作。

关卡、翻译、字体、音频和许可放在 APK 的 `assets/forge/`，第三方说明也随包提供。
文本资源通过 AssetManager 读取；存档和日志仍使用应用私有文件目录
`files/raygame.forge/`，不需要外部存储或网络权限。

已验证 APK 交叉编译、签名、对齐、原生入口、图标和资源完整性；触点 ID 重排、
多指操作、固定步短按缓存、菜单/失焦取消有桌面逻辑和隐藏窗口 smoke 检查。
**尚未安装或启动 Android 真机/模拟器**，启动、音效、触感、后台/锁屏恢复、
刘海/手势区域与性能耗电仍需设备验收；桌面触屏模拟不代表这些检查已通过。

## 操作

| 动作 | 键盘/鼠标 | 手柄 |
| --- | --- | --- |
| 移动 | A/D 或左右箭头 | 左摇杆 |
| 跳跃/二段跳 | Space / W，按住跳得更高 | A / 下方按钮 |
| 冲刺 | Shift / X | X / 左侧按钮 |
| 穿过单向平台 | S/下 + 跳跃 | 摇杆向下 + A |
| 暂停/返回 | Escape / P | Start |
| 重试检查点 | R，或暂停菜单 | 暂停菜单 |
| 全屏 | F11，或设置菜单 | 设置菜单 |
| 菜单 | 上下、Enter，或鼠标点击 | 方向键、A |
| 调整设置 | 左右，或点击设置行 | 方向键左右 |
| 制作信息 | 主菜单 C | — |

中英文切换：主菜单或暂停页进入“设置”，选择“语言 / LANGUAGE”，使用左右键、
手柄方向键或鼠标点击切换。语言立即生效并保存；重新启动保留选择，途中切换
不会重置关卡。旧设置存档没有语言字段时默认中文，仍保留音量等已有设置。

正常用户存档与日志位于 `%LOCALAPPDATA%/raygame.forge/`。
`settings.sav` 与 `campaign.sav` 使用引擎版本化校验和/原子写入与备份恢复。
关卡途中保存后继续会从最近检查点恢复，保留已收集电池；过关后继续进入下一关。
最终通关会保留关卡纪录，并可以开始新一轮。默认用户存档不会被验收脚本改动。

## 数据与资源

`assets/levels/*.level` 是有版本、字段范围和数量检查的纯文本关卡。当前三关使用
version 2，加载器仍兼容不含新机关的 version 1 文件。
文本读取兼容 LF / Windows CRLF、UTF-8 BOM、空白行与缩进后的 `;` 注释；名称和
提示末尾的空白会被去除，避免影响翻译匹配。解析错误会记录文件路径及原始行号。
`solid`、`ledge`、`moving`、`hazard`、`laser`、`drone`、`cell`、`checkpoint`
和 `sign` 定义内容；可以修改平台位置/移动周期和电池分布，无需改引擎代码。
当前 campaign 固定为三关，增加第四关需要扩展游戏的关卡索引与进度 schema。

version 2 新指令如下，距离使用游戏单位、时间使用秒：

| 指令 | 参数 | 行为 |
| --- | --- | --- |
| `ice` | `x y width height grip` | 实体冰面，抓地倍率 0.05–1，控制水平起步、转向和刹停 |
| `belt` | `x y width height speed` | 实体传送带，输送速度绝对值不超过 240，负数向左；仍受墙壁与危险区域约束 |
| `fragile` | `x y width height breakDelay rebuildDelay` | 单向开裂平台，警告 0.2–3 秒、消失 0.5–10 秒；角色占据平台区域时推迟恢复 |
| `airflow` | `x y width height forceX forceY` | 气流区域；水平加速度绝对值不超过 2000，垂直为 -4000–0，负数向上；冲刺时不受气流偏转 |
| `crawler` | `x y range period` | 底盘中心、左右巡逻半径与完整往返周期；脚底在 y+16，完整路线必须位于连续的静止平台上 |
| `sentry` | `x y direction range period phase` | 炮口、方向（-1/1）、射程（80–900）、周期与非负相位；弹速 360，周期至少覆盖预警、飞行及 0.2 秒间隔 |

这些参数属于 Forge 的角色控制与机关模拟，不修改引擎刚体的全局物理参数。

`tools/generate-assets.py` 生成原创 ICO/PNG、灯光纹理、八种音效和配乐；
需要 Pillow，正常构建使用已提交文件。角色和场景由 `Presentation.cpp` 绘制，
便于直接调整动画、材质和布局。字体及各自的 SIL OFL 许可在 `assets/fonts`。
第三方说明见 `THIRD_PARTY_NOTICES.md`。

`tools/generate-android-icons.py` 从同一 Forge 图标设计生成 Android 普通、
自适应和主题图标，资源位于 `android/res/`；正常构建使用已提交资源，无需 Python。

`assets/locales/zh-CN.tsv` 使用英文原文和简体中文译文的 TAB 分隔目录，
同一目录覆盖游戏 UI 和三关数据的名称/说明/标牌；未翻译的自定义关卡文字
保留原文。增加中文字符时须用 `tools/prepare-chinese-fonts.py` 更新字体子集，
来源/重现步骤见 `assets/fonts/README.md`。当前字体包含游戏文本需要的字形，
不是任意中文输入/动态字形缓存或复杂文字塑形系统。

## 验收边界

逻辑测试包括三关通过正常移动/跳跃/冲刺命令收集全部电池并到达出口的路线
搜索，未使用传送或修改碰撞几何；还检查跳高、二段跳、冲刺、单向/移动平台、
危险穿透、检查点和进度恢复。新增检查覆盖冰面惯性、传送带方向/碰撞/跳离、
气流进入与离开、坍塌/恢复/重生、巡逻车躲避、炮塔预警/射线遮挡/冲刺保护、
旧关卡兼容与错误数据拒绝，以及所有检查点跨多个射击周期的安全性。
图形 smoke 检查菜单/关卡/结算渲染、机关警告/恢复画面、暂停、
存档、4:3/超宽屏坐标及退出资源释放，并注入键盘/鼠标/手柄快照走通设置、
暂停恢复、重试、继续游戏与覆盖存档确认。每次验证在隔离目录使用默认起点，
不操作正常用户存档。截图另作人工视觉检查。

这些检查不替代人工完整通关、真实手柄/多 DPI/多显示器、音效主观试听和
长时间运行验收。中英文切换与中文字形已有回归和截图检查；按键重绑、完整
无障碍选项及商用发行渠道尚未实现。

## 触屏按钮与下落

安卓默认使用放大的圆角图标按钮：左右箭头移动、上箭头跳跃、闪电冲刺和暂停。
按住跳跃可跳得更高，再次按可二段跳；冲刺按钮底部显示冷却进度。设置页可调
操作按钮大小（100% / 120% / 140%，默认 120%）。绘制和触摸区域使用相同布局。

下落仍用于穿过单向平台：移动手指向下拉，再用另一根手指按跳跃。
独立下落按钮默认隐藏，可在设置里打开；键盘 S / 下方向 + 跳跃仍可用。
菜单和触控复用框架 ButtonPainter 的阴影、高光、按压和选中样式。
