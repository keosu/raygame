# Garden Guard / 花园守卫

植物大战僵尸玩法的完整三关示例：收集阳光、选择植物、在五行九列草坪布防，抵御三波敌人。
六种植物为向日葵、豌豆射手、坚果墙、冰花、樱桃炸弹、双发射手，另有铲子；
敌人分普通、装甲和快速型。每行一次割草机救场，再次被突破则失败，清空全部波次后胜利。

```powershell
xmake run garden_guard
pwsh -NoProfile -File scripts/build-android.ps1 -Game garden_guard
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
