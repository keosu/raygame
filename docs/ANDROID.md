# Android APK 构建与手动验证

当前提供五个 **实验性 Android APK**（含 FORGE）：Android 8.0 / API 26 起、ARM64
(`arm64-v8a`)、OpenGL ES 2.0，target SDK 35。已实际交叉编译并检查 APK，
尚未在真机或模拟器运行；用户负责本轮安装验证。

## 产物与操作

Release 原生代码使用本地调试密钥签名，适合侧载测试。它不是应用商店发行包。

| APK（位于 `bin/android/arm64-v8a/release/`） | 屏幕方向 | 触屏操作 |
| --- | --- | --- |
| `space_shooter.apk` | 横屏，可反向横屏 | 在游戏区域拖动移动，按住自动射击，飞船位于手指上方 |
| `bouncing_balls.apk` | 横屏，可反向横屏 | 点击场地添加球；底部 RESET 重置、GRAVITY 切换重力 |
| `neon_breakout.apk` | 横屏，可反向横屏 | 拖动移动挡板，点击发球，结算页点击 PLAY AGAIN 重开 |
| `bullet_hell.apk` | 竖屏，可反向竖屏 | 在游戏区域拖动移动，按住自动射击；结算页可重开 |
| `forge_runner.apk` | 横屏，可反向横屏 | 左侧箭头移动；右侧跳跃/冲刺，可多指同时操作；下箭头 + 跳跃穿过单向平台 |

四个小示例都有右下角 PAUSE、暂停页 RESUME/EXIT；系统返回键切换暂停。
保留外接键盘的原有操作。触屏版本暂未实现弹幕游戏的独立慢速移动按钮。
同一方向内使用传感器允许屏幕翻转，不在本轮开放任意横竖方向切换。

FORGE 使用右上暂停按钮和完整触屏菜单；设置行左半侧减少、右半侧增加，支持
中英文切换。系统返回键切换暂停、退出设置/制作信息，在主菜单保存并退出。
默认 60 FPS、系统全屏和 1280×720 Fit 等比画布，保留键盘/手柄操作。跳跃可
按住控制高度，再次点击二段跳；控制按钮按触点 ID 跟踪，多指及触点顺序变化
不会把跳跃变成冲刺。页面切换、失焦与窗口映射变化会清除操作并等待手指松开。

每个游戏有独立图标：Windows EXE 内嵌多尺寸 ICO，Android 包含普通密度图标、
Android 8+ 自适应图标和 Android 13+ 主题单色图层。原始 SVG、生成工具和尺寸说明
见 [示例图标说明](../examples/common/ICONS.md)；FORGE 的生成器是
`examples/forge_runner/tools/generate-android-icons.py`。图标形状由系统或启动器决定。

每个游戏的 manifest 和启动器资源位于 `examples/<game>/android/`，自己的
运行资源位于 `examples/<game>/assets/`，共用字体和许可在
`examples/common/assets/fonts/`。构建把共用与专用资源合并到
`build/android/example-assets/<game>/`，各 APK 只包含自己的资源；窗口 PNG
为 `assets/icon.png`，字体为 `assets/fonts/*`。增量构建会清除 staging 中已
移除的文件，避免旧资源遗留。FORGE 使用自己的全部资源，不合并四小示例的字体：
暂存于 `build/android/example-assets/forge_runner/forge/`，APK 内为
`assets/forge/`，另包含 `assets/THIRD_PARTY_NOTICES.md`。桌面独立 SDK 工程保留。

包名分别是 `org.raygame.spaceshooter`、`org.raygame.bouncingballs`、
`org.raygame.neonbreakout`、`org.raygame.bullethell`、`org.raygame.forge`，五个包可同时安装。
Android 首次侧载时按系统提示允许相应文件管理器安装应用。后续更新保留同一
调试密钥，才能直接覆盖安装并保留数据；不要删除 `build/android/debug.keystore`。
密钥、工具链与 APK 均不提交到 Git。

