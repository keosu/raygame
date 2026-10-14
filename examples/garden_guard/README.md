# Garden Guard / 花园守卫

植物大战僵尸玩法的完整三关示例：收集阳光、选择植物、在五行九列草坪布防，抵御三波敌人。
六种植物为向日葵、豌豆射手、坚果墙、冰花、樱桃炸弹、双发射手，另有铲子；
敌人分普通、装甲和快速型。每行一次割草机救场，再次被突破则失败，清空全部波次后胜利。

```powershell
xmake run garden_guard
xmake android --game=garden_guard
```

选择上方种子卡或键盘 1–6，点击/触摸空格种植；点击阳光收集。卡片需要阳光和冷却，
7 选择铲子，点击已种格子移除。先种向日葵，尽早覆盖五行；装甲敌人用双发或炸弹，
快速敌人用冰花/坚果。`P` / Back 暂停，`R` 重试，`F11` 全屏；失焦自动暂停。
关卡解锁、最高分和音效开关自动保存在
`FileSystem::UserDataDirectory("raygame.garden_guard")`。

Windows 输出 `bin/garden_guard.exe`，保留同级 assets；Android APK 位于
`bin/android/arm64-v8a/release/`。菜单提供说明、选关、音效开关。

`Model.h` 为无窗口规则，`Game.h` 为 Scene/Component 和原创代码美术，
`../common/CampaignGame.h` 管理共用菜单/触屏/存档。采用相似玩法，图标、角色、
场景和音效重新制作，未使用植物大战僵尸原始素材。共用字体许可随 assets/fonts 分发。

逻辑检查：`xmake run campaign_tests`。图形/输入检查：
`xmake run garden_guard --smoke --data build/garden-smoke`，结果在 smoke.log。
Android 构建校验不代替真实设备操作/后台/音频验收。

菜单和游戏界面支持简体中文 / English（默认中文），设置页可即时切换语言、
音效开关/音量、精细/标准画质、60/120 FPS，以及桌面全屏/窗口；退出重启仍保留。
菜单按可用区域等比缩放，支持鼠标、触屏以及方向键/Tab + Enter 导航。
精细画质使用框架 CanvasQuality::Smooth 超采样，受 4096 像素纹理上限约束；
低性能设备可选择标准画质。游戏画布保持等比，宽屏/竖屏额外区域不拉伸玩法几何。
中文字体为 Noto Sans SC 子集，许可和重建说明在 assets/fonts/。
启动显示主菜单，详细操作放在“玩法说明”页；种植失败提示只短暂显示在草坪下方。

可直接将种子卡拖到草坪，松手时种植；植物跟随指针，触屏预览向上偏移以避开手指。
绿色格子表示可种植，红色表示占用/阳光不足/冷却中；拖到草坪外取消且不扣费。
铲子也可拖动移除植物。点击选择再点击种植仍支持；失焦/暂停/窗口变化取消拖拽。
拖拽使用框架 PointerGesture，捕获稳定手指 ID，抬起只执行一次，其他手指不接管。
