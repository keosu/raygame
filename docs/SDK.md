# 独立项目接入 RayGame

桌面项目可以安装引擎的公开头文件和静态库，再通过 xmake 接入。消费项目只
引用安装目录；无需引用引擎的 `src`、`examples` 或测试文件。当前依赖仍由
xmake 下载/缓存：raylib-cpp 5.5.0 和 raylib 5.5，SDK 不包含它们的二进制。

## 构建与安装

在引擎仓库根目录执行（Windows/MSVC）：

```powershell
xmake f -p windows -a x64 --toolchain=msvc -m release --build_examples=n --build_tests=n -y
xmake build -y raygame
xmake install -o C:/SDK/raygame/windows-x64-release raygame
```

Linux 使用 `xmake f -p linux -a x86_64 -m release --build_examples=n --build_tests=n -y`，
然后构建并安装到可写目录。Debug 使用 `-m debug`，并选择独立安装目录。
安装时明确指定 `raygame`，避免安装示例。安装目录结构：

```text
include/raygame/*.h
lib/raygame.lib                  # Linux 为 libraygame.a
share/raygame/xmake.lua          # 可搬迁的接入脚本
share/raygame/sdk.lua            # 版本、平台、架构、模式和系统依赖
```

Windows 默认使用 Release `MD` / Debug `MDd`。消费项目必须使用相同平台、架构、
模式、运行库和兼容的编译工具链；接入脚本检查前四项，编译工具链的 ABI 兼容
仍由使用者保证。SDK 目录可以整体搬迁；需要在消费项目重新配置新的路径。
SDK 本身没有打包示例字体、窗口图标或 APK 资源，游戏负责自己的资源与许可。
这套接入目前面向桌面 xmake 项目，Android SDK 打包与独立 CMake 接入尚未验收。

## 消费项目

在另一个目录创建 `xmake.lua`：

```lua
set_project("my_game")
set_languages("c++17")
set_encodings("utf-8")
add_rules("mode.debug", "mode.release")

option("raygame_sdk")
    set_showmenu(true)
    set_description("Installed RayGame SDK directory")
option_end()

local sdk = get_config("raygame_sdk")
if sdk then
    includes(path.join(sdk, "share/raygame/xmake.lua"))
    target("my_game")
        set_kind("binary")
        add_files("main.cpp")
        add_raygame_sdk()
    target_end()
end
```

使用 `#include <raygame/RayGame.h>`，或按需包含
`<raygame/Scene.h>`、`<raygame/Input.h>` 等公开头文件。

```powershell
xmake f -p windows -a x64 --toolchain=msvc -m release --raygame_sdk=C:/SDK/raygame/windows-x64-release -y
xmake build -y
xmake run my_game
```

若希望 Windows 游戏启动时无控制台，在自己的目标内添加
`add_ldflags("/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup", {force = true})`，
并使用 `Diagnostics` 记录文件日志。独立接入不会自动套用示例游戏的启动策略。
游戏自带暂停/设置菜单时，调用 `SetDefaultShortcutsEnabled(false)` 接管引擎默认
Escape/P/F1/F11/Alt+Enter（Android 同时关闭默认返回键/触屏暂停按钮）。设置
音乐音量使用 `AudioManager::SetMusicVolume(name, value)`，避免重新播放音乐。

中文字体须显式指定 Unicode 字符集，默认字体加载只包含西文字符：

```cpp
std::vector<int> glyphs = {'?', 'A', ' ', 0x7ee7, 0x7eed}; // 继续
auto font = RayGame::ResourceManager::LoadFontResource("ui-zh", "assets/Chinese.ttf", 48, glyphs);
```

字体文件必须包含这些字形；列表会复制给 raylib，调用者无需延长其生命周期。
同名加载复用当前资源，修改字号/字符集使用相同参数的 `ReloadFont` 重载。
字符集最多 8,192 个有效 Unicode 标量；非法码点/过长列表拒绝加载，替换失败
保留旧句柄。FORGE 从翻译目录收集字符并按字号加载，仍非动态字形缓存/文字塑形。

完整独立消费例子见 [FORGE](../examples/forge_runner/README.md)：三关横版平台游戏，
含菜单、音频、存档和版本化关卡文件。根工程默认已包含该游戏，使用
`xmake run forge_runner` 运行；要验收安装后的 SDK，根目录运行
`pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify` 可构建并验收。

`FileSystem::ReadAssetText(path, text, maxBytes)` 用于关卡/翻译等只读文本资源：
桌面读取 UTF-8 文件路径，Android 从 APK assets 根目录通过 AssetManager 读取，
支持压缩资源与大小上限，失败保留原输出。`ReadText` / `WriteAtomic` 仍只操作
普通文件，供私有存档使用。FORGE Android 目标在根工程中构建，其 APK 资源路径
为 `forge/levels/...`、`forge/locales/...`；独立 SDK 消费验收仍面向桌面。

## 可重复的消费验收

构建好当前配置的 `raygame` 目标后，在引擎仓库根目录执行：

```powershell
xmake lua scripts/verify-sdk.lua
xmake lua scripts/verify-sdk.lua --graphics
```

脚本将 SDK 安装到 `build/sdk-validation/` 下的新临时目录，再搬到含空格的新路径；将独立消费项目复制到
另一子目录，编译并检查头文件、物理/固定 tick、输入、TTS 参数拒绝和存档。不引用引擎源码
或示例资源。脚本还验证模式、架构及 Windows C 运行库不匹配会被明确拒绝。
`--graphics` 额外创建真实隐藏窗口、程序生成纹理并绘制四帧，检查退出清理，
需要 OpenGL 环境。Windows/Linux CI 使用无窗口模式。

日志及 `report.lua` 位于 `build/sdk-validation/<平台>/<架构>/<模式>`。
临时 SDK 和消费项目保留以便诊断，其路径写入报告。报告只在全部检查通过后
写入；验证只覆盖所选配置，不代表干净机器发行、离线依赖或完整游戏验收。

恢复本仓库的示例/测试目标：

```powershell
xmake f --build_examples=y --build_tests=y --example_console=n -y
xmake build -a -j 1 -y
```

桌面引擎库按平台/架构/模式输出到 `build/lib/<平台>/<架构>/<模式>`，防止
Debug/Release 库相互覆盖。桌面 EXE、PDB、ILK、测试程序都在
`build/desktop/<平台>/<架构>/<模式>` 中生成，Forge 使用其下的 `forge_runner/`。
构建完成后只把游戏 EXE、运行资源和说明复制到 `bin/`（Forge 在 `bin/forge_runner/`）。
`xmake run` 使用对应模式的 build 目标与资源；切换模式后再次构建会更新 bin 发布副本，
不再依赖旁置 `.raygame-config` 标记或强制重新链接。

SDK 导出 `raygame/TextToSpeech.h`，Windows 的 SAPI/COM 系统库由接入脚本传递。
语音用法与 Android Java 打包要求见 [TTS](TEXT_TO_SPEECH.md)；当前安装 SDK 入口
仍面向桌面，Android APK 使用根工程规则。
