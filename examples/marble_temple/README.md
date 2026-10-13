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

菜单和游戏界面支持简体中文 / English（默认中文），设置页可即时切换语言、
音效开关/音量、精细/标准画质、60/120 FPS，以及桌面全屏/窗口；退出重启仍保留。
菜单按可用区域等比缩放，支持鼠标、触屏以及方向键/Tab + Enter 导航。
精细画质使用框架 CanvasQuality::Smooth 超采样，受 4096 像素纹理上限约束；
低性能设备可选择标准画质。游戏画布保持等比，宽屏/竖屏额外区域不拉伸玩法几何。
中文字体为 Noto Sans SC 子集，许可和重建说明在 assets/fonts/。
启动显示主菜单，玩法与快捷键说明放在“玩法说明”页，游戏中仅显示状态与危险提醒。

彩球只按颜色区分，使用原创带高光/反射/柔和阴影和透明边缘的纹理；三关神庙背景
及发射器经过超采样生成，轨道绘制与模型使用同一组曲线。重建素材需要 Pillow：

```powershell
python examples/marble_temple/artwork/generate-marbles.py
python examples/marble_temple/artwork/generate-scenery.py
```
