# Agent 工作状态

更新时间：2026-10-13。

## 目标与约束

按 `PRODUCT_READINESS_REVIEW.md` 逐步产品化 RayGame。当前推进 M0，继续保留
GameObject/Component 和 raylib，维持原四个游戏及 FORGE 可用。用户明确要求持续保存 agent
状态；每个工作包结束及交接前更新本文件。仓库 `AGENTS.md` 已固化该规则。

用户偏好：中文交流，直接推进已授权工作，验证后提交；示例无控制台但保留
文件日志，标题栏跟随 Windows 主题，游戏适配窗口比例。未授权创建子 agent。

## 分支工作包：Ultra Guard 示例（仅在 `example/ultra-guard` 分支，不合入 main）

用户要求把外部塔防游戏作为示例放到独立分支，不推送 main。最初基线 main `d942764`；
用户要求后已 rebase 到 main `095bbea`（只有本文件冲突，保留双方条目），并同步为外部
仓库 `9cef57b` 的游戏版本。游戏为奥特曼同人（第三方版权/商标），README 顶部有非官方
声明；公开仓库的分支同样公开可见，已向用户说明。是否合入 main 由用户另行决定。

- `examples/ultra_guard/`：11 个 .cpp + 头文件（与外部仓库 `9cef57b` 的 src 一致，仅改
  资源目录为 `bin/assets/ultra_guard/`、数据目录 `raygame.ultra_guard`、日志控制台跟随
  `RAYGAME_QUIET_EXAMPLE`、Android 包名 `org.raygame.ultraguard`）；assets（字体子集与
  OFL、WAV 音效与 OGG 音乐、`levels/` 关卡文件、`skins/neon` 示例皮肤）、android/、
  resources/windows/、tools/（字体/音频/图标/示例皮肤脚本，路径改为相对示例目录、本仓库
  `scripts/subset-font.py`、`bin/` 与 `build/`）、README（关卡与皮肤说明）。
- 同步版本新增：文本关卡（`Levels.cpp`，`文件:行号` 报错、关卡页分页、星级按 id 存档）、
  皮肤（`Skins.*`，调色板/头像/怪兽帧条/地图地面，标题页切换，`--export-skin` 模板、
  `--skin`）、BGM 改 Ogg Vorbis、留边模糊填充（rebase 后 main 已有 `MarginStyle`，不再删除）。
  外部仓库独有的 xmake Android 打包（`set_config("ndk_sdkver")`、APK 检查规则）未带入，
  示例沿用本仓库 `examples/xmake.lua` 与 `scripts/build-android.ps1`。
- `examples/xmake.lua`：示例名单加入 `ultra_guard`；源码改为 `<name>/*.cpp`（其余七个
  示例目录都只有 main.cpp，行为不变）；README 随资源复制。`scripts/build-android.ps1`
  的 -Game 选项与 all 列表加入 `ultra_guard`。`examples/README.md` 加一行。

### 通过的检查

同步版本（rebase 后，独立 worktree）：

- Windows/MSVC x64 Release 全目标构建；454 引擎逻辑、图形 smoke 0 failures；
  clang-format 47 文件与 `git diff --check` 通过。
- 示例 `--balance` 报告与外部仓库逐字节一致（9 局胜 8）；`--capture` 经典与 `--skin neon`
  各 15 张截图正常（含开头的皮肤切换往返）；`--export-skin` 输出 17 个文件；日志确认
  从 `bin/assets/ultra_guard/` 读取 3 个关卡、neon 皮肤与 OGG 音乐。
- 示例 `tools/make_font.py` 重新生成的字体与 charset 与外部仓库逐字节一致。
- `build-android.ps1 -Game ultra_guard` 通过全部 APK 检查（4,100,925 B）。

### 未运行

Debug、Linux/macOS（CI 只在 main 推送/PR 或手动触发时运行）、Android 设备、人工试玩。

## 最近工作包：弹道命中记录的地址复用问题（已修复并本地验证）

基线 `d945ebc`。外部塔防游戏的平衡机器人在同一种子下偶发结果不同。原因：`Projectile`
用裸 `Collider*` 记录本程已命中的对象；怪兽死亡后新怪兽的碰撞体可能分配在同一地址
（Windows 堆的块选择不完全确定），长寿命的穿透/回旋弹会把新目标当成已命中而穿过。
现在命中记录保存所属组件的 `weak_ptr`，地址相同且原对象仍存活才视为已命中；没有
所属对象的碰撞体仍按指针记录。新增回归测试：静止穿透弹位置上反复销毁并重建目标
40 次，每次都须命中（去掉修复后 3/3 次失败）。顺带对 `Display.h`（上一包遗漏）与新测试
运行 clang-format，只有换行变化。

通过：Windows/MSVC x64 Debug/Release 全构建；两模式各 3 次 **454 引擎逻辑**零失败，
130 Forge、37 campaign 零失败，`smoke` 两模式 0 failures；clang-format 19.1.5 的 47 文件
检查通过，`git diff --check` 通过。外部游戏平衡机器人连续 4 次报告逐字节一致。
注意：用 `Copy-Item` 恢复头文件会保留旧修改时间，xmake 不会重新编译（本包曾因此
误判一次失败），恢复后需更新时间戳。未运行：Linux/macOS、Android。

## 上一工作包：Fit 画布留边的填充方式（已实现并本地验证，`809343d`）

基线 `d942764`。用户希望窗口比例不同时，四周留边不再是纯黑。`Display` 新增
`MarginStyle::{Solid, BlurredCanvas}`、`SetMarginStyle(style, brightness)`、`SetMarginColor`；
BlurredCanvas 把当前画布经三级 1/4 缩小的离屏纹理模糊后按画布比例铺满窗口并调暗，
只在实际存在留边时计算。`PresentOpaque` 增加可选 tint；`PrepareMarginBlur` /
`DrawMarginFill` 公开供自定义呈现与测试。默认仍为原来的深色纯色，现有游戏外观不变。
README 渲染一节补充说明。

通过：Windows/MSVC x64 Release 全构建、453 引擎逻辑、图形 smoke 0 failures（新增：模糊
留边延续画布颜色并按亮度调暗、纯色留边使用指定颜色）；外部塔防游戏 1100×860 窗口的
合成截图确认上下留边为模糊场景色。未运行：Debug、Linux/macOS、Android 设备上的
实际效果与开销（每帧三个小纹理 pass）。Ultra Guard 示例只在 `example/ultra-guard`
分支，其状态记录也在该分支。

## 上一工作包：作为子模块嵌入与外部项目 Android 打包（已实现并本地验证）

基线 `0279a78`。外部塔防项目为支持 Android 改为 `includes` 本仓库 `xmake.lua`，
暴露三处只在“raygame 不是根工程”时出现的问题，本包修复：

- `xmake.lua`：检测是否为根工程；被包含时不调用 set_project/set_version，
  `build_examples`/`build_tests` 默认关闭（根工程仍默认开启，CI 不受影响）。
- `xmake/rules/android-tts.lua`：Java 桥接源码与依赖文件按规则文件位置解析，
  不再用 `os.projectdir()`（外部项目中会指向错误目录）。
- `scripts/build-android.ps1`、`scripts/verify-sdk.lua`：所有 xmake 调用加 `-P` 固定
  本仓库。此前在子模块目录运行会选中父工程：Android 构建把 APK 写进父工程 bin
  后找不到产物；SDK 检查的 install 找不到 raygame 构建。
- `docs/ANDROID.md` 新增“外部项目以子模块使用 raygame 并打包 APK”。

### 通过的检查

- 根工程桌面 Release 配置列出全部示例/测试目标，全构建通过，453 引擎逻辑通过。
- 在子模块目录内直接运行（此前失败的场景）：`build-android.ps1 -Game engine_lab`
  成功并通过脚本全部 APK 检查（含 TTS 桥接）；`verify-sdk.lua` 通过。
- 外部塔防项目：桌面构建只含 raygame + 游戏两个目标；Android ARM64 Release APK
  构建、签名、16 KiB 对齐、ELF 入口、静态运行库、TTS dex、资源 SHA-256 检查通过。

### 未运行与下一步

未运行 `tests/android_tools.ps1`（未改发现逻辑）、Linux/macOS、Debug Android、
任何 Android 设备/模拟器（本机无设备与模拟器）。下一步：推送后检查 CI。

## 上一工作包：特效层、弹道组件、拖尾粒子与主题按钮（已实现并本地验证）

基线 `3681079`，开始时工作区干净；提交前 rebase 到并行会话的 `54a4cf0`（仅 Android
工具/CI/文档，与本包代码无重叠，状态文档冲突保留双方条目）。上一包迁移后塔防游戏仍需自写的部分是弹道、
光线/冲击环/飘字、拖尾火花与按钮样式；用户要求把这些贡献给框架。游戏不作为
示例提交（用户决定，涉及第三方 IP）。

- `include/Effects.h`：`EffectLayer` 组件，`Beam`/`Ring`/`Text`（参数结构体），
  随模拟时间老化（暂停冻结），光线与冲击环叠加混合，飘字带出现弹跳与描边；
  非有限坐标、空文本、非正寿命忽略；`RingRadius` 供玩法查询冲击波前沿。
- `include/Projectile.h`：`Projectile` 组件，`Straight/Homing/Boomerang/Lob`；
  命中用场景 PhysicsWorld 的 `OverlapCircle`（层掩码/触发器过滤），`pierce` 每程
  每个碰撞体一次，回旋返程重置；钩子 `onHit/onArrive/onStep/onDraw`；到达、非穿透
  命中或 `maxLife` 后自毁，`Finish()` 可由回调提前结束；渲染位置可插值。
- `ParticleEmitter::alignToVelocity/stretch`：粒子朝向速度，贴图拉长为条带，无贴图
  画线；默认关闭。
- `UI`：`Button(rect, label, ButtonStyle, size, enabled)`（复用 ButtonPainter，禁用态
  仍捕获指针但不点击，按下时文字下沉），`Panel(rect, fill, border, roundness)`，
  `Text(x, y, text, size, color, align, heading, outline, outlineColor)`；原 `Button`
  改为调用新重载，外观不变。`RayGame.h` 与 SDK 单头文件测试包含两个新头。
- README 新增“Effects, projectiles and themed UI”一节。

### 通过的检查

- Windows/MSVC x64 Debug/Release 全目标构建；各 **453 引擎逻辑**（新增 16 项：特效
  老化/无效输入/冲击半径、四种弹道/穿透/回旋两程/追踪移动目标/抛物到达/寿命/
  onStep、拖尾朝向随速度转向）、**130 Forge**、**37 campaign** 零失败。
- `smoke` 两模式 0 failures（新增：禁用主题按钮不点击且绘制禁用底色、UI::Text 描边），
  `tts_smoke` 两模式 0；Engine Lab、Marble Temple、Garden Guard、Forge `--smoke` 两模式
  退出码 0（各模式先重建）。clang-format 47 文件通过；独立副本 `verify-sdk.lua --graphics`
  通过（含新头文件单独包含）。
- 外部塔防项目改用 EffectLayer/Projectile/拖尾/主题按钮后编译、平衡机器人、截图巡览正常。

### 未运行与下一步

未运行 Linux/macOS、Android 构建与设备、人工试玩。EffectLayer 文字依赖 UI 字体，
未设置字体时退回 raylib 默认字体（不含中文）。下一步：推送后检查 CI。

## 并行工作包：Android 构建工具自动发现（并行会话完成，已推送 54a4cf0）

用户反馈其他机器 Android 脚本报错，指出假定了 NDK 版本。原脚本默认只查找
`ndk/28.2.13676358`，Build Tools 也只用 `35.0.0`；虽然支持 `-NdkRoot`，
未设置环境变量的其他版本无法自动发现。用户尚未提供另一机器报错与安装版本，
本包修复已确认的固定目录依赖，不把它当作已复现用户的全部报错。

- 新 `scripts/android-tools.ps1` 独立负责发现：显式 NDK → 有效环境变量 → 所选
  SDK 下最新完整稳定版 `ndk/*` → `ndk-bundle`，从 source.properties 读版本并
  数字排序；跳过预览与未完整安装。显式坏路径报告缺失文件，不静默回退。
- Build Tools 自动选最新完整稳定版 >=35.0.0，新增 `-BuildToolsVersion` 可固定，
  35 是 16 KiB ZIP 对齐工具下限，SDK platform 35/target 35 仍为项目要求。
  JDK 支持 PATH 中 javac 的安装根；脚本打印最终路径/版本，校验报告记录两版本。
- CI 仍固定 r28c/Build Tools35，增加自动发现测试；文档区分可发现版本与实际
  构建兼容性。测试以 build/ 临时目录模拟布局，无下载/编译/设备操作。
- 开始基线 `f81e588`、工作区干净。验证期间远程新增 `3681079`，已安全 stash
  本包改动、pull --rebase 快进后恢复，合并后的 Android 与 Windows Release 已重验。

### 本轮已通过

18 项自动发现检查：只有 r27/Build Tools36、多版本数字排序、预览/不完整目录、
显式覆盖、两个 NDK 环境变量/失效提示、外部路径/空格路径、ndk-bundle、缺 SDK/
Build Tools 与旧版本错误提示。本机默认发现后八 Release APK 构建/签名/对齐/
资源/TTS 桥接校验通过。第二套真实 SDK 自动选 Build Tools36，Engine Lab 重新
编译/打包/完整校验通过；调用者 JAVA_HOME/PATH/XMAKE_CONFIGDIR 与 Windows
配置 hash 保持，坏 NDK 路径失败也保持环境。PowerShell 语法/actionlint/diff 检查通过。
合并 `3681079` 后八 APK 重建/完整校验通过，Windows/MSVC Release 八示例构建
与 437 项引擎逻辑零失败；根桌面配置仍为 Release/MSVC/ccache=n。

### 未运行与下一步

测试日志 `build/android-tools-*`，最终 APK 验证日志
`build/android-tools-merged-all-release.log`、报告 `build/android/verification-release.json`
含八包；Build Tools36 Lab 日志 `build/android-tools-sdk36-lab.log`，模拟测试
`build/android-tools-final-tests.log`。只有 NDK r28c 在本机真实安装，r27/r29 的发现
使用模拟目录，未声称其原生编译通过；真实 Build Tools35/36 已分别验证。
未安装/运行 Android 设备、未下载其他 NDK，新 CI 尚待推送；工具版本兼容性以
构建/16 KiB 校验为准，脚本不自动安装。桌面 Debug/图形检查本包暂未重跑。
下一步：提交 `Discover installed Android NDK and Build Tools versions`，沿用用户此前
验证通过后推送的授权普通推送、检查 CI；另一机器可直接运行脚本重试。