## Windows / PowerShell 一键构建

依赖：xmake 3.1.1+、JDK 17+、Android SDK platform 35、build-tools 35.0.0、
NDK r28c (`28.2.13676358`)。不需要 Gradle、Java 游戏 Activity 或 Android Studio 项目。
也可用 Android Studio/命令行 SDK Manager 安装所需组件：

```powershell
sdkmanager 'platforms;android-35' 'build-tools;35.0.0' 'ndk;28.2.13676358'
```

本轮缓存工具可直接运行：

```powershell
pwsh -NoProfile -File scripts/build-android.ps1
# 只生成一个游戏：
pwsh -NoProfile -File scripts/build-android.ps1 -Game neon_breakout
pwsh -NoProfile -File scripts/build-android.ps1 -Game forge_runner
```

脚本查找 `ANDROID_HOME` / `ANDROID_SDK_ROOT`、`ANDROID_NDK_HOME` /
`ANDROID_NDK_ROOT`、`JAVA_HOME`，以及已知的本机工具缓存。可显式指定：

```powershell
pwsh -NoProfile -File scripts/build-android.ps1 `
    -SdkRoot 'C:/Android/Sdk' `
    -NdkRoot 'C:/Android/Sdk/ndk/28.2.13676358' `
    -JavaHome 'C:/Java/jdk-17'
