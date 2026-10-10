# Agent 工作状态

更新时间：2026-10-10。

## 目标与约束

按 `PRODUCT_READINESS_REVIEW.md` 逐步产品化 RayGame。当前推进 M0，继续保留
GameObject/Component 和 raylib，维持原四个游戏及 FORGE 可用。用户明确要求持续保存 agent
状态；每个工作包结束及交接前更新本文件。仓库 `AGENTS.md` 已固化该规则。

用户偏好：中文交流，直接推进已授权工作，验证后提交；示例无控制台但保留
文件日志，标题栏跟随 Windows 主题，游戏适配窗口比例。未授权创建子 agent。

## 最近工作包：跨平台工具、代码格式与示例体验（已实现并本地验证）

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
