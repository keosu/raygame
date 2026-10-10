# 示例与参考游戏

每个示例的源码与专用资源放在自己的目录，便于定位、修改和打包。

| 目录 | 内容 | 构建入口 |
| --- | --- | --- |
| `space_shooter/` | 飞船射击 | 根目录 `xmake build space_shooter` |
| `bouncing_balls/` | 物理球场 | 根目录 `xmake build bouncing_balls` |
| `neon_breakout/` | 霓虹打砖块 | 根目录 `xmake build neon_breakout` |
| `bullet_hell/` | 弹幕射击 | 根目录 `xmake build bullet_hell` |
| [forge_runner/](forge_runner/README.md) | FORGE 三关横版平台参考游戏，中英文界面 | 根目录 `pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify` |
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

根目录 `xmake` 默认构建引擎和四个小示例，桌面 EXE 保持 `bin/<name>.exe`。
共用字体复制到 `bin/assets/fonts/`，专用资源复制到 `bin/assets/<name>/`。
Android 打包在 build 目录按游戏合并共用与专用资源，不把其他游戏资源放进 APK。
运行方法和设备验收边界见 [Android 文档](../docs/ANDROID.md)。

FORGE 是完整的独立 SDK 消费工程，使用自己的 `xmake.lua`、测试、资源和许可。
它只引用安装后的 SDK，可以整体复制到仓库外构建；放在 examples 中方便发现。
它的输出为 `examples/forge_runner/build/windows/x64/<mode>/forge_runner.exe`，
运行时保留相邻的 `assets/`。SDK 接入见 [SDK 文档](../docs/SDK.md)。
