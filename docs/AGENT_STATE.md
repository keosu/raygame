# Agent 工作状态

更新时间：2026-10-09。

## 目标与约束

按 `PRODUCT_READINESS_REVIEW.md` 逐步产品化 RayGame。当前推进 M0，继续保留
GameObject/Component 和 raylib，维持四个游戏可用。用户明确要求持续保存 agent
状态；每个工作包结束及交接前更新本文件。仓库 `AGENTS.md` 已固化该规则。

用户偏好：中文交流，直接推进已授权工作，验证后提交；示例无控制台但保留
文件日志，标题栏跟随 Windows 主题，游戏适配窗口比例。未授权创建子 agent。

## 最近工作包：FORGE 中英文切换与清晰中文字体（已实现并验证）

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
   实现，先做人工完整体验与真实输入设备验收；核对远端 CI，再推进
   按键重绑/无障碍、通用内容序列化/管线和发行包验收；中英文切换已完成，
   勿重复实现现有流程。
3. **后续工程化**：CI 图形 smoke、Actions Node 24 迁移、许可/发行包与稳定签名。

并行于主线的待验收事项：Android 真机启动、字体/触控、存档、后台/锁屏恢复、
安全区域和长稳性能尚无用户反馈记录，用户已明确暂缓；不代为安装或启动设备。发行前
还需稳定签名、干净机器运行、SDK 发行与许可文件，CI 产物生成不能替代这些。

实现边界：资源为同步图形线程 API，借用原生指针不能跨替换/卸载保存；句柄不
支持外部原生资源的所有权导入。输入边沿以布尔值合并，不保存多次转换顺序，
不是完整录制回放。事件/补间仍是静态入口，作用域限运行时线程；组件禁用时
保留订阅，销毁/显式取消时移除；直接调用用户 override 需要显式激活作用域。