## 上一工作包：塔防游戏迁移暴露的粒子/纹理缺口（已实现并本地验证）

基线 `f81e588`。本检出在本包进行中被并行会话 `pull --rebase` 快进（`5531c8e` →
`1ba4c61`）；本包改动只含自身 hunk，提交前已 stash/rebase 到 `f81e588`。
用户要求外部塔防项目改用上一包的新接口并评估能否大幅简化开发；迁移中发现
三处阻碍用框架组件替换自写实现的缺口，本包只补这三处。

- `Particle` 保存发射时的 `fromColor/toColor`：此前每帧按发射器当前颜色重算，
  改发射器颜色会给已存活粒子整体换色，单个发射器无法复用于不同颜色的命中/爆炸。
  示例与 Lab 都只在初始化时设色，外观不变。
- `ParticleEmitter::radialVelocity/speedMin/speedMax/spreadAngle`：沿物体旋转方向
  的张角内随机方向、按速率范围发射；原矩形速度范围会产生方形爆炸轮廓。默认关闭。
- `ResourceManager::CreateTextureFromImage(name, image, mipmaps)`：程序生成图像
  （柔光、烘焙头像）成为受管 TextureHandle，可选 mipmap + 三线性；同名复用，
  无效图像得到 Failed 句柄。此前只能生成纯色/硬边圆。
- README 粒子节补充；更正“远程并行工作包”中的检查计数（见该节）。

### 通过的检查

- Windows/MSVC x64 Debug/Release 全目标构建；各 **437 引擎逻辑**（新增 3 项：
  多色复用、径向方向、径向速率范围）、**130 Forge**、**37 campaign** 逻辑零失败。
- `smoke` 两模式 0 failures（新增 CreateTextureFromImage mipmap 与无效图像检查），
  `tts_smoke` 两模式 0 failures；Engine Lab、Marble Temple、Garden Guard、Forge
  `--smoke --data <隔离目录>` 两模式退出码 0（Debug/Release 各自重建后运行，
  两模式共用 `bin/`，须先重建对应模式）。
- 独立副本 `verify-sdk.lua --graphics` 通过；clang-format 19.1.5 默认 47 文件通过。
- 外部塔防项目迁移后编译、`--balance` 机器人、`--capture` 截图巡览正常。

日志：`build/ext/`（`*2-*`、`*3-*`、`smoke-{debug,release}-<name>/`）。

### 未运行与下一步

未运行 Linux/macOS、Android 构建与设备、人工试玩与音频试听。下一步：推送后检查 CI。

## 上一工作包：远程引擎扩展 rebase 与审查（已验证并推送）

用户要求 pull --rebase、解决冲突并评价远程提交，随后授权验证通过后推送。
开始时工作区干净，main 为 `3d6600a`，本地三个提交在 `0fc15f8` 上。
先创建 `backup/before-rebase-20261013-3d6600a`，pull 获取远程 `e196a10`
（Add engine features found missing while building a tower defense game）。

- rebase 已完成；推送前 fetch 又获取仅更新 CI 记录的 `5531c8e`，一并 rebase。
  最终保留三个本地包：TTS/Lab `b8781f9`、campaign `45ed663`、画质/菜单/触控
  `6abc0f4`。解决 Display、UI、GameEngine 主循环、测试入口、
  格式脚本和本文档冲突；保留远程扩展与本地 TTS/超采样/Unicode/按钮功能。
  `git range-diff` 已检查，远程与本地原有检查均保留。
- 审查确认 UI 捕获缓存上一帧悬停 bool，移入/移出 HUD 后的第一帧会误判，
  ResetInteraction/失焦也未清除它。改为保存控件区域并用当前输入查询，
  暂停/切场景/Shutdown 清理。自定义 RenderDepth 返回非有限值此前未统一处理，
  现由 Scene 构建排序键时统一视为零，仍只读取一次。
- Lab 显式设置透明 Cyan endColor，保持原纯 alpha 淡出；Lab 图形测试改为
  正常引擎 Render 路径，覆盖 UI BeginFrame/EndFrame；冲突代码格式已修整。
- 新增七项行为回归；修复前 `429 passed / 5 failed`，修复后引擎 `434 passed / 0 failed`。

### 本轮通过的检查

- Windows/MSVC x64 Debug/Release 八示例完整重建（ccache=n）；各 **434 引擎**、
  **130 Forge**、**37 campaign** 逻辑通过。原四示例 graphics smoke、Forge、
  Lab、Marble、Garden 图形/输入 smoke 两模式零失败；原生静音 TTS 两模式零失败。
  图形覆盖离屏恢复、不透明呈现、叠加粒子、真实三声部音效/冷却/停止、
  菜单/语言/设置/多比例/拖种植/存档；已查看新的中文主菜单和拖种植截图。
- SDK 两模式安装/搬迁/单头文件/逻辑/隐藏图形/配置拒绝通过；Forge 独立
  Release 构建、130 逻辑与图标通过，其独立图形入口使用 `-SkipGraphics`。
  根 Release EXE 图标与 Forge 图标通过。
- 八 ARM64 Release APK 关闭缓存完整重建，签名/16 KiB ZIP 与 ELF 对齐/
  入口/资源 hash/TTS dex 与 manifest 校验通过；沿用原密钥，未安装启动设备。
  日志 `build/rebase-android-no-cache-{config,build,verify}.log`，报告
  `build/android/verification-release.json`。首轮缓存构建仅作辅助，最终以上述为准。
- clang-format 19.1.5 默认 47 文件、actionlint、UTF-8/LF、`git diff --check` 通过。
  `5531c8e` 仅改变状态文档：第二次 rebase 后与验证时的实现树逐字节一致。
  SDK 日志 `build/sdk-validation/windows/x64/{debug,release}`，本包其它日志和
  图形产物 `build/rebase-*`；根配置 Windows/MSVC x64 Release、examples/tests 开启、
  GUI 子系统、ccache=n。
- 原远程功能提交 `e196a10` 的 CI run 38418657798 与文档提交 `5531c8e` 的
  run 38419374411 均成功，已通过 GitHub CLI 查询；这是原提交证据，与合并后检查分开。
