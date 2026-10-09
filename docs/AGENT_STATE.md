# Agent 工作状态

更新时间：2026-10-09。

## 目标与约束

按 `PRODUCT_READINESS_REVIEW.md` 逐步产品化 RayGame。当前推进 M0，继续保留
GameObject/Component 和 raylib，维持四个游戏可用。用户明确要求持续保存 agent
状态；每个工作包结束及交接前更新本文件。仓库 `AGENTS.md` 已固化该规则。

用户偏好：中文交流，直接推进已授权工作，验证后提交；示例无控制台但保留
文件日志，标题栏跟随 Windows 主题，游戏适配窗口比例。未授权创建子 agent。

## 最近工作包：CI 构建和产物下载（远端已验证）

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
   仓库内 `android/raylib.lua` 去掉重复包装并保留链接检查，未修改系统包仓库。
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

1. **物理世界隔离**：将全局 PhysicsWorld 逐步迁移为场景/运行上下文拥有的
   实例；保留必要兼容入口。验证两个世界分别注册/步进/清理互不影响、切关无泄漏。
2. **更新阶段与表现插值**：明确固定步物理前/后和表现更新顺序、插值及丢帧
   观测；把 Breakout 等混用变帧移动与固定碰撞的行为迁移到明确的时间域。
3. **补齐 M0 验收**：真实设备初始化/显存失败、多 DPI/多显示器、跨平台验证。
   R04 和整个 M0 不能仅凭本包标记完成。
4. 再推进 M1：外部消费项目、完整参考游戏的菜单/设置/多关流程、版本化内容
   序列化、资源管线和中文文字，具体验收见产品化评审文档。

并行于主线的待验收事项：Android 真机启动、字体/触控、存档、后台/锁屏恢复、
安全区域和长稳性能尚无用户反馈记录；按用户要求不代为安装或启动设备。发行前
还需稳定签名、干净机器运行、独立 SDK 消费和许可文件，CI 产物生成不能替代这些。

实现边界：资源为同步图形线程 API，借用原生指针不能跨替换/卸载保存；句柄不
支持外部原生资源的所有权导入。输入边沿以布尔值合并，不保存多次转换顺序，
不是完整录制回放。事件/补间仍是静态入口，作用域限运行时线程；组件禁用时
保留订阅，销毁/显式取消时移除；直接调用用户 override 需要显式激活作用域。
