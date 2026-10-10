# 示例与参考游戏

每个示例的源码与专用资源放在自己的目录，便于定位、修改和打包。

| 目录 | 内容 | 构建入口 |
| --- | --- | --- |
| `space_shooter/` | 飞船射击 | 根目录 `xmake build space_shooter` |
| `bouncing_balls/` | 物理球场 | 根目录 `xmake build bouncing_balls` |
| `neon_breakout/` | 霓虹打砖块 | 根目录 `xmake build neon_breakout` |
| `bullet_hell/` | 弹幕射击 | 根目录 `xmake build bullet_hell` |
| [forge_runner/](forge_runner/README.md) | FORGE 三关横版平台参考游戏，中英文界面 | 根目录 `xmake build forge_runner` / `xmake run forge_runner` |
| `common/` | 四个小示例的 UI 辅助代码、共用字体及许可 | 随示例或图形 smoke 复制 |

四个小示例的布局一致：

```text
<name>/
  main.cpp
  assets/icon.png                   # 运行时资源
  artwork/icon.svg                  # 可编辑图标源
  resources/windows/<name>.ico/.rc   # EXE 内嵌资源
  android/AndroidManifest.xml
  android/res/                      # Android 启动器图标
```

共用内容仅放入 `common/`，专用关卡、图片、音频等放入示例自己的 `assets/`。
图标生成和 EXE 资源编译过程见 [图标说明](common/ICONS.md)。

根目录 `xmake` 默认构建引擎、四个小示例和 FORGE。四小示例桌面 EXE 保持 `bin/<name>.exe`。
共用字体复制到 `bin/assets/fonts/`，专用资源复制到 `bin/assets/<name>/`。
Android 打包在 build 目录按游戏合并共用与专用资源，不把其他游戏资源放进 APK。
运行方法和设备验收边界见 [Android 文档](../docs/ANDROID.md)。

FORGE 在根工程中链接当前引擎，输出为 `bin/forge_runner/forge_runner.exe`（Linux
无 `.exe` 后缀），相邻 `assets/`、README 和许可随构建复制，可整体打包。
`build_examples=n` 关闭全部五个桌面游戏；`build_tests=y` 仍可单独构建并运行
`forge_tests`，默认使用隔离的测试存档目录。Android 默认也包含 FORGE；单独构建
使用 `pwsh -NoProfile -File scripts/build-android.ps1 -Game forge_runner`，生成
`bin/android/arm64-v8a/release/forge_runner.apk`，触屏与验收边界见游戏说明。

FORGE 同时保留自己的独立 SDK 入口：Windows 可运行
`pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify`，输出仍为
`examples/forge_runner/build/windows/x64/<mode>/forge_runner.exe`。也可将整个
游戏目录复制到仓库外，使用已安装的 SDK 构建。两个入口复用 `targets.lua`，
源码、资源与测试保持一致；SDK 接入见 [SDK 文档](../docs/SDK.md)。
