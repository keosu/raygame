# Engine Lab / 对象实验室

可操作的引擎 API 示例。每一页创建真实的 Scene、GameObject 和组件，底部按钮改变其行为，
计数器显示实际运行状态。界面使用英文，TTS 可试听中文和英文。

```powershell
xmake build engine_lab
xmake run engine_lab
# 仅构建 Android APK，不安装设备：
pwsh -NoProfile -File scripts/build-android.ps1 -Game engine_lab
```

Windows 入口 `bin/engine_lab.exe`，保留同级 `assets/`；Android 入口
`bin/android/arm64-v8a/release/engine_lab.apk`。Windows 无控制台，窗口跟随系统主题；
使用等比 Expand 画布，缩放/宽屏不拉伸对象。Android 横屏，可反向横屏。

| 页面 | 对象与 API | 可尝试的操作 |
| --- | --- | --- |
| Objects | Scene、GameObject、Transform、Component 生命周期及插值 | 停止旋转、创建/销毁、缩放父对象，观察子对象 |
| Physics | PhysicsWorld、Rigidbody、Box/CircleCollider、Camera2DComponent | 施加冲量、生成刚体、相机跟随，观察接触/触发计数 |
| Sprites | SpriteRenderer、SpriteAnimation、ResourceManager、TextureHandle | 替换共享纹理、暂停动画、翻转精灵，观察资源 revision |
| Effects | ParticleEmitter、TimerComponent、DestroyAfter、EventBus、TweenManager、ObjectPool | 手动触发事件/粒子、暂停定时器、更换发射形状 |
| Tilemap | Tilemap、相机坐标换算、solidTiles | 点击/触摸画格子、擦除、切换 ID 1 的实体查询标记 |
| Services | TextToSpeech、AudioManager、SaveSystem、FileSystem、Input、TouchTracker | 中英文朗读、停止、语速/音量、保存/加载、播放音效 |

键盘 `1–6` 切页，`R` 重建当前场景，`F11` 全屏，`Esc` / Android Back 退出。
鼠标或触摸点击左侧页面与底部按钮。切页会取消旧场景的事件、补间和语音。
Tilemap 的 solidity 是查询属性，不能自动代替刚体碰撞体。

Services 的语速/音量供下一次朗读使用。中文按钮要求系统安装可用中文声音；
缺失时状态栏显示错误。Windows 使用 SAPI 声音，Android 使用系统默认 TTS 服务，
游戏代码无需编写 Java。系统 TTS 音量独立于游戏音效音量；完整约定见仓库
`docs/TEXT_TO_SPEECH.md`。

存档和日志使用 `FileSystem::UserDataDirectory("raygame.engine_lab")`，Windows 默认为
`%LOCALAPPDATA%/raygame.engine_lab/`，Android 位于应用私有 files 目录；也可用
`--data <目录>` 显式指定。存档仅保存语速和音量，点击 Load settings 恢复。

图形检查会注入真实按钮/触摸输入，检查对象生命周期、碰撞、资源替换、跨场景取消、
瓦片坐标和存档，并导出页面与不同比例截图；不会自动朗读或播放音效：

```powershell
xmake run engine_lab --smoke --data build/engine-lab-smoke
# 检查 build/engine-lab-smoke/smoke.log，进程失败返回非零。
xmake build tts_smoke
xmake run tts_smoke
```

`tts_smoke` 是独立的 Windows 系统合成检查，使用零音量，需要已安装声音和音频端点。
它不代替人工试听；Android APK 构建不代替设备上的朗读、后台停止和生命周期验收。

阅读顺序：`main.cpp` 的 UI/服务 → `LabScene.h` 的场景和自定义组件 → `include/` API。
图标、纹理、短音效为本例原创；`artwork/generate-sheet.py` 使用 Pillow 重建纹理/音效，
图标用仓库 `scripts/generate-icons.py` 重建。共用字体许可随 `assets/fonts/` 分发。
