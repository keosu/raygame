# Marble Temple / 彩球神庙

祖玛玩法的完整三关示例：鼠标瞄准/点击发射，触屏拖动瞄准/抬手发射，同色三连消除，
闭合缺口可形成连锁，清空有限球链后过关，球链抵达终点则失败。每九发获得一发范围爆破。
Space / 鼠标右键 / SWAP 交换当前和下一颗彩球。三关依次解锁，最高分自动保存。

```powershell
xmake run marble_temple
pwsh -NoProfile -File scripts/build-android.ps1 -Game marble_temple
```

Windows 输出 `bin/marble_temple.exe`，保留同级 assets；Android APK 在
`bin/android/arm64-v8a/release/`。`P` / Back 暂停，`R` 重试，`F11` 全屏。
菜单提供说明、选关、声音开关；失焦自动暂停。三关轨道、配色、速度和彩球数量不同。
进度/最高分/音效开关保存在 `FileSystem::UserDataDirectory("raygame.marble_temple")`。

`Model.h` 为无窗口规则，`Game.h` 为 Scene/Component、绘制和游戏交互，
`../common/CampaignGame.h` 为共享菜单、触屏/存档流程。美术/图标/音效为原创，
采用相似玩法，未使用祖玛的原始素材。共用字体许可随 assets/fonts 分发。

逻辑检查：`xmake run campaign_tests`。图形/输入检查使用隔离数据：
`xmake run marble_temple --smoke --data build/marble-smoke`，结果在 smoke.log。
Android 构建校验不等于真机触感/音效验收。