```

默认 ARM64、Release、4 个编译任务；`-Mode debug` 可构建含调试信息的版本。
脚本不会安装、启动设备或修改全局环境。它在 `build/android/config` 下使用
独立的 `XMAKE_CONFIGDIR`，恢复调用者的 JAVA_HOME/PATH/config 环境，不改变
普通 `xmake` 命令使用的桌面配置。

签名密钥首次自动生成到 `build/android/debug.keystore`，使用标准调试密码
`android`，与用户 `~/.android/debug.keystore` 分开。每个 APK 会检查签名、
ZIP 对齐、ARM64 ELF 的 16 KiB LOAD 段对齐、NativeActivity 导出入口、静态
C++ 运行库以及各自资源/共用字体和许可的 SHA-256；缺失、多余或内容不一致的
运行资源会使检查失败。SHA-256 等结果存入
`build/android/verification-release.json`（Debug 对应 `verification-debug.json`）。
构建检查也覆盖 APK 中的普通、自适应和主题图标资源。

## GitHub Actions 构建与下载

`.github/workflows/ci.yml` 的 Android 任务在 Windows 2022 runner 上调用同一
脚本，固定 xmake 3.1.1、JDK 17、SDK 35/build-tools 35.0.0 和 NDK r28c。
推送到 `main`、面向 `main` 的 PR，或在 Actions → CI → Run workflow 手动触发
后，成功运行的 Artifacts 中提供 `raygame-android-arm64-release`，含五个 APK
及 `verification.json`，保留 14 天。同一工作流还构建并上传 Windows x64
Debug/Release EXE 和字体资源，Linux 保留 Debug/Release 构建与逻辑测试。

CI 每次生成独立的临时调试密钥，不上传密钥，也不需要配置仓库签名 Secret。
因此不同运行或本地与 CI 之间的 APK 签名不同；需要先卸载旧版本再安装，卸载
会清除应用数据。需要持续覆盖安装时，继续使用保留同一调试密钥的本地构建。
CI 检查构建/签名/打包，不安装或启动 Android 应用。

## 直接使用 xmake（Linux/macOS 也可参考）

配置好 JAVA_HOME/PATH，生成上述 debug.keystore 后：

```text
xmake f -p android -a arm64-v8a -m release --toolchain=ndk --ndk=<NDK目录> --android_sdk=<SDK目录> --ndk_sdkver=26 --build_toolver=35.0.0 --runtimes=c++_static -y
xmake build -j 4 -y
```

这些直接命令会切换当前目录的 xmake 配置；Windows 推荐使用前面的隔离脚本。
本轮只在 Windows 主机实际构建了 ARM64 Release APK，其他主机/ABI 尚未验证。

## 实现与平台边界

- xmake `android.native_app` 负责 NativeActivity APK 打包；raylib 5.5 负责
  Android 事件循环。`--wrap=android_main` 在进入游戏 `main()` 前保存 Activity
  上下文，日志初始化就能使用应用私有目录。
- Android 日志同时进入 `logcat`（tag `raygame`）和应用私有
  `files/raygame.<游戏名>/logs/game.log`；Breakout 存档也在应用私有目录。
  不请求外部存储或网络权限，原子存档仍走普通 POSIX 文件系统。
- 字体打入 APK 的 `assets/fonts`，raylib 使用相对 assets 根目录的
  `fonts/Barlow-Medium.ttf` 等路径读取。`FileSystem::AssetExists` 同时支持
  APK 资源查询，不能用普通 `std::filesystem::exists` 判断 APK 内文件。
  FORGE 使用 `forge/fonts/...`；其关卡/翻译通过 `FileSystem::ReadAssetText`
  和 AssetManager 读取，支持压缩资源与大小上限。普通 `ReadText/WriteAtomic`
  仍用于应用私有存档，不把存档查询混入 APK 资源。
- Android 输入帧保存最多 8 个触点 ID/位置；`Input::GetTouchPosition` 按当前
  Display 画布换算。Android 由原生 Surface 决定实际窗口尺寸，画布继续使用
  四小示例使用 Expand、FORGE 使用 Fit 等比适配；桌面专用标题栏/窗口大小
  API 不会在 Android 正常游戏流程中调用。
- xmake-repo 的 raylib 5.5 Android 配方添加了 `--wrap=fopen`，与该版本
  自带 `android_fopen` 重复，实测会出现 `undefined symbol: __wrap_fopen`。
  `xmake/packages/raylib.lua` 提供固定版本和源文件校验和的本地配方，去掉错误包装，
  保留实际链接检查。它仅在 Android 配置下生效，不修改系统包仓库。
- 原生库显式禁止未解析符号，并设置 16 KiB ELF 对齐；APK 中库压缩存放、由
  系统解压后加载。当前 C++ 运行库静态链接，不遗漏 `libc++_shared.so`。
  引擎静态库在 Android 显式启用 `-fPIC`，避免 Debug 的全局变量/TLS 引用
  无法链接入 ARM64 NativeActivity 共享库。

## 本轮验证与下一步

已检查：五个 Release APK 和 FORGE Debug APK 构建、v2/v3 签名、ZIP/ELF 对齐、
原生入口、图标和完整资源。MSVC Debug/Release 各 316 项引擎、69 项 FORGE
逻辑检查通过，原图形 smoke 和 FORGE 桌面/触屏模拟 smoke 均为零失败；
完整记录见 `AGENT_STATE.md`。本包尚未推送或运行新的远端 CI。
这些检查不能替代 Android 运行验证。

手动验证建议先看：能否启动和显示字体、触屏移动/发球/菜单是否响应、暂停后
返回游戏是否正常，以及 Breakout 退出重开后最高分是否保留。随后再检查切换
后台/前台、锁屏、手势导航与刘海区域、传感器翻转及长时间运行表现。若失败，
记录机型、Android 版本、触发步骤和 `raygame`/`AndroidRuntime` 的 logcat。

FORGE 还需检查多指移动 + 跳跃 + 冲刺、下落单向平台、菜单/语言切换、三关
内容及重新启动后的检查点恢复。桌面触屏模拟已经覆盖操作路由，不能替代
Android 的真实触点采样、音频、AssetManager 和生命周期运行验收。

GPU 上下文彻底丢失后的资源重建、安全区域布局、完整多点触控路由、低内存/
后台恢复和性能/耗电仍需真实设备验收。Android 当前属于试验平台，不表示 M0
或整个产品级框架已完成。