- 本包代码提交 `1ba4c61` 已普通推送 origin/main，未 force push；新 CI
  [run 38421676802](https://github.com/keosu/raygame/actions/runs/38421676802)
  六个 job 全部成功：Windows/Linux Debug/Release、C++ formatting、Android ARM64
  八 APK。本地 main 与远程同步、工作区干净，原提交备份分支保留。
  本次补记只更新状态/评审文档，不改变上述已验证的程序代码。

### 未运行、限制与下一步

审查未发现剩余阻塞问题；上述两处缺陷已修复并有失败到通过的证据。
粗筛候选对保留注册序，但因之前解算才形成的重叠下一步报告；带半径 SegmentCast
对盒体按方角扩张，是保守近似，会在角落提前命中。新 API 有用，但尚无性能基准，
不承诺粗筛在密集碰撞场景的收益；UI 捕获也不替代完整焦点/模态/拖放系统。

未在本地运行 Linux/macOS、本包新 Android Debug、设备/模拟器、人工音频试听、
性能基准、多 DPI 矩阵或长稳。Android 继续只构建校验、不安装启动。
提交主题 `Fix UI capture and custom depth after rebasing engine extensions`。
后续按 Android 设备反馈和实际性能测量推进，不把构建/逻辑通过表述为全部平台、
设备或性能验收。原备份 `backup/before-rebase-20261013-3d6600a` 留给用户按需处理。

## 远程并行工作包：引擎能力补齐（来自塔防游戏实践，已实现并本地验证）

基线 `0fc15f8`，开始时工作区干净。用户在外部项目（以子模块使用本仓库的奥特曼
塔防游戏）中发现框架高层组件不足，要求直接在引擎修复并推送。本包只加能力与
修正契约，保留现有生命周期、回调顺序与示例外观。

- `Display`：画布改为关闭混合贴到窗口（`PresentOpaque`），半透明绘制降低画布
  alpha 后不再混入窗口底色而变暗。新增 `OffscreenScope`，在 Render 中途烘焙
  纹理时挂起画布并恢复逻辑投影；`InFrame()` 查询状态。scope 不继承之前压栈的矩阵。
- 渲染排序：`Component::ySort` / 虚函数 `RenderDepth()`，同 layer、renderOrder 内按
  渲染 y 升序（2.5D 遮挡）；排序键每帧只读取一次，NaN 视为 0。
- `ParticleEmitter`：`startColor`→`endColor` 含 alpha 插值（修复 endColor 未生效、
  初始 alpha 被覆盖），新增 `endSizeScale`、`drag`、可选 `texture`（可见旋转）、
  `ParticleBlend::Additive`；`maxParticles` 在 Start 时预留并始终遵守，`GetParticles()`。
  三个使用粒子的示例（Space Shooter 两处）显式设置透明 `endColor`，保持原纯 alpha 淡出。
- `PhysicsWorld`：≥48 个碰撞体时按 x 扫描粗筛（`SetBroadphaseThreshold`），候选对
  按注册序处理，Enter/Stay 顺序不变；同一步中因先前解算才重叠的对下一步报告。
  新增 `OverlapCircle/OverlapBox/SegmentCast(带半径扫掠，近到远)/Raycast`，带层掩码
  与触发器过滤，不派发回调、不产生接触。非有限包围盒不参与裁剪，与全部候选配对。
- `GameEngine::SetTimeScale`（0–8，`GameTime::GetTimeScale` 同步）：固定步长不变，
  每帧追赶上限 8×ceil(scale) 步；droppedSeconds 现以模拟秒计。
- `UI::IsMouseOverUI/Capture/BeginFrame/EndFrame`：Panel、Button、ProgressBar 自动登记，
  引擎在每帧 Render 前后调用，Update 读到上一帧结果。
- `AudioManager`：`LoadSound(name, path, voices)` 用 raylib 别名实现多声部、
  `SetSoundCooldown` 节流、`SetChannelVolume(Sfx/Music)`、`StopSound`、
  `GetVoiceCount/GetPlayingVoices`；`PlaySound` 返回是否播放。单声部行为不变。
- `ResourceManager::CodepointsFromText`（UTF-8 校验，拒绝过长/代理/控制字符）与
  `SetFontOptions`（mipmap/过滤，重载后保持，卸载时清除）；通用
  `scripts/subset-font.py`。README 新增三节 API 说明，审查文档更新粒子条目。

### 通过的检查

- Windows/MSVC x64 Debug/Release 全目标构建；引擎逻辑与 **127 项 Forge 逻辑**通过，零失败。
  （更正：当时记录的“391 项（原 335 + 新 56 项）”计数有误；该提交在本机实测为
  **434 项**，其中 `tests/engine_extensions.h` 为 63 项，与 rebase 包记录一致。）
- 原四示例图形 smoke 两模式 **0 failures**，新增 GPU 检查：半透明画布不透明呈现、
  OffscreenScope 写入目标并恢复画布坐标、贴图叠加粒子像素值、音频三声部重叠/
  冷却/停止（本机有音频设备，已实际运行）。
- 独立副本中 `verify-sdk.lua` 及 `--graphics` 通过；`build-forge.ps1 -Mode release
  -Verify -SkipGraphics` 通过；clang-format 19.1.5 的 22 文件检查通过（新测试头纳入范围）。
- 外部塔防项目以子模块编译通过。
- 已推送 `e196a10`；远端 CI run 38418657798 全部通过：Windows/Linux Debug/Release、
  C++ formatting、Android ARM64 APKs（Linux 构建与 Android 打包由 CI 验证，本地未跑）。

日志：`build/ext/`（本仓库作为子模块时须用 `xmake -P .`，否则 xmake 会选中父工程；
`verify-sdk.lua` 内部未传 `-P`，因此在独立副本 `%TEMP%/raygame-sdkcheck` 中运行）。

### 未运行与下一步

未运行 macOS、Android 设备、Forge 图形 smoke（`-SkipGraphics`）、人工试玩与音频试听。退出接触按碰撞体地址顺序派发是既有行为（两世界间顺序不同），
本包未改。下一步：可考虑渲染组件注册/可见性裁剪、粒子曲线与
局部空间、UI 控件状态与输入路由（审查文档第 5、6 节）。

## 上一工作包：示例画质、菜单、拖拽与移动控制（已实现并本地验证）

基线 `da09343`。用户要求两款 campaign 的适配/菜单/中英文/设置、彩球质感、
花园拖种植，以及 Forge 放大触屏图标按钮。随后反馈启动 PAUSED、设置被上层
遮罩盖住、左下操作说明遮挡、全屏顶部内容缺失。本包未创建子 agent；Android
仍只构建/校验 APK，不安装或启动设备。

- 框架增加 `PointerGesture` 鼠标/单指捕获、拖动阈值、释放及失焦/映射取消，
  复用 TouchTracker；公开头文件由 SDK 导出。`ButtonPainter` 提供统一阴影、
  边缘、高光、按压/选中/禁用及跳跃/冲刺等图标，UI、campaign 和 Forge 共用。
- `Display::CanvasQuality` 增加 Smooth 两倍原生采样，保持 4096 边长限额与
  分配失败保留旧画布。UI 可用 Unicode 正文/标题字体；测量/绘制一致，Shutdown
  清空。campaign 中英目录和 Noto Sans SC 子集可重建，语言默认中文。
- 两款共享设置：中英、音效/音量、标准/精细、60/120 FPS、桌面窗口/全屏，
  即时保存；菜单等比居中缩放、鼠标/触屏/键盘导航。启动固定主菜单；从暂停
  打开设置后返回原暂停页。完整操作说明只在帮助页，游戏中仅状态与短暂反馈。
- 修复 `GameEngine::Render` 无条件叠加暂停框：关闭默认快捷键的自定义游戏
  暂停模拟时不再绘制引擎 PAUSED。Render 作为 protected 方法让图形检查走正常
  Run 相同的完整流程，新增实际像素回归，保留原示例默认暂停框。
  campaign 原先把居中 origin 作为 Camera2D target，产生反向平移；改为明确的
  正向矩阵平移/缩放，修复竖屏/非 16:9 全屏的顶部裁切与按钮绘制/命中错位。
- Marble 彩球仅颜色区分，使用原创柔边高光/反射/阴影纹理；三关神庙背景和
  青蛙发射器经超采样生成，轨道节点与模型一致。没有使用原作素材。
- Garden 种子/铲子可拖到格子，预览跟随指针、有效/无效格标色，触屏向上偏移；
  松手种植一次，外部/失焦/切页取消不扣费，落点阳光不会抢走种植操作。
  保留点击选择再点击种植。
- Forge 左右/跳跃/冲刺/暂停图标按钮放大，默认 120%，可选 100/120/140%；
  绘制与触摸同布局。独立下落默认隐藏，移动手指向下拉再用另一指跳跃可穿过
  单向平台；独立按钮可开启，键盘原操作保留。中文字体更新为 394 字符。

### 已通过的最终检查

- Windows/MSVC x64 Debug/Release 八示例完整重建，各 **371 引擎逻辑**、
  **130 Forge 逻辑**、**37 campaign 逻辑**通过；原四示例 graphics、Forge、
  Engine Lab、两款新游戏图形/输入检查均零失败。默认/自定义暂停像素回归
  加入后单独重建并重跑两模式 smoke，通过；六关普通命令逻辑通关保持通过。
- 两款图形检查走正常 Render，覆盖初始失焦仍停在菜单且无引擎叠层、菜单/
  暂停进入设置、全屏设置/完整顶部 HUD、全屏窗口往返、4:3/超宽/竖屏实际
  面板像素与输入对应、中英/质量切换、存档重启；Garden 另覆盖鼠标/触屏拖放、
  冷却/占用/成本、外部取消、失焦取消及阳光重叠。已查看最终代表截图。
- Release 正常启动（非 --smoke）两款游戏，等待资源初始化后主菜单正常；
  1280×720 原生客户区截图 `build/polish-normal-<name>/normal-menu-client.png`。
  仅向本次启动的 PID 发 WM_CLOSE，未终止用户进程。早期截图捕获过早/工具
  DPI 坐标不一致，已用线程 DPI awareness 重新捕获；不作为最终画面证据。
- 桌面 SDK Debug/Release 独立消费、单头文件、搬迁、配置不匹配拒绝与隐藏窗口
  图形检查通过；八 Windows Release EXE 的八尺寸图标通过。Forge 独立 SDK
  Release 编译、130 项逻辑与图标也通过，该独立入口使用 -SkipGraphics，
  Forge 两模式图形成绩来自根工程。
- 八 ARM64 Release APK 完整重建并验证签名、16 KiB ZIP/ELF 对齐、入口/静态
  运行库、图标、资源 hash、Java TTS 桥接/manifest；Marble **4,287,423 B**、
  Garden **1,423,680 B**、Forge **2,993,599 B**。保留原签名。三个受影响 Debug
  APK 关闭缓存完整重建/复验通过：Marble **7,248,831 B**、Garden **4,405,568 B**、
  Forge **6,299,071 B**；常规缓存的 Forge Debug 曾为 6,303,167 B，最终记录以
  无缓存新包为准。最后补全帮助页快捷键后，两款重建两模式并重跑图形和 APK
  校验通过；最终 Release 报告含八包，Debug 报告最后单独校验 Garden，
  其他两包的通过结果保留在对应日志；全部 RuntimeTested=false。
- 彩球/背景/发射器 PNG、四份 Campaign 字体/目录和两份 Forge 字体隔离重建
  逐字节一致；292/312/394 字符覆盖与背景轨道节点对照通过。clang-format
  19.1.5 的 46 文件、actionlint、修改文本 UTF-8/LF/末尾换行、Python 语法和
  `git diff --check` 通过。

早期为避开用户 EXE 锁建立 `build/polish-desktop` 副本，xmake 编译缓存带回了
该副本的绝对头文件依赖，造成根工程后续修改未重编译。本包已关闭 Windows
ccache 并完整重建两模式，以上最终成绩使用新产物，旧 polish 日志不作为最终
证据。Android Release 也关闭缓存重建并复验，结果与常规构建一致。
一次 Android 补验误配为 Windows、与图形检查争用 EXE，已改为显式 Android
配置后完成；不把该失败算作 Android 运行失败。

最终 Windows 日志 `build/polish-final-<mode>-*.log`；两款/Lab 图形在
`build/polish-final-<mode>-<name>/`，Forge 在 `build/polish-final-<mode>-forge/`。
SDK `build/polish-final-sdk-<mode>.log`，Android 最终无缓存日志
`build/polish-final-android-no-cache-*.log`，帮助页最后补验 `build/polish-help-*`，
Forge 独立 `build/polish-final-forge-sdk-release.log`，资源复验 `build/polish-assets/`。
根配置 Windows x64 Release、examples/tests 开启、GUI 子系统，ccache=n。

### 未运行、限制与下一步

未安装/运行 Android 真机/模拟器，未运行 Linux/macOS、新远端 CI、人工完整
通关/手柄/音频试听、多 DPI/多显示器矩阵或长稳。正常客户区截图不替代 DPI
矩阵验收。超采样的显存/帧时间、Android 安全区/GPU 恢复以及拖动/按钮实际
手感仍须设备测试；这次增加的是指针工具与按钮绘制，不是通用拖放对象系统。

本包提交主题：`Polish campaign menus, marble art and mobile controls`。用户可运行
`bin/marble_temple.exe`、`bin/garden_guard.exe` 与 `bin/forge_runner/forge_runner.exe`
体验菜单/全屏/拖放；手动安装同签名新 APK 验收按钮大小、下拉跳跃与拖种植。

## 上一工作包：祖玛与植物大战僵尸玩法示例（已实现并本地验证）

基线 `196c6d0`，开始时工作区干净。用户要求增加两个完整游戏示例。
提交主题：`Add Marble Temple and Garden Guard campaign examples`。
继续沿用 Android 只构建 APK、不安装或启动设备的约定。

- `examples/marble_temple` / 彩球神庙：三条不同曲线轨道、有限球链、瞄准发射、
  沿轨道切线插球、同色三连、闭合缺口连锁、当前/下一颗交换、每九发范围爆破。
  三关分别 45/57/69 颗供给球，速度和配色不同；清空胜利、到达终点失败。
  彩球有不同符号辅助辨识，投射物使用扫掠碰撞。
- `examples/garden_guard` / 花园守卫：五行九列，阳光拾取/生产、成本/种子冷却，
  向日葵/豌豆射手/坚果墙/冰花/樱桃炸弹/双发射手六种植物和铲子，普通/装甲/
  快速敌人。同排射击、减速、啃食、范围爆炸，每行一次割草机救场；三关各三波，
  分别 24/33/42 个敌人，清空全部波次胜利，耗尽救场后再次突破失败。
- 两款共用 `examples/common/CampaignGame.h`：菜单/说明/锁定选关、暂停/重试、
  逐关结算/最终胜利、自动保存解锁/最高分/音效开关。鼠标和捕获式触屏操作，
  失焦取消手势并暂停，Back/P 暂停、R 重试、F11 全屏。真实 Scene/Component
  驱动固定步规则与绘制；游戏模型独立于窗口。等比 Expand 画布居中固定玩法区域。
- 角色、场景、图标为原创代码绘制/SVG，短音效由标准库 Python 合成；共用字体
  保留许可。没有使用原游戏素材。Windows GUI 子系统、原生标题栏主题、文件日志
  和用户数据目录沿用引擎；smoke 必须显式指定隔离数据目录。
- 默认构建/Android/CI 扩至八个示例，Windows 产物包括新游戏 README，图标工具
  支持七个共用字体示例；增加 `campaign_tests`，格式范围扩至 40 个 C++ 文件。
  README、Android、SDK、TTS 和图标说明同步更新。本包未修改引擎实现。

### 通过的检查

- Windows/MSVC x64 Debug/Release 八示例构建，两模式各 **359 项引擎逻辑**、
  **127 项 Forge 逻辑**、**37 项新 campaign 逻辑**通过，零失败。原四示例 graphics
  smoke、Forge、Engine Lab 及两个新游戏图形/输入 smoke 两模式均 **0 failures**。
  最后菜单/输入调整后两款均重建并重跑两模式 smoke；最终根工程 Release 全构建通过。
- 新逻辑检查经普通发射/交换、阳光收集/种植命令实际完成六关，无瞬间胜利夹具：
  Marble 约 42.4/40.5/65.4 秒，Garden 约 121.3/128.2/136.2 秒。另覆盖插球顺序、
  缺口连锁、有限供给/不能提前胜利、扫掠碰撞、冷却、边界/成本、同排伤害/减速、
  炸弹自毁/范围、产阳光、阻挡/啃食、双发、割草机/第二次突破及结束后输入拒绝。
  自动通关不代表人工难度或手感验收。
- 新 smoke 覆盖菜单/说明/锁定、触屏发射/换色/种植/收集/铲子、暂停、失焦取消、
  多指捕获不重复发射，以及夹具触发的胜败/下一关/重试/解锁流程；这些夹具不充当
  完整玩法通关证据。新实例重启恢复进度和音效开关通过，四种窗口比例截图已查看。
- 八个 ARM64 Release APK 和两个新游戏 Debug APK 的 v2/v3 签名、16 KiB ZIP/ELF
  对齐、原生入口、静态运行库、图标/完整资源 hash、Java TTS 桥接及 manifest 校验
  通过。最终新 Release 包：Marble **1,353,661 B**、Garden **1,292,220 B**；Debug
  分别 **4,159,421 B**、**4,102,076 B**。保留签名密钥；无改动八包增量重建 hash
  一致，最终 `build/android/verification-release.json` 含八包，RuntimeTested=false。
- 桌面 SDK 独立消费 Release（含隐藏窗口图形、搬迁/配置不匹配拒绝）通过；
  本包未改 SDK API，Debug 独立消费成绩属于上一包，未重跑。八个 Windows Release
  EXE 图标/八尺寸通过，新 EXE 的 GUI 子系统和 Release 配置标记通过。
- clang-format 19.1.5 的 40 文件、actionlint 1.7.7、Python/PowerShell 语法、
  修改文本 UTF-8/LF/末尾换行与 `git diff --check` 通过。

日志 `build/campaign-{debug,release}-{build,tests,forge_tests,campaign_tests,smoke}.log`；
新游戏/实验室图形 `build/campaign-smoke-{debug,release}-<name>/`，Forge 图形
`build/campaign-forge-smoke-{debug,release}/`，截图概览 `build/campaign-overview.png`。
最终新游戏构建 `build/campaign-final-<mode>-<name>.log`，全构建
`build/campaign-final-release-build.log`；Android 最终日志
`build/campaign-android-final-release.log`、`build/campaign-android-debug-<name>.log`；
SDK `build/campaign-sdk-release.log`。最终根配置 Windows/MSVC x64 Release，
examples/tests 开启，example_console=n。

### 未运行、限制与下一步

未安装/运行 Android 真机或模拟器，未运行 Linux/macOS、新远端 CI、人工完整
通关/试听/难度调优或长稳。Android Debug 本包仅重建两个新游戏；设备触感、
后台/锁屏、系统安全区与 GPU 恢复仍待验收。两款 UI 目前为英文，中文 README
介绍操作；交付的是各三关的小型玩法复刻，不包含商业原作的全部内容。
M0 的硬件、多 DPI/多显示器、干净机器与发行许可/稳定签名验收仍未完成。

下一步：`xmake run marble_temple` / `xmake run garden_guard` 人工体验完整三关，
重点检查彩球插入方向/连锁/速度，以及阳光经济/种子冷却/敌人波次难度。
Android APK 在 `bin/android/arm64-v8a/release/`，由用户手动安装验收触屏、
后台返回、音效和存档；推送后再检查本提交的远端 CI，不把历史成绩当成新验证。

## 上一工作包：Windows/Android TTS 与 Engine Lab（已实现并本地验证）

基线 `0fc15f8`，开始时工作区干净。用户要求添加 Windows/Android 系统 TTS，
并增加一个展示引擎各类对象的 example；继续沿用 Android 不安装/启动设备的约定。

- 新增公开 `TextToSpeech` / `SpeechOptions` / `SpeechState`，支持 UTF-8、指定语言、
  语速、独立音量、替换朗读、停止及错误查询。Windows 用 SAPI 异步合成；Android
  用引擎内统一 Java/JNI 桥接，游戏无需单独写 Java。初始化期间只保留最新请求，
  JNI 用 UTF-16 传文本、通过 Activity ClassLoader 找类、轮询状态而非回调原生对象。
- GameEngine 每帧更新语音，失焦/暂停/旧场景卸载后停止，Shutdown 释放；独立使用
  API 时需同线程 Update/Shutdown。Linux/macOS 安全返回 Unavailable，未实现后端。
- `engine_lab` 六页演示真实对象：层级/生命周期、刚体/碰撞/触发器/相机、共享纹理与
  动画、粒子/定时器/事件/补间/对象池、瓦片绘制/实体查询、TTS/音效/存档/输入。
  带原创新图标、纹理、短音效，Windows GUI/主题、等比布局、鼠标与触摸按钮。
- 根构建/Android/CI 增至六示例。Android 用 javac/D8 打包桥接及 manifest 查询声明，
  不依赖 Gradle；规则显式 add_orders 保证 NativeActivity 完成后再添 dex/对齐/签名。
  批量构建曾暴露缺少顺序约束，已修正；生成类目录会清除旧 class。Lab 资源路径使用
  APK 相对名字，避免 ./ 前缀。SDK 导出新头文件并传递 Windows SAPI/COM 链接库。
- 新增 `docs/TEXT_TO_SPEECH.md`、示例 API 对照/运行说明；格式范围扩至 30 个 C++ 文件。

### 通过的检查

- Windows/MSVC x64 Debug/Release 六示例构建，分别 **359 项引擎逻辑**、
  **127 项 Forge 逻辑**通过；原四示例 graphics smoke、Forge smoke、Lab smoke
  两模式均 **0 failures**。Lab 覆盖按钮/触摸、动画推进、资源 replacement、跨场景
  事件/补间取消、碰撞/触发、瓦片映射、保存恢复与四种窗口比例。
- Windows 原生 `tts_smoke` 两模式均 **0 failures**：静音真实合成、UTF-8/补充字符、
  XML 按普通文本、缺语言错误及恢复、长度上限、替换、失焦/暂停/切场景停止、重新初始化。
  该成绩不等于人工试听或中文声音质量验收。
- 桌面 SDK 独立消费 Debug/Release（含隐藏窗口）通过：单头文件 include、新接口
  输入拒绝、SAPI 链接、SDK 搬迁与配置不匹配检查；Forge 独立 SDK Release 编译、
  127 项逻辑和八尺寸图标通过，独立入口 graphics 使用 `-SkipGraphics`。
- 六个 ARM64 Release APK 构建、v2/v3 签名、16 KiB ZIP/ELF 对齐、入口、图标、完整
  资源 hash、classes.dex 中桥接类与 manifest 声明通过。Engine Lab Release
  **1,271,622 B**、Debug **4,429,638 B**；Lab Debug 同样通过打包校验。
  保留原签名密钥。无改动重建六 APK hash 一致；删除 Lab APK 后重建恢复，并确认
  注入的无效旧 class 被清理。最终报告 `build/android/verification-release.json` 含六包。
- 六 Windows Release EXE 图标通过；已查看六页截图及布局。clang-format 19.1.5、
  actionlint 1.7.7、Python/PowerShell 语法、修改文本 UTF-8/LF、git diff --check 通过。

日志 `build/tts-*`；Lab 截图/结果在 `build/tts-lab-final-{debug,release}/`，
原生语音日志 `build/tts-{debug,release}-tts_smoke.log`，Forge 图形在
`build/tts-forge-{debug,release}/`，概览 `build/tts-lab-overview.png`。
最终根配置 Windows/MSVC x64 Release、examples/tests 开启、example_console=n。

### 未运行、限制与下一步

未安装/运行 Android 设备或模拟器，未运行新远端 CI、Linux/macOS、人工音频试听、
中文声音质量/真实无引擎与缺语言场景、后台/销毁/锁屏及长稳验收。其余五包未构建
Android Debug。Android 打包扩展依赖 xmake 3.1.1 的 NativeActivity 中间 APK，升级
xmake 需重验；Android 安装 SDK 接口尚未提供，使用根工程规则。

用户可启动 `bin/engine_lab.exe`，或手动安装 Engine Lab APK，在 Services 页试听
English/Chinese、Stop 和速率/音量；中文须安装可被 SAPI 或 Android TTS 使用的声音。
接着手动验收 Android 页面资源、触摸、切页停止、后台返回和无语音包错误提示。
本包不替代 M0 硬件/多 DPI/长稳与商业发行验收。

## 上一工作包：Windows 第二关 CRLF 加载错误（已修复并验证）

基线 `28c3e78`，开始时工作区干净。用户反馈 Windows 第二关启动报错，尚未
提供具体报错原文。本机标准 LF 资源正常；在隔离游戏副本中只将第二关转换为
Windows CRLF，旧程序进入第二关确实退出，日志为
`Unknown level directive at line 35`。该行是上一包删除激光后留下的空行。
ReadAssetText 保留原始字节，解析器把残留的 CR 当成非空指令；此外名字/标牌
末尾的 CR 会妨碍中文翻译匹配。这是本地确认的复现条件，并非已读取用户的报错。

- 关卡解析兼容 LF/CRLF、UTF-8 BOM、纯空白行和缩进注释，去掉文本末尾空白。
  保留非法指令/字段拒绝，解析错误补充文件路径和原始行号。没有修改通用文件
  读取语义、全局 Git 换行配置或用户存档，也没有靠删除第二关空行规避问题。
- 增加真实三关的 CRLF/BOM、翻译键一致性、混合换行/空白/注释、错误定位检查；
  修复前 **119 passed / 8 failed**，修复后全部通过。
- Forge 图形 smoke 改为实际经第一关结算按钮进入第二关，再保存返回并通过
  “继续旅程”重新进入第二关，覆盖用户可能遇到的两个入口。README 说明文本格式。

### 通过的检查

- Windows/MSVC x64 Debug/Release 全目标构建、各 **127 项 Forge 逻辑**和
  **335 项引擎逻辑**通过，原四示例图形 smoke 两模式均零失败。
- 保留原 CRLF 第二关资源的隔离游戏，换入修复后 Debug/Release EXE 均图形
  smoke **0 failures**；根工程正常 LF Release 同样 **0 failures**。两种进入
  第二关的实际菜单路径均通过，验证目录与真实用户存档隔离。
- 独立 SDK Release 编译、127 项逻辑、八尺寸图标通过；其图形入口使用
  `-SkipGraphics`。两个 Windows 输出目录均已更新，最终根配置为 MSVC x64
  Release、examples/tests 开启、example_console=n。
- Forge ARM64 Release APK **2,981,254 B** 构建/签名/16 KiB 对齐/原生入口/
  静态运行库/图标/资源校验通过；保留原签名密钥，未安装或运行设备。
- clang-format 19.1.5、修改文本 UTF-8/LF、`git diff --check` 通过。

日志/隔离副本：`build/forge-conduit-fix/`。`baseline-data/logs/game.log` 保留
旧 EXE 的失败证据，`regression-before.log` 为修复前八项失败；
`fixed-crlf-{debug,release}` 与 `normal-release` 保存修复后的图形结果。

### 未运行与下一步

未运行 Linux/macOS、新远端 CI、Android 真机/模拟器、Android Debug 或独立
SDK Debug；未进行人工完整通关与长稳。用户可启动更新后的
`bin/forge_runner/forge_runner.exe` 再试第二关；若原始报错不同，继续依据用户
提供的原文和实际启动目录定位，勿把本次 CRLF 复现当成已确认的用户日志。

## 上一工作包：Forge 三关玩法差异（已实现并本地验证）

基线 `a9a0f79`，开始时工作区干净。用户要求三关除了背景之外，在敌人、场景
元素和摩擦手感上有更多变化。本包只改 Forge 游戏及其文档，不改引擎接口。

- 铸造厂加入三段顺向/逆向传送带、三辆装甲地面巡逻车。箭头显示输送方向，
  传送带受墙壁和危险区约束，跳离即停止输送；巡逻车不能冲刺穿过，需跳跃。
- 冷却管廊加入 0.12–0.16 抓地倍率冰面，起步、反向和松手刹停更慢，保持
  空中操控；三处冷却风机气流代替升降台，托举角色抵达高处单向平台。
- 反应核心加入五块踩踏后 0.65–0.8 秒坍塌、2.5 秒后恢复的平台，以及五座
  周期炮塔。平台显示警告/恢复进度，重建避开角色占据区域；炮塔预警 0.75 秒，
  水平有限射程弹丸受实体平台遮挡，可跳跃或冲刺躲避。敌人与弹丸增加扫掠接触。
- 调整第三关两处机关与落点的组合，三关正常指令路线搜索全部通过；所有检查点
  跨五秒敌人周期仍安全。电池数量/索引及检查点顺序保持，旧进度可继续，重生
  恢复坍塌平台。没有把自动路线搜索描述为人工手感验收。
- 外部关卡升为 version 2，新增 ice/belt/fragile/airflow/crawler/sentry 指令
  及字段校验，保留 version 1 读取。README 说明玩法、应对与参数。八条中英文
  机关提示、388 字符字体子集更新；不新增操作键，桌面和触屏共用玩法逻辑。

### 通过的检查

- Windows/MSVC x64 Debug/Release 全目标构建通过，各 **335 项引擎逻辑**、
  **119 项 Forge 逻辑**通过，零失败。包含三关不传送的自动完整通关、冰面惯性、
  输送碰撞、气流、坍塌/重建/重生、装甲巡逻车、炮塔周期/遮挡/冲刺、解析兼容。
- 两模式原图形 smoke 及 Forge 中英文/多指模拟 smoke 均 **0 failures**；
  Forge 增加坍塌恢复和炮弹画面，每模式导出 24 张截图。已人工查看三关与机关
  警告/恢复的截图，预览 `build/forge-variety-preview.png`。
- Forge 独立 SDK Release 编译、119 项逻辑和八尺寸 EXE 图标检查通过；该入口
  使用 `-SkipGraphics`，上述图形成绩来自根工程两模式。
- Forge ARM64 Release APK **2,977,158 B**，构建、v2/v3 签名、16 KiB ZIP/ELF
  对齐、原生入口、运行库、图标及完整资源校验通过；未更换签名密钥。
- clang-format 19.1.5 的 21 文件检查通过。两份中文字体隔离重建逐字节一致、
  全部 388 字符覆盖通过。修改文本 UTF-8/LF 与 `git diff --check` 通过。

日志 `build/forge-variety-*`；图形目录 `build/forge-variety-smoke-{debug,release}/`。
APK `bin/android/arm64-v8a/release/forge_runner.apk`，校验报告
`build/android/verification-release.json`（本次仅 Forge）。最终根工程为
Windows/MSVC x64 Release、examples/tests 开启、example_console=n。

### 未运行与边界

- 未运行 Android 真机/模拟器、Linux/macOS、新远端 CI、Android Debug 或独立
  SDK Debug；本包未重建其余四个 APK。Android 构建不代表设备运行与触感验收。
- 未做人工从头到尾通关、真实手柄、音频试听、DPI/多显示器或长稳；M0 原有
  待验收项保持。气流和抓地倍率属于 Forge 控制器，不是新的引擎刚体材质系统。

下一步：用户试玩三种地形，重点反馈冰面刹停距离、气流出入口、末关坍塌时间
与炮塔难度；Android 使用同签名新 APK 手动验收。推送后再检查本提交 CI。

## 上一工作包：跨平台工具、代码格式与示例体验（已实现并本地验证）

基线 `de9112a`，开始时工作区干净。用户要求审视跨平台差异和紧凑源码，并改进
Forge 安卓宽屏/三关美术/主角、Bouncing Balls 多形状、Bullet Hell 玩法可理解性。

- 新增公开 `TouchTracker`（ID 稳定、坐标映射、按下/释放、失焦/改画布取消、
  异常触点阻塞）和 `CanvasLayout`（内容矩形内的边缘/中心锚点），由 SDK 导出。
  Forge 的多动作缓冲复用 tracker；Bullet Hell 分别捕获移动和 FOCUS 手指。
- Forge 改 Expand 等比画布，扩展世界可见范围、居中菜单与对应命中坐标、
  边缘 HUD/触控按钮；相机处理小于可视区域的关卡。三关改成暖铸造厂、蓝色
  冷却管廊、紫金反应核心，新角色有护甲/头盔/背包/围巾及更清晰的关节动画。
- Bouncing Balls 现在显示 Shape Lab：圆/方/长方形，混合/实心/描边独立选择，
  碰撞与边界按形状尺寸处理。描边仅为外观，仍是完整圆/矩形碰撞体。
- Bullet Hell 增加目标/操作图示、练习/正式入口、三秒准备、自动射击与多指
  慢速按钮，练习不扣护盾；正式三次机会；暂停/失焦取消手势。
- `.clang-format` / `.editorconfig`、`scripts/format-code.py` 及 CI 格式检查；
  clang-format 19.1.5，默认覆盖 20 文件。Forge 源码/测试、两示例及新工具展开
  为四空格/100 列的 C++ 格式；九个纯格式文件与格式化前基线逐字节对比通过。
  `docs/PLATFORM_GUIDE.md` 说明文件/输入/布局/生命周期/图形差异与工具边界。

### 通过的检查

- 最终源码 Windows/MSVC x64 Debug/Release 全目标，分别引擎 **335 passed,
  0 failed**、Forge **72 passed, 0 failed**；两模式原图形 smoke 与 Forge 221 帧
  中英文/多指模拟 smoke 均 **0 failures**。检查角色、美术、宽屏按钮及菜单截图。
- 五个 ARM64 Release APK 签名、16 KiB ZIP/ELF 对齐、入口、静态运行库、
  图标/资源 SHA-256 校验通过；Forge **2,956,678 B**，Shape Lab **1,267,283 B**，
  Bullet Hell **1,283,664 B**。密钥未修改，报告 `build/android/verification-release.json`。
- Windows 独立 SDK 消费通过，含两个新头文件的单独 include；模式/架构/运行库
  不匹配拒绝检查通过。Forge 独立 SDK Release 编译、72 项逻辑及八尺寸图标通过，
  该入口使用 `-SkipGraphics`，图形测试成绩来自根工程两模式。
- 最终五个 Windows Release EXE 图标通过；clang-format 19.1.5 的 20 文件范围
  检查及九个纯格式文件与格式化后的基线比对通过；actionlint 1.7.7 通过。
  旧 actionlint 缓存缺失，已按发布校验和重新下载工具到 build/，后续实际检查通过。
- 34 个改动文件的 UTF-8/LF/文件末尾换行、Python 语法及 `git diff --check` 通过。

日志统一为 `build/platform-examples-*`，Forge 截图在
`build/platform-forge-smoke-{debug,release}/screenshots/`；原示例截图在
`bin/screenshots/`，这些均为 Windows 渲染/触摸输入模拟截图。
本地最终恢复 Windows/MSVC x64 Release、examples/tests 开启、example_console=n。

### 未运行与边界

- 未运行 Android 设备/模拟器、Linux/macOS 本机、新远端 CI、这次源码的 Android
  Debug 或独立 SDK Debug。APK 构建不证明设备运行/锁屏恢复/性能耗电验收完成。
- 系统级 WindowInsets 自动查询、真实空心容器碰撞、其他小示例通用 UI 多指迁移
  未实现。公共 TouchTracker 的事件只保留一帧，固定更新的动作缓冲仍由使用者负责。

下一步：用户手动安装新 APK，确认宽屏原生 Surface、两侧操作区、三关视觉与主角、
弹幕练习/双指 FOCUS；如系统仍给 Surface 留边，记录机型/系统和窗口行为后定位。
推送后检查本提交的 CI 格式任务及五 APK/桌面产物。M0 长稳/硬件验收继续待办。

## 上一工作包：Android 本地命令与自有 key 使用说明（文档）

用户询问默认本地 Android 构建命令、其他平台缺失 keystore 和自有 key 的用法。
基线 `eac4f5d`，开始时工作区干净；核对当前构建脚本和 xmake 3.1.1 打包规则。

- `docs/ANDROID.md` 明确 Windows 默认构建五个 ARM64 Release APK，包装脚本
  首次创建被 Git 忽略的本地 debug.keystore，直接 xmake 不创建密钥。
- 补充 Linux/macOS 的 JDK/SDK/NDK 环境、存在性保护的 keytool 生成、完整
  xmake 配置及单独构建 Forge 命令；已有标准 Android debug key 可传路径。
- 说明当前 `--android_keystore` 只配置路径，密码固定 android，未开放
  alias/独立 key password，包装脚本也没有 -Keystore 参数。为已有自有 key
  提供两种 shell 的 apksigner 交互签名与校验命令，输出独立 signed.apk。
  没有修改构建/CI 签名行为，没有修改或删除现有 key。

通过的检查：在 Windows 使用 build/ 下新生成的临时 JKS，指定自定义 alias、
不同的 store/key 密码，按文档经 stdin 回答两个交互提示后重签现有 Forge
Release APK。v2/v3 签名、16 KiB ZIP 对齐及签名证书 SHA-256 与该 key 一致性
通过；原 Forge APK 和原 debug.keystore 的 SHA-256 保持不变。
文档两段 POSIX shell、四段 PowerShell 命令语法检查，UTF-8/LF 和
`git diff --check` 通过。
日志：`build/android/signing-doc-check-deafe814d7e04fa8ba31d859ffcc1c15/verification.log`。

未运行：本包没有原生代码改动，不重跑编译/逻辑/图形测试；未在 Linux/macOS
主机运行构建，未运行远端 CI，未安装/启动 Android 设备。构建中直接使用自定义
密码/alias 仍非现有功能，当前可用路径是构建完成后用自有 key 签名。

下一步：用户按所在平台执行文档命令，自有 key 场景安装 signed.apk，并继续
手动验收 Forge 的启动、多指触控和存档。跨机器更新需保留原私钥。

## 上一工作包：FORGE Android APK 与多指触控（已实现并验证构建）

用户继续要求支持 Android。本包基线 `09c8443`，沿用不安装/启动设备的约定。
提交主题：`Add FORGE Android APK packaging and multitouch controls`。

1. 根工程增加 Forge NativeActivity/ARM64/API 26+/target 35/OpenGL ES 2.0
   目标，包名 `org.raygame.forge`，横屏及反向横屏。源码、三关、音频、字体和
   本地化复用桌面版本；独立 SDK 工程仍面向桌面。新设置默认 60 FPS，移动端
   固定系统全屏，保持 1280×720 Fit 等比画布。
2. 新增 `FileSystem::ReadAssetText`：桌面 UTF-8 文件路径，Android 通过
   AssetManager 分块读取压缩/未压缩资源，检查大小上限，失败保留输出。
   Forge 关卡和翻译改用该 API；Android 资源根为 `forge`，raylib 直接读取
   同目录字体/音频。普通 ReadText/WriteAtomic 仍只用于私有文件/存档。
3. TouchControls 按触点 ID 捕获移动、下落、跳跃、冲刺、暂停或菜单角色。
   支持多指组合、移动区左右滑动；跳跃/冲刺短按缓冲至固定步，之后仅消费
   一次，下落修饰键也随短按保留。页面切换、失焦、映射变化清除操作并要求
   已按住的手指先抬起；多指菜单不会提升其他手指或产生重复点击。
4. 游戏增加触屏按钮与菜单、制作信息入口、左右半行调整设置、系统 Back
   暂停/返回/保存退出。移动提示与关卡标牌改为触屏说明；新增中文翻译，
   字体子集扩至 353 字符。桌面键盘/鼠标/手柄入口保留，触屏模式忽略 Android
   合成鼠标，避免双重触发。普通 Android 流程不调用桌面窗口图标/标题 API。
5. Forge 自有 Android manifest、五密度/自适应/主题图标及生成器。APK 使用
   `assets/forge/` 的 28 个资源和单独的第三方说明；资源同步清理过期文件。
   build-android 默认构建/检查五游戏，支持 `-Game forge_runner`；CI 同一路径
   自动包含 Forge APK。修复 Android Debug 首次暴露的静态引擎非 PIC 链接问题，
   Android 引擎显式 `-fPIC`，可正确链接全局变量/TLS 到 NativeActivity 共享库。

### 本包通过的检查

- 五个 ARM64 Release APK 构建、v2/v3 签名、ZIP/ELF 16 KiB 对齐、原生入口、
  静态运行库、图标和全部资源 SHA-256 检查通过。Forge Release **2,952,582 B**，
  Debug **6,086,022 B**；Forge Debug 同样通过上述检查。aapt 核对包名、API、
  NativeActivity、ARM64 和 `forge_runner` 库元数据。
- 同配置增量重跑保留五 APK 的 hash；人工放入 Forge staging 的过期资源
  被清除，五游戏资源再次全部通过。没有更换或删除本地 Android 签名密钥。
- Windows/MSVC x64 Debug/Release 全目标通过：各 **316 passed, 0 failed**
  引擎逻辑、**69 passed, 0 failed** Forge 逻辑；原图形 smoke 零失败。
  Forge 扩展到 221 帧的桌面/中英文/触屏 smoke 均 **0 failures**，含多指驱动
  Pilot、暂停、设置双向调整、Back、焦点、存档、制作信息和覆盖确认。
- Windows 独立 SDK Release：69 项逻辑、八尺寸图标和扩展图形 smoke 通过。
  根工程最终 Release 五 EXE 图标通过；恢复 Windows/MSVC x64 Release、
  build_examples=y、build_tests=y、example_console=n。
- 已查看触屏游戏、主菜单和设置截图。10 个 Android 图标资源及两份中文字体
  在隔离目录重建，逐字节一致；字体覆盖 353 字符。PowerShell/Python 语法、
  actionlint、UTF-8/LF 与 git diff --check 通过。

### 未运行与边界

- **未安装或启动 Android 真机/模拟器**。APK 构建不代表 Android 启动、
  AssetManager/音频实际运行、多指触感、后台/锁屏恢复、刘海/手势安全区、
  GPU 上下文恢复或性能耗电已验收。设备运行继续由用户手动验证。
- 未推送或运行本包远端 CI，未运行 Linux/macOS、本包独立 SDK Debug 或
  其他四游戏的 Android Debug。Linux/桌面分支保留，不能据本地结果宣称新 CI
  或全平台通过。M0 硬件/长稳与商业发行验收仍未完成。
- Android APK 使用本地调试签名；CI 每次签名不同，覆盖安装需相同签名。
  多指操作属于 Forge 游戏层，尚未把通用 UI 触摸路由改造扩展至其他四示例。

入口：`pwsh -NoProfile -File scripts/build-android.ps1 -Game forge_runner`；
APK：`bin/android/arm64-v8a/release/forge_runner.apk`（Debug 使用同级 debug）。
默认不带 `-Game` 构建五 APK。校验报告 `build/android/verification-{release,debug}.json`；
日志 `build/forge-android-{all-release,apk-debug,incremental,apk-metadata,apk-manifest}.log`，
`build/forge-android-desktop-{debug,release}-*.log`、`build/forge-android-final-*.log`、
`build/forge-android-sdk-release.log`。桌面触屏截图在
`build/forge-android-smoke-{debug,release}/screenshots/`，不是 Android 设备截图。

下一步：用户手动安装 Forge Release APK，优先检查启动/中英文、三指组合、
返回键与重启后的检查点恢复；按 `docs/ANDROID.md` 记录机型/系统/日志。
推送后核对本提交 CI 的五 APK 与桌面产物，再推进真实设备和长稳矩阵。

## 上一工作包：FORGE 接入默认桌面构建与 CI 产物（已实现并验证）

用户指出根目录 build 未包含 FORGE，并要求 CI 也能生成。本包基线 `445f1e3`。
开始时 `examples/forge_runner/src/main.cpp` 有用户添加的一个空行，保留原样，
不纳入本包提交。提交主题：`Include FORGE in default desktop builds and CI artifacts`。

1. 根工程增加默认 `forge_runner` 和可单独构建的 `forge_tests`，分别服从
   `build_examples` / `build_tests`。默认桌面 build 现在包含五个游戏；引擎
   单独构建和 tests-only 仍可用。Android 仍仅定义原四个游戏。
2. 游戏的 `targets.lua` 供根工程和独立 SDK 工程共同使用。根工程链接当前
   raygame，在 build 中同步 `<raygame/...>` 头文件；phony 依赖设 build.fence，
   确保并行编译开始前准备好头文件。独立 SDK 入口继续只使用安装目录。
3. 根工程输出为 `bin/forge_runner/forge_runner.exe`，保留相邻 assets、README
   和第三方说明。资源按内容同步并清除过期文件，不改动四小示例共用 assets。
   FORGE 始终为 Windows GUI 程序；继承根工程配置标记，切换模式正确重新链接。
4. CI 的 Windows/Linux Debug/Release 默认构建包含 FORGE，并分别构建/运行
   forge_tests。Windows 保留独立 SDK 构建和图标检查。Windows 总游戏包增加
   完整 forge_runner 子目录；独立 Forge 包在压缩包根目录包含 EXE/assets/说明，
   同时新增 Linux Forge 下载包。各测试独立步骤，防止 PowerShell 后续命令
   覆盖前一个原生命令的失败状态。相关 README/SDK 说明已更新。

### 本包通过的检查

- Windows/MSVC x64 根工程 Debug/Release 默认及全目标构建通过；各 **311 passed,
  0 failed** 引擎逻辑和 **49 passed, 0 failed** Forge 逻辑，原图形 smoke 与
  Forge 中英文图形 smoke 各 **0 failures**。两模式五 EXE 图标通过，最终五个
  Release EXE 均为 GUI 子系统。游戏编译/链接日志没有新增警告。
- tests-only、engine-only、examples-only 的目标列表和构建通过；tests-only
  Forge 49 项通过。从空 Forge 输出/头文件 staging 运行默认 `-j 4` 构建成功。
  最后恢复 Windows/MSVC x64 Release、build_examples=y、build_tests=y、example_console=n。
- 独立 SDK Debug/Release 两种入口各 49 项逻辑及图标检查通过；Release 图形
  smoke **0 failures**。独立 Debug 使用 CI 的 `-Verify -SkipGraphics` 路径，
  明确未运行该入口的图形 smoke；根工程 Debug 图形已另行通过。
- 28 个 Forge 输出资源逐一 SHA-256 匹配；损坏的输出翻译被恢复，过期探针
  被清除，增量无编译/链接且 EXE hash 不变，四小示例资源 hash 不变。
  按 CI 布局制作的 31 文件 ZIP 全部 hash 匹配；解压到含空格的新目录后，
  实际运行 Forge 图形 smoke **0 failures**。
- Android 原配置目标列表仍为引擎和原四游戏；没有新增 Forge APK 目标。
  actionlint、UTF-8/LF 与 git diff --check 通过。

### 未运行与边界

- **未推送或运行本包远端 CI**，本机没有可用 Linux/WSL，未做 Linux/macOS
  本地编译/运行。已查询到基线 `445f1e3` 的远端运行 `38047782926` 成功，
  含 Windows Forge 两模式产物；这是基线成绩，不是本包 CI 验证。
- 本包未重新构建 Android APK，也未运行设备；FORGE 暂无 Android 入口、
  打包/资源适配与触屏控制。用户追问 Android 支持时已说明目前仅桌面可用。
- 未扩展玩法或 M0 硬件/长稳验收。Linux artifact 解压后需 `chmod +x forge_runner`，
  Windows 目标机器仍需相应 VC++ 运行库；未做干净机器发行验收。

当前直接入口：根目录 `xmake build` / `xmake run forge_runner`；逻辑测试
`xmake build forge_tests` / `xmake run forge_tests`。独立入口仍为
`pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify`。
根工程可试玩包：`build/forge/FORGE-root-windows-x64-release.zip`。
日志 `build/forge-root-{debug,release}-*.log`、`build/forge-root-sdk-*.log`、
`build/forge-root-{tests-only,engine-only,examples-only,final}-*.log`；
资源/打包报告 `build/forge-root-package-verification.json`，图形结果在
`build/forge-root-{debug,release,package}-smoke/`。生成物备份在 build 内保留。

下一步：推送后核对本提交 Windows/Linux CI 和新的下载包；FORGE Android
若继续实施，需作为单独移植工作包，不能把桌面构建接入描述为 APK 已支持。

## 上一工作包：按示例归属整理游戏与资源（已实现并验证）

用户同意把参考游戏放入 examples，专用资源归各示例自己的目录。本包基线
`fb7c474`，开始时工作区干净。提交主题：
`Group example games and their resources by ownership`。

1. `games/forge_runner` 整体移至 `examples/forge_runner`，仍是只消费已安装
   SDK 的独立 xmake 工程，未加入根工程默认四示例。字体/音频/关卡/翻译/
   许可随本目录保存；构建脚本、CI 上传路径、SDK 和游戏说明同步更新。
2. 四个小示例各自拥有 `assets/icon.png`、`artwork/icon.svg`、
   `resources/windows/<name>.ico/.rc` 和 `android/`。共用字体/许可移至
   `examples/common/assets/fonts`，图标说明移至 `examples/common/ICONS.md`。
   新增 `examples/README.md`，列出五个示例、资源归属与构建入口。
3. 桌面保持 `bin/<name>.exe`；共用字体复制到 `bin/assets/fonts`，专用
   资源复制到 `bin/assets/<name>/`，窗口 PNG 的读取路径同步更新。
   tests-only 配置下 smoke 仍依赖完整资源；资源复制不改写内容相同的文件。
4. Android 按游戏在 `build/android/example-assets/<name>` 合并共用与专用
   资源，打包只使用该 staging 目录；增量构建清除 staging 中已移除的文件。
   构建脚本逐文件核对 APK 资源 SHA-256，并拒绝缺失、多余和冲突资源，
   报告新增资源清单及 `AssetContentsVerified`；原签名/对齐/入口检查保留。
5. Windows 图标校验与 Python 生成器按游戏寻找资源。图标说明补充 ICO →
   RC 编译 → EXE PE 资源的过程，以及运行时窗口 PNG 的加载方式。

### 本包通过的检查

- Windows/MSVC x64 Debug/Release 全目标构建通过，各 **311 passed,
  0 failed**，图形 smoke 各 **0 failures**；两模式四 EXE 八尺寸内嵌图标
  与源 ICO 一致，四个 Release EXE 为 GUI 子系统。构建日志无新增警告。
- 将原 `bin/assets` 移到 build 备份后，从空目录复制全部 9 个资源，逐文件
  hash 与新源路径一致，smoke 通过。Release tests-only 配置再从空目录
  构建/运行 tests 与 smoke，分别 311 passed / 0 failures；最后恢复
  Windows/MSVC x64 Release、build_examples=y、build_tests=y、example_console=n。
- FORGE 从新路径独立构建 SDK 和游戏，Debug/Release 各 **49 passed,
  0 failed**、172 帧中英文图形 smoke **0 failures**，两模式 EXE 八尺寸
  图标通过；游戏编译/链接无新增警告，Release 为 GUI 子系统。
  Release 试玩 ZIP 按新路径更新，31 个文件与输出核对一致；解压到含空格的
  新目录后，实际运行图形 smoke **0 failures**。
- 四个 Android ARM64 Release APK 从新目录编译受影响对象、重新链接/打包，
  v2/v3 签名、ZIP/ELF 16 KiB 对齐、NativeActivity 入口和静态运行库检查
  通过。每 APK 的 6 个运行资源与各自源文件逐字节一致，没有其他游戏资源。
  人工放入 staging 的过期资源探针被清除；同配置无改动再构建复用原生和
  APK 产物，资源检查再次通过。没有安装/启动 Android 设备。
- 113 个迁移文件中 **109 个逐字节不变**，另外 4 个仅为路径/归属说明
  文档；所有迁移 C++、字体、音频、图片和打包资源内容不变。
  新布局隔离重跑图标生成器，52 个生成资源逐字节一致。
- PowerShell/Python 语法、actionlint、UTF-8/LF 与 git diff --check 通过。

### 未运行与边界

- 未运行本包 Linux/macOS、远端 CI、真实手柄、多 DPI/多显示器或长稳验收；
  保留跨平台分支。Android 真机/模拟器继续按用户要求暂缓。
- 本包是目录归属与打包调整，没有改变玩法/中文渲染，也没有将 M0 或商业
  发行验收标为完成。FORGE 仍是桌面参考游戏，未移植到 Android。
- 四小示例仍共享 bin 中的 EXE/字体目录；FORGE 输出与资源独立，整体复制
  时应保留自己的 assets。既有配置标记仍保证桌面模式切换时正确链接。
- 历史工作包中的旧路径保留为历史记录；**当前入口以下方新路径为准**。

运行：`pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify`。
FORGE EXE：`examples/forge_runner/build/windows/x64/release/forge_runner.exe`。
原四示例：`bin/<name>.exe`；Android：`bin/android/arm64-v8a/release/<name>.apk`。
日志：`build/migration-{debug,release}-{config,build,tests,smoke,icons}.log`、
`build/migration-tests-only-*.log`、`build/migration-forge-{debug,release}.log`、
`build/forge/<mode>/*.log`、`build/migration-android-{build,nochange}.log`、
`build/android/verification-release.json`、`build/migration-icons.log`。
迁移 hash 清单 `build/migration/moved-files.json`；原桌面资源备份也在此目录。
试玩包 `build/forge/FORGE-windows-x64-release.zip`，解压运行验证目录
`build/migration/packaged FORGE`；不要删除本地 Android 签名密钥。

下一工作包建议优先完整试玩 FORGE，记录手感/难度/音效/真实手柄问题；
之后增加按键重绑和无障碍选项，测量中文字体图集的内存及首次创建耗时，
再推进 M0 的故障/DPI/多显示器/长稳矩阵与干净机器发行验收。远端 CI 尚需
推送后核对，勿把本包本地通过或历史 CI 记作新远端通过。

## 上一工作包：FORGE 中英文切换与清晰中文字体（已实现并验证）

用户要求支持中英文切换，中文字体好看清晰。本包基线 `22fcbc6`，开始时工作区
干净。提交主题：`Add Chinese and English localization with clear FORGE SC fonts`。

1. 新增 `TextCatalog` 与 `assets/locales/zh-CN.tsv`，86 行翻译覆盖菜单、HUD、
   逐关/最终结算、保存提示和三关名称/序言/标牌。中英文共用布局及测量/绘制
   路径；动态区域文本先翻译再拼接。品牌/第三方字体名保留原文。
2. 设置增加“语言 / LANGUAGE”，键盘/鼠标/手柄可即时切换简体中文/English，
   立即保存 `language=zh-CN/en`，标题栏同步切换；途中切换不重置世界。
   新设置及旧存档无语言字段默认中文，其他设置/关卡进度保留；未知语言安全
   回退中文。设置输入计数、按键 smoke 与更长中文按钮布局相应调整。
3. 引入 Noto Sans SC 的 Medium(500)/SemiBold(600) 子集并改名 FORGE SC，
   342 个 Unicode 标量覆盖本游戏中文及 ASCII，两文件合计约 160 KB。
   原始 SIL OFL 和版权随包，来源 SHA-256/重建步骤在 assets/fonts/README.md。
   `prepare-chinese-fonts.py` 固定字重/裁剪/改名/保留版权许可，普通构建无需
   Python 或网络。正文与标题独立字重，按实际字号 2 倍光栅化、双线性采样，
   补偿中文字面比例并设置 16px 最小字面，英文保留原字体。
4. 公开 ResourceManager 字体 API 新增 `vector<int>` 字符集重载；保持原调用
   兼容和类型句柄/替换失败保留规则，拒绝非法 Unicode 标量及超过 8,192 字符
   的列表。说明见 SDK.md；原示例继续使用默认西文加载。
5. 图形 smoke 扩展到 172 帧，检查中文全字形覆盖、实际文字像素、键盘切英文/
   手柄切中文、设置重新加载和途中进度不变，并导出两种语言的代表页面。

### 本包通过的检查

- FORGE MSVC x64 Debug/Release 各 **49 passed, 0 failed**，图形 smoke 均
  **0 failures**。包含旧设置/未知语言回退、两语言读写及原 42 项玩法回归。
  两种模式的 EXE 八尺寸图标检查通过，游戏编译/链接没有新增警告。
- 原引擎 MSVC Debug/Release 全目标构建通过，各 **311 passed, 0 failed**，
  图形 smoke 零失败；新增指定字形替换和非法 Unicode/代理区替换保留测试。
  四示例 Release 八尺寸图标检查通过。最终 IDE 为 Windows/MSVC x64 Release，
  build_examples=y、build_tests=y、example_console=n。
- 自动核对三关全部名称/序言/标牌有翻译；两个字体的 cmap 覆盖目录全部字符，
  字重为 500/600、名称为 FORGE SC。隔离重建两字体，逐字节一致。
- 中文主菜单、设置、暂停、确认、制作信息、游戏 HUD 和结算截图已查看；
  英文设置/原布局也已查看。预览 `build/forge-localization-preview.png`。
- Android 四 ARM64 Release APK 构建/签名/对齐/入口/资源检查通过，复用编译
  缓存；没有新增 APK 中文资源或运行设备。
- Python/PowerShell 语法、UTF-8/LF 与 git diff --check 通过。

### 未运行与边界

- 本包仅本地 Windows 验收，未运行 Linux/macOS、远端 CI、真实手柄或多 DPI/
  多显示器/长稳检查；M0 和商业发行验收继续未完成。Android 真机仍暂缓。
- 中文字体是本游戏目录所需字形的子集。自定义中文内容需要补翻译并重建字体；
  未实现任意中文输入、动态字形缓存、IME 或复杂文字塑形。未翻译的自定义
  关卡文字回退原文；现有三关文本已完整覆盖。
- 字形图集按使用字号同步缓存，首次显示一个字号会创建图集；没有后台异步
  字形加载。字体文件的精简大小不代表 GPU 图集同样小。

运行：`games/forge_runner/build/windows/x64/release/forge_runner.exe`。
主菜单/暂停 → 设置 → 语言 / LANGUAGE，左右或点击切换。
日志 `build/forge-localization-{debug,release}.log`、`build/forge/<mode>/*.log`；
截图在各模式 smoke-data/screenshots；原引擎日志
`build/forge-i18n-engine-{debug,release}-{config,build,tests,smoke}.log`，Android
`build/forge-i18n-android-build.log`。字体重现 `build/forge-fonts/reproduce.log`。
Release 试玩 ZIP 更新为 `build/forge/FORGE-windows-x64-release.zip`。

下一步：根据实际游玩反馈继续打磨手感/关卡，推进真实输入/多 DPI 与长稳验收；
按键重绑和无障碍仍待推进，中英文切换已实现，勿重复建设。

## 上一工作包：FORGE 独立横版平台参考游戏（已实现并验证）

用户询问 M0 验收含义，并明确优先实现完整横版平台参考游戏，要求工业级效果。
本包开始时工作区干净，基线 `8efbf85`。提交主题：
`Add FORGE platform game as an independent RayGame SDK consumer`。

1. 新增 `games/forge_runner` 独立 xmake 项目，只消费安装后的 RayGame SDK，
   不引用引擎源码/原公开头文件/examples。三关工业科幻平台游戏 FORGE /
   The last delivery：Foundry、Conduit、Heart，收集全部电池解锁出口。
2. 固定步自定义角色模拟，包括加减速、跳跃宽限/缓冲、可变跳高、二段跳、
   墙跳/滑墙、冲刺冷却、单向平台下落和移动升降台。周期激光、无人机、危险
   区域、检查点/死亡重生；危险分段检测，死亡不能同时触发出口过关。
3. 完整主菜单、继续/新游戏覆盖确认、暂停/重试、设置、逐关/最终结算与制作
   信息。键盘/鼠标/手柄输入、失焦自动暂停；固定 1280×720 Fit 等比画布。
   per-user 文件日志/校验存档/原子写入/备份恢复，保存失败提示。
   过关后存档指向下一关，最终通关保留纪录并关闭 active run。
4. 三层视差工业场景、机械设施/平台、关节角色/围巾、灯光、粒子、落地反馈、
   可关闭震屏、HUD/排版。原创八种合成音效与 32 秒配乐，八尺寸 EXE ICO 和
   窗口 PNG。字体及原始 OFL、常见依赖源许可随 assets 提供。
5. 关卡为独立版本化文本，加载检查字段范围/数量和未知指令；生成脚本源码与
   资源均提交，正常构建不依赖 Python。campaign 索引固定三关，增加第四关
   需扩展游戏索引与存档 schema，无需修改引擎。
6. 引擎新增默认开启的 `SetDefaultShortcutsEnabled`，游戏可接管桌面快捷键
   和 Android 默认返回/暂停。新增 `AudioManager::SetMusicVolume`，调整
   音量不重置播放游标。四个示例保留默认行为。
7. `scripts/build-forge.ps1` 隔离 xmake 配置构建/安装 SDK 和游戏，支持
   Debug/Release、逻辑/图标/图形验收；`-SkipGraphics` 明确报告未运行图形。
   Windows CI 加入构建/逻辑/图标与游戏包上传，CI 使用无图形模式。
   图标检查脚本保持默认四游戏，增加可指定 EXE/ICO 的参数。接入见 SDK.md。

### 本包通过的检查

- MSVC 2022 x64 Debug/Release 原引擎全目标构建通过，各 **311 passed,
  0 failed**，图形 smoke 各 **0 failures**，四示例八尺寸内嵌图标均匹配。
- FORGE 独立 SDK Debug/Release 构建通过，各 **42 passed, 0 failed**，
  图形 smoke 各 **0 failures**。逻辑路线搜索通过正常移动/跳跃/冲刺命令
  完成三关的全部电池与出口，不传送/改几何；针对性检查另有传送定位。
  包含跳高/二段跳/宽限、单向/移动平台、危险穿透、检查点、损坏恢复、最终
  完成和不可写目录。新游戏编译/链接没有新增警告。
- 108 帧图形 smoke 覆盖菜单/设置/三关/最终结算、暂停世界时间冻结、重载
  存档、4:3/超宽屏坐标和退出资源释放；注入键盘/鼠标/手柄快照走通设置
  保存、暂停/恢复/重试、返回菜单、继续游戏、覆盖确认取消与恢复焦点。
  smoke 保存目录隔离，每次默认起点，不修改正常用户进度。
- FORGE Debug/Release EXE 八种尺寸图标与源 ICO 一致；Release 确认 GUI
  子系统。截图及 `build/forge-preview.png` 已人工查看。
- 把完整游戏及 SDK 复制到 `build/forge-independent consumer` 含空格的新
  路径，实际独立编译/运行及图形 smoke 通过。CI 使用的 Release
  `-Verify -SkipGraphics` 路径实际通过，明确显示图形未运行。
- 图标/灯光/音频生成器在隔离目录重跑，13 个生成文件逐字节一致。
  PowerShell/Python 语法、actionlint、UTF-8/LF、git diff --check 通过。
- Android ARM64 Release 四 APK 重新打包，签名/ZIP 与 ELF 16 KiB 对齐/
  入口/静态运行库/字体许可/图标检查通过，复用编译缓存；未运行设备。
- 最终 IDE 配置为 Windows/MSVC x64 Release、build_examples=y、
  build_tests=y、example_console=n。

### 未运行的检查与已知边界

- 未做人工从头到尾通关/手感调优、真实手柄、多 DPI/多显示器/长稳测试、
  音频主观试听、Linux/macOS 或 Android 游戏移植/设备运行。程序化美术和
  全流程已交付，不能将本包描述为已通过商业发行/工业级质量全部验收。
- 游戏 UI 为英文；中文字体/本地化、按键重绑、完整无障碍、通用场景栈/
  预制体/地图动画导入尚未实现，本包没有将整个 M1 标为完成。
- M0 的真实设备初始化/显存耗尽错误路径、多 DPI/多显示器与长稳验收仍
  未闭合。Android 真机按用户此前要求继续暂缓，不安装/启动设备。
- 没有推送或运行新远端 CI。本包新增 Windows CI 的图形 smoke 仍跳过；
  Linux 分支保留，历史 `83be019` CI 成绩不能算成本包验收。
- MSVC Release 依赖 VC++ 2015–2022 x64 动态运行库，未在干净机器验收。
  依赖完整许可/发行符号/根目录 LICENSE/签名与发行渠道仍需后续闭合。

入口：`pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify`；
EXE：`games/forge_runner/build/windows/x64/release/forge_runner.exe`，保留相邻
assets。正常用户目录 `%LOCALAPPDATA%/raygame.forge`。详细操作/内容见游戏 README。
本地运行包 `build/forge/FORGE-windows-x64-release.zip` 含 EXE、assets 与说明；
它是本次可试玩包，未做干净机器发行验收。
日志：`build/forge-script-{debug,release}.log`、`build/forge/<mode>/*.log`；
图形截图/文件日志在各模式 smoke-data；原引擎回归
`build/forge-engine-{debug,release}-{config,build,tests,smoke,icons}.log`；
Android `build/forge-android-build.log` 与 `build/android/verification-release.json`。
不要删除 Android 本地签名密钥。

下一工作包：优先人工体验 FORGE 完整通关/音效/真实手柄，形成具体问题再
打磨关卡与表现；核对本提交远端 CI，然后按下方 M0 硬件矩阵推进。

## 上一工作包：独立桌面 SDK 消费与配置切换（已实现并验证）

用户要求继续剩余 TODO。本包选择产品评审 R12 的独立 SDK 消费验收，并修复
此前已记录的构建配置混用问题。开始时工作区干净，基线为 `7660d2b`。
提交主题：`Add standalone SDK consumption and reliable desktop mode switching`。

1. 新增 `build_examples` / `build_tests` 选项，默认均开启；同时关闭只定义
   引擎目标。可保留 tests/smoke 并关闭四个示例二进制，smoke 仍使用真实示例
   内容和原有资源。Android 默认四 APK 目标与打包路径保留。
2. 桌面引擎库移至 `build/lib/<plat>/<arch>/<mode>`，消除 Debug/Release
   相互覆盖。实际往返发现共用 `bin` 的 EXE 也会被模式独立的链接缓存误判；
   新增 `xmake/rules/desktop-config.lua`，在每个桌面 EXE 旁保存配置标记，
   模式/架构/运行库/编译器/控制台选项变化时仅强制重新链接。原 `bin/*.exe`
   路径、GUI 启动与文件诊断保留；同配置无改动构建继续复用产物。
3. `xmake install -o <prefix> raygame` 安装命名空间头文件
   `include/raygame/*.h`、静态库和 `share/raygame` 接入脚本/配置记录，不再
   额外安装一套根目录头文件。安装内容可以整体搬迁；消费目标调用
   `add_raygame_sdk()` 获得公开头文件、引擎库、固定版本 raylib 依赖及系统库。
   配置阶段明确拒绝平台/架构、模式及 Windows C 运行库不匹配。
4. 新增 `tests/sdk_consumer` 独立项目及 `scripts/verify-sdk.lua`。脚本将
   SDK 安装到新临时目录，再搬到含空格路径，复制并构建另一个独立消费项目。
   检查安装头文件内容与命名空间、独立包含/标准库互操作、物理/固定 tick、
   输入、存档和错误配置拒绝；可用 `--graphics` 检查真实隐藏窗口、程序生成
   纹理、四帧绘制与退出清理。全部成功才写通过报告，失败保留日志。
5. 桌面 Windows/Linux CI 增加无窗口独立 SDK 验收；说明见 `docs/SDK.md`
   和 README。没有改动引擎或示例游戏的 C++ 行为。

### 本包通过的检查

- MSVC 2022 x64 Debug/Release 全目标构建通过；两种模式逻辑测试各
  **311 passed, 0 failed**，图形 smoke 均 **0 failures**。
- 关闭示例/测试后只定义并成功构建 raygame；关闭示例且开启测试时，实际
  tests/smoke 构建运行通过。随后恢复所有目标。
- Debug (`MDd`) / Release (`MD`) 的 SDK 搬迁、独立编译/运行及图形验收
  全部通过；各模式三类错误配置被明确拒绝，恢复后再次运行通过。
  消费编译/链接日志确认只引用临时安装头文件/引擎库及 xmake raylib 缓存，
  不引用引擎源码、原公开头文件目录或 examples。Release 另运行 CI 使用的
  无窗口脚本通过；新版 SDK 消费编译没有新增警告。
- 新配置规则下实际 `Release → Debug → Release` 构建/运行通过，18 次检查
  确认六个 EXE 的 C 运行库随模式正确切换，四游戏每次均保持 GUI 子系统。
  每种模式的测试/smoke/八尺寸 EXE 图标检查通过；两种引擎库 SHA-256 保持
  不变，未使用 `-r` 强制重编译。相同模式无改动构建没有编译/归档/链接。
- Android ARM64 Release 四 APK 构建脚本实际通过，v2/v3 签名、ZIP/ELF
  16 KiB 对齐、入口、静态运行库、字体/许可及图标资源检查通过。本次复用
  原生编译缓存，不描述为完整 Android 原生重编译；没有安装或运行设备。
- `actionlint`、UTF-8/LF 与 `git diff --check` 通过。最终本地配置为
  Windows/MSVC x64 Release，`build_examples=y`、`build_tests=y`、
  `example_console=n`。

### 本包未运行的检查与边界

- 未运行本包 Linux/macOS 构建或新远端 CI。已核对此前 `83be019` 的远端
  [`37960859746`](https://github.com/keosu/raygame/actions/runs/37960859746)：
  五任务成功，Windows Debug/Release 各 311、Linux 各 310 项通过；四 EXE
  图标及 Android 打包通过，三个产物未过期。这属于历史提交的成绩；
  `7660d2b` 和本包尚无对应远端运行，未推送或触发远端执行。
- SDK 的 raylib-cpp 5.5.0/raylib 5.5 仍由 xmake 解析下载/缓存，未打成
  离线 SDK；相容编译工具链的 ABI、完整第三方许可/发行符号仍需验收。
  本包未实现独立 CMake 或 Android SDK 消费，也未在干净机器运行发行包。
- 四游戏与 tests/smoke 仍共享 bin 的 EXE 路径；配置标记保证构建时正确
  重新链接，不能同时保留两种模式 EXE。消费项目使用 xmake 默认配置隔离
  输出。示例字体/窗口图标属于游戏资源，SDK 不包含它们。
- 本包是 SDK 消费基础验收，不是完整参考游戏或整个 M0 完成。设备失败、
  多 DPI/多显示器、长稳和 Android 真机验收继续待办。

日志：`build/sdk-{debug,release}-{config,build,tests,smoke,icons}.log`、
`build/sdk-{engine-only,tests-only}-*.log`、`build/sdk-mode-*.log`、
`build/sdk-mode-switch-report.json`、`build/sdk-release-nochange-build.log`、
`build/sdk-verification-{debug,release,release-headless}.log`、
`build/sdk-android-build.log`、`build/ci-37960859746.log`。
SDK 分步日志/报告位于 `build/sdk-validation/windows/x64/<mode>`，实际临时
SDK/消费项目路径写入 `report.lua`；Release 最近报告为无窗口检查，图形检查
记录在 release/graphics.log。Android 报告为
`build/android/verification-release.json`。不要删除本地 Android 签名密钥。

下一工作包先核对本提交远端 Windows/Linux SDK CI，再按下方 M0/完整参考
游戏路线推进；本包已通过项无需重复实现。

## 上一工作包：示例目录归属与 xmake 拆分（已实现并验证）

按用户要求核对根目录资源归属并整理构建入口。开始时工作区干净，基线为
`83be019`。提交主题：`Organize example resources and split xmake target definitions`。

1. 示例 APK manifest/图标移至 `examples/android`；原创图标源文件移至
   `examples/artwork`；字体/许可/窗口 PNG 移至 `examples/assets`；四游戏
   Windows ICO/RC 同时移至 `examples/resources`。smoke 使用这些示例资源。
   67 个迁移文件中 66 个逐字节不变，图标 README 仅更新路径。
2. 原 `android/raylib.lua` 是引擎 Android 依赖配方，移至
   `xmake/packages/raylib.lua`，由顶层在 Android 配置下包含。
3. 顶层 `xmake.lua` 保留工程、依赖和引擎目标，包含 `examples/xmake.lua`
   与 `tests/xmake.lua`。示例选项、资源复制、四游戏及 APK 配置归前者；
   逻辑/图形 smoke 目标归后者，仍不在 Android 定义测试二进制。
   示例声明帮助函数改为 local，各子文件按自身目录解析源码和资源路径。
4. APK 签名密钥显式按工程根目录转为绝对路径，防止 native_app 规则按新的
   scriptdir 错误解析默认密钥。保留现有密钥、目标名、选项、默认目标行为、
   GUI 启动与日志，以及 `bin/assets`、`bin/android/<arch>/<mode>` 产物布局。
5. 图标生成/Windows 校验脚本及 README、Android 文档已更新；CI 调用入口和
   产物路径仍有效。没有修改引擎或示例游戏的 C++ 行为。

### 本包通过的检查

- MSVC 2022 x64 Debug/Release 全目标完整重建通过；每种模式均
  **311 passed, 0 failed**，图形 smoke 均 **0 failures**。
- 两种模式四 EXE 的八尺寸内嵌图标全部匹配；四 Release EXE 确认使用
  Windows GUI 子系统。将原 `bin/assets` 移到忽略目录备份后，资源目标重新
  复制全部 9 个文件，内容与 `examples/assets` 一致；Release smoke 在此基础上通过。
- 图标脚本在 `build/layout-icon-generator` 的隔离目录中实际运行，生成的
  52 个 ICO/RC、窗口 PNG、Android 图标/配置文件与已提交内容逐字节一致。
- Android 脚本配置/构建通过，并从新目录实际重新打包四个 ARM64 Release
  APK；v2/v3 签名、ZIP/ELF 16 KiB 对齐、入口、静态运行库、字体/许可及
  launcher/adaptive/themed 图标检查全部通过；四 APK 各自的全部 9 个 assets
  内容均与新源目录逐字节一致。没有改动 C++，本次复用原生
  编译缓存，不能描述为完整 Android 原生重编译。
- `actionlint`、PowerShell/Python 脚本语法、UTF-8/LF 与 `git diff --check`
  通过；最终桌面配置保持 MSVC Release、`example_console=n`。

### 未运行的检查与已知限制

- 本包未运行 Linux/macOS、远端 CI 或 Android 真机/模拟器。跨平台分支与
  CI 产物路径已保留，但此前远端成绩不能当作本包验证。
- Debug/Release 仍共用 `bin/raygame.lib`。本次首次切换 Debug 时，缓存判断
  未重归档引擎，遗留 Release 库引发 LNK2038；使用 `xmake build -r -a -j 1 -y`
  完整重建后通过。此问题属于已有产物布局；切换模式做本地矩阵验收时应完整重建。
- 子目录 xmake 文件由顶层包含，构建命令仍从仓库根目录运行；此包不是独立
  SDK 消费项目。后续先核对本提交的 Windows/Linux/Android CI，再推进本文末尾
  M0 验收和 SDK 消费主线。Android 运行验收继续按用户要求暂缓。

日志：`build/layout-{debug,release}-{config,build,tests,smoke,icons}.log`、
`build/layout-debug-rebuild.log`、`build/layout-assets-{copy,and-subsystem}.log`、
`build/layout-icon-generator.log`、`build/layout-ci-check.log`、
`build/layout-android-{build,assets}.log`；APK 报告为
`build/android/verification-release.json`。
资源备份位于 `build/layout-previous-assets`；不要删除 Android 本地签名密钥。

## 上一工作包：独立物理世界、更新阶段/插值与示例图标（已实现并验证）

用户要求先完成最高优先级的物理世界隔离和更新阶段/表现插值，且四游戏 EXE/APK
均使用独立图标；Android 真机验收暂缓。本包没有安装或启动 Android 设备。
提交主题：`Isolate scene physics, add fixed phases and interpolation, brand example icons`。

1. 每个 Scene 持有非全局的 PhysicsWorld，注册、接触表、清理代次和重入状态
   独立。切关/关闭只清当前场景物理；禁止把场景碰撞体注册到另一个世界。
   独立 GameObject 必须显式注册到独立世界，世界销毁会安全解除绑定。
2. 阶段固定为帧 Update → 全对象 FixedUpdate → 全刚体积分 → 当前世界碰撞
   与回调 → PostFixedUpdate → 应用 OnUpdate/场景 LateUpdate → Render。
   相机跟随移至 LateUpdate；积分不再依赖刚体和控制器的添加顺序。
3. 固定阶段的 GameTime::DeltaTime/GetTime 返回固定 dt/tick 时间，离开阶段
   或异常时恢复帧视图。FrameTiming 提供固定步数、插值系数、模拟时间、当前帧
   及累计丢弃的长帧时间；暂停时间不计为丢帧。F1 面板显示步数和丢弃时间。
4. Transform 提供不修改模拟状态的渲染 TRS/点变换，包含父子层级和最短角度插值。
   Rigidbody 默认启用，纯固定移动可显式启用。精灵/动画/地图和示例绘制接入，
   暂停、视口变化、重载、异常和 Teleport 重置历史。框架仍是单应用图形/输入/
   时间服务；独立物理世界不表示多个窗口或并行应用循环已受支持。
5. Breakout 的球/挡板，Space Shooter 的移动/射击和碰撞，以及 Bullet Hell 的
   弹幕/玩家/Boss/炮台均按固定阶段模拟；Bouncing Balls 在物理后处理边界。
   弹幕数组保存前后位置用于绘制，结算/视口变化消除历史拖影。
6. 四个原创 SVG 图标分别对应飞船、弹球、砖块挡板和弹幕核心。EXE 内嵌八种
   ICO 尺寸，运行窗口加载对应 PNG；APK 含五种密度、API 26 自适应图标和 API 33
   主题单色图层。源码和生成说明现位于 `examples/artwork/icons`，普通构建
   不需要 Python。
7. `scripts/verify-windows-icons.ps1` 比对 EXE 八种资源与原始 ICO，并已加入 CI；
   Android 构建脚本新增图标资源检查。README 已提供阶段契约、插值和 API 迁移。

### 本包最终验证

- MSVC 2022 x64 Debug/Release 全目标构建通过，每种模式 **311 passed, 0 failed**。
  新增多世界嵌套步进/隔离清理、1,000 次清理后注册/接触稳定性、刚体顺序无关、
  30/60/144 FPS 控制运动、层级插值、暂停/重载/异常恢复和长帧统计检查。
- Debug/Release 图形 smoke 均 **0 failures**，包括插值像素与物理位置不变断言、
  运行窗口图标、原四游戏 × 四比例、全屏/主题/资源和异常清理。
- 四个 Release EXE 的八种图标图像均与各自 ICO 精确匹配。
- 四个 ARM64 Release APK 重新生成，位于 `bin/android/arm64-v8a/release/`，
  各约 1.36–1.38 MB；签名、16 KiB 对齐、入口、字体/许可及图标资源检查通过。
  aapt 实际包信息确认各 manifest 引用了自定义 launcher 图标。
- 图标预览 `build/icons-preview.png` 和四游戏布局预览
  `build/worlds-examples-preview.png` 已人工查看。`actionlint` 和
  `git diff --check` 通过；最终桌面配置仍为 MSVC Release、无控制台。
- 本包未运行 Linux/macOS、新 GitHub 工作流、Android 真机/模拟器；先前 CI
  成绩仍属于 `a9ce490`，不能当成本包远端验证。控制运动回归也不代表所有随机
  游戏逻辑已实现确定性回放，图形 smoke 没有自动全图差异基线。

日志：`build/worlds-{debug,release}-{config,build,tests,smoke}.log`、
`build/worlds-icons-check.log`、`build/worlds-android-build.log`；APK 校验报告仍为
`build/android/verification-release.json`。不要删除本地 Android 调试签名密钥。

迁移要点：旧 `PhysicsWorld::Step/Clear/...` 改为 `scene.GetPhysicsWorld()` 的实例
方法；`Step/CheckCollisions` 只处理碰撞。手动循环需显式执行积分与后处理；
直接 `Rigidbody::FixedUpdate()` 积分改为 `Integrate(dt)`。禁用碰撞体保留所属世界
绑定但不参与检测；Clear 不发送销毁期碰撞事件。Scene 的 Clear/Load 规则不变：
组件遍历期间不能立即重新创建对象。异常恢复清掉追帧和插值历史，不回滚业务写入。

## 上一工作包：CI 构建和产物下载（远端已验证）

提交主题：`Build and upload Windows games and Android APKs in CI`。
实现提交：`a9ce490`；2026-10-09 已核对对应远端运行和产物。

- `.github/workflows/ci.yml` 保留 Windows/MSVC 和 Linux Debug/Release 构建与
  逻辑测试，增加 Windows 四 EXE/字体资源上传；新增 Android ARM64 Release
  构建及签名/打包校验，上传四 APK 和 `verification.json`。产物保留 14 天。
- 增加 `workflow_dispatch`，保留 `main` push/PR 触发。固定 xmake 3.1.1，
  runner 为 Windows 2022 / Ubuntu 24.04。Android 用 JDK 17、SDK 35、
  build-tools 35.0.0、NDK r28c，复用 `scripts/build-android.ps1`。
- CI 使用每次新建的临时调试签名，密钥不上传。不同运行的 APK 不能直接覆盖
  安装；卸载会清除数据，详见 `docs/ANDROID.md`。没有自动安装、模拟器或发布流程。
- README 和 Android 文档已说明产物名、下载路径、签名限制和手动运行方法。

本包验证：`actionlint 1.7.12`、Android 步骤 PowerShell 语法检查和
`git diff --check` 通过，记录在 `build/ci-validation.log`。
已确认 xmake v3.1.1 的 Android `xmake.lua/load.lua/package.lua` 规则与本机
已成功构建 APK 的版本一致。仅改 CI/文档，未重复执行上一包已通过的编译/游戏测试。

远端状态：用户已推送，GitHub 运行
[`37953515827`](https://github.com/keosu/raygame/actions/runs/37953515827)
对应 `a9ce490`，五个任务均成功。Windows Debug/Release 各 **259 passed,
0 failed**，Linux Debug/Release 各 **258 passed, 0 failed**（平台专有检查数量
不同）。Android 四个 APK 构建和脚本验证成功，v2/v3 签名均通过。

三个产物 `raygame-windows-x64-{debug,release}`、`raygame-android-arm64-release`
已上传且检查时未过期；Windows 每份 9 个文件，Android 5 个文件。远端日志保存在
`build/ci-37953515827.log`。本次仅核对任务、日志和上传记录，没有下载并运行这些
CI 产物；CI 不执行图形 smoke 或 Android 运行验收，APK 继续由用户手动验证。

后续工程维护：日志有部分 Actions 使用 Node 20 的弃用提示，目前实际运行成功，
可单独升级兼容 Node 24 的 action 版本。本轮仅更新验证记录，没有修改运行时代码。

## 上一工作包：Android APK（已实现并验证构建）

用户要求尝试生成 APK，并明确由其手动安装/验证；不要运行 adb 安装或模拟器。
本机已有 `$USERPROFILE/.cache/balanceball-tools/jdk/jdk-17.0.18+8` 和 Android 34
SDK，以及 `.android` 中的工具/调试密钥。本包使用现有 JDK；SDK 35 和 NDK r28c
已下载至 `$LOCALAPPDATA/raygame/android-sdk`，未覆盖已有工具或密钥。

提交主题：`Add Android APK builds and touch controls for example games`。
提交号：`a79fd50`。

1. xmake 将四游戏构建为 NativeActivity 共享库/APK，ARM64、API 26 起、target
   API 35、OpenGL ES 2.0。Release 原生代码使用本地调试签名，适合侧载测试。
   raylib 固定为 5.5。原始 xmake-repo 配方的 `--wrap=fopen` 导致真实链接失败，
   仓库内 raylib 配方（现位于 `xmake/packages/raylib.lua`）去掉重复包装并
   保留链接检查，未修改系统包仓库。
2. Android 入口桥接在启动日志前取得 Activity；日志和存档写应用私有目录，
   日志同时写入 logcat。字体及许可打入 APK；`FileSystem::AssetExists` 支持
   AssetManager，着色器存在检查也使用该接口。
3. 输入快照支持最多 8 个触点及 ID，位置按当前 Display 画布映射。四游戏增加
   触屏操作、暂停/退出菜单；系统返回键切换暂停。前三游戏横屏、弹幕游戏竖屏，
   同方向内可传感器翻转。Android 使用原生 Surface 尺寸并保留 Expand 等比适配。
4. 修复暂停页按钮被禁用的背景按钮清除按压状态的问题，暂停/切场景时显式重置
   UI 交互。Bouncing Balls 重开改用 `Clear(); Load();`，维持回调作用域契约。
5. `scripts/build-android.ps1` 自动发现工具、生成独立调试密钥、构建并检查 APK。
   Android 配置放在 `build/android/config`；产物、对象文件与桌面隔离，调用结束
   恢复环境。工具链、签名密钥、日志和 APK 不提交到 Git。

### 本包最终验证

- 四个 ARM64 Release APK 实际构建成功，位于
  `bin/android/arm64-v8a/release/{space_shooter,bouncing_balls,neon_breakout,bullet_hell}.apk`，
  各约 1.1 MB。签名 v2/v3、ZIP 对齐、ELF LOAD 段 16 KiB 对齐、ARM64 ABI、
  NativeActivity 导出入口、静态 C++ 运行库和字体/许可检查全部通过。
- 构建脚本已实际执行；四份 SHA-256/路径/元数据记录在
  `build/android/verification-release.json`，`RuntimeTested=false`。
- MSVC 2022 x64 Debug、Release 全目标构建通过；每种模式 **259 项逻辑检查
  通过，0 失败**，图形 smoke 均 **0 failures**。新增触点快照/固定 tick/失焦
  检查，并在图形 smoke 验证文件资源查询、四游戏四比例的触点映射和菜单按压。
- 最终桌面配置为 Windows x64、MSVC Release、`example_console=n`。
- `git diff --check` 通过。本包未执行 Android 真机/模拟器、Android Debug、
  其他 ABI 或 Linux/macOS 构建/运行验证；不把构建检查视为 Android 运行验收。

日志：`build/android-script-build.log`、
`build/android-desktop-{debug,release}-{config,build,tests,smoke}.log`。
依赖原始失败记录：`build/android-dependency-failure.log`。
运行 `pwsh -NoProfile -File scripts/build-android.ps1` 可重新生成全部 APK。
保持 `build/android/debug.keystore`，覆盖安装依赖同一签名；未改用户 `.android` 密钥。
构建说明、各游戏操作和手动验收项见 `docs/ANDROID.md`。

Android 当前限制：未验证后台/锁屏恢复、GPU 上下文丢失后资源重建、刘海/手势
安全区域、性能/耗电/长稳；多点触控路由和弹幕独立慢速按钮尚未实现。下一步先
根据用户真机反馈修复，再按本文末尾 M0 主线推进，不能将整个 M0 标记完成。

## 已完成基线

- `04cdfad`：0.2 引擎改进与 MSVC 构建修复。
- `69565f1`：全屏、字体/HUD、原生标题栏与 GUI 子系统。
- `7039164`：生命周期异常清理、时间域、安全存档、文件日志、四游戏比例适配。
- `d95cdb9`：类型资源句柄、固定 tick 输入缓冲与回调所有者作用域。
- `a79fd50`：Android APK 构建、触屏操作、私有文件与 APK 资源适配。
- `a9ce490`：CI 构建/上传 Windows EXE 和 Android APK，五任务已在 GitHub 通过。
- 历史工作包先后通过 MSVC Debug/Release 各 206、252 项逻辑检查，图形 smoke
  零失败；本包新增 Android 支持后的最终成绩见上文。

## 上一工作包：资源、输入和回调作用域

提交主题：`Add typed resources, buffered input and callback ownership scopes`。
提交号：`d95cdb9`。以下为该包实现和历史验证记录。

1. 纹理/字体/着色器迁移至类型句柄，共享稳定元数据。显式卸载立即失效；
   同名重建不复活旧句柄；成功替换更新引用和修订号，失败保留旧值并记录错误。
   原生资源仍由管理器在图形线程释放，句柄析构仅释放元数据。
2. SpriteRenderer、SpriteAnimation、Tilemap、UI 和示例迁移为句柄。
   无效精灵/地图绘制洋红占位，字体回退到 raylib 默认字体；加载失败不会把
   raylib 默认字体/着色器当作拥有的资源卸载。较重资源实现移至 `.cpp`。
3. 可注入 `InputFrame`，引擎每帧采样一次。固定 tick 缓存并消费边沿，零步
   不丢短按、多步不重复；处理暂停/恢复、失焦、设备断开、切场景与异常复位。
   鼠标原始位置按当前画布映射，动作绑定在输入重置后保留。
4. `CallbackScope/ScopeToken` 支持应用/场景/对象/组件生命周期。事件和补间
   在回调中自动继承所有者；销毁立即取消并释放捕获；自取消、嵌套派发、回调
   异常及捕获析构引发的取消均有回归。应用订阅跨关卡保留；Shutdown 取消
   初始化前已注册的应用回调。物理碰撞和计时器回调继承组件作用域。
5. 场景清理可更新作用域；手动重载使用 `Clear(); Load();`。Breakout 和
   BulletHell 已去掉重开时的全局事件/补间清空。README 提供 API 迁移与契约。

## 上一工作包验证（历史基线）

MSVC 2022 x64，通过 xmake 验证：

- Debug、Release 全目标构建通过；两种模式各 **252 项逻辑检查通过**。
- Debug、Release 图形 smoke 均 **0 failures**。包括真实纹理/字体/着色器
  卸载/替换/关闭、占位绘制像素检查、四游戏 × 四比例的 16 组布局、原生标题栏、
  全屏往返和初始化/渲染异常清理。
- 逻辑回归包括 30/60/144 FPS 输入、短按、暂停/失焦、异常恢复、作用域释放和
  1,000 次场景清理/重载后的订阅行为；这不是 1,000 次完整 GPU 关卡压力测试。
- 四个 Release 游戏实机启动后正常关闭，Exit=0；GUI 子系统、stdout/stderr
  为空、用户目录日志更新均通过。未结束用户自己的进程。
- 人工查看四游戏代表比例截图，字体、资源和布局正常。尚无全图差异自动基线。
- `git diff --check` 通过。没有执行本批 Linux/macOS 构建或运行验证。

日志位于忽略目录：`build/m0-{debug,release}-{build,tests,smoke}.log`；
启动记录 `build/m0-release-launch.json`，截图 `bin/screenshots`，汇总预览
`build/m0-preview.png`。最终本地配置保留 Release、`example_console=n`。

## 验证入口

```powershell
xmake f -p windows -a x64 --toolchain=msvc -m debug --example_console=n -y
xmake build -a -j 1 -y
xmake run tests
xmake run smoke
```

Release 使用 `-m release`。串行构建减少本机偶发 EXE 文件锁冲突；不要结束
用户启动的进程。图形 smoke 需要 OpenGL 环境。新日志放在 `build/`，不要
覆盖已跟踪的 `build_err.txt`。PowerShell 使用无 profile 模式，源码 UTF-8/LF。

## 当前边界与下一步

本包已完成，M0 尚未整体完成。无需重复实现或重跑本包已有通过项，除非后续
改动影响它们。继续前先核对工作区与本记录，再按路线推进：

1. **补齐 M0 验收**：真实设备初始化/显存失败、多 DPI/多显示器、跨平台与长稳
   验证；独立物理世界、固定阶段、插值与丢帧观测已完成，不要重复实现。
2. **参考游戏与 SDK 后续**：FORGE 三关/菜单/设置/存档/独立 SDK 接入已
   实现，目录整理已完成，入口为 `examples/forge_runner`。先做人工完整体验
   与真实输入设备验收；核对远端 CI，再推进按键重绑/无障碍、通用内容
   序列化/管线和发行包验收；中英文切换已完成，测量字体图集大小和首次
   创建耗时后再决定字号量化/预热等优化，勿重复实现现有流程。
3. **后续工程化**：CI 图形 smoke、Actions Node 24 迁移、许可/发行包与稳定签名。

并行于主线的待验收事项：Android 真机启动、字体/触控、存档、后台/锁屏恢复、
安全区域和长稳性能尚无用户反馈记录，用户已明确暂缓；不代为安装或启动设备。发行前
还需稳定签名、干净机器运行、SDK 发行与许可文件，CI 产物生成不能替代这些。

实现边界：资源为同步图形线程 API，借用原生指针不能跨替换/卸载保存；句柄不
支持外部原生资源的所有权导入。输入边沿以布尔值合并，不保存多次转换顺序，
不是完整录制回放。事件/补间仍是静态入口，作用域限运行时线程；组件禁用时
保留订阅，销毁/显式取消时移除；直接调用用户 override 需要显式激活作用域。
