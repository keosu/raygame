# FORGE / The last delivery

独立消费 RayGame SDK 的工业科幻横版平台参考游戏。你是一名携带最后火种的
信使，要穿过铸造厂、能源通道和反应炉核心，收集全部能源电池并重启机器。
三关有递进的平台、移动升降台、周期激光和巡逻无人机；每关出口需要全部电池。

玩法与完整流程：

- 移动加减速、跳跃宽限和输入缓冲、按住/轻点的可变跳高、空中二段跳、
  墙跳/滑墙、冲刺与冷却；单向平台支持下落，升降台会带着角色移动。
- 主菜单、新游戏确认、继续游戏、暂停、检查点重试、逐关结算和最终通关。
  关卡进度、已收集电池、设置和最佳时间保存到用户目录；保存失败给出提示。
- 三层视差工业场景、关节角色动画、围巾、灯光、粒子、落地反馈和可关闭的
  镜头震动；原创合成音效及 32 秒工业氛围循环配乐。
- 键盘、鼠标和手柄可操作主流程；固定 1280×720 画布等比适配窗口与全屏。
  GUI EXE 不弹控制台，标题栏继承引擎的 Windows 原生主题，诊断保留到文件。

## Windows 构建与运行

在 RayGame 仓库根目录使用 PowerShell 7、Visual Studio 2022 和 xmake 3.1.1：

```powershell
pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify
./games/forge_runner/build/windows/x64/release/forge_runner.exe
```

脚本在独立配置中构建/安装 SDK，然后构建游戏，不改 IDE 当前桌面配置。
Debug 使用 `-Mode debug`。`-Verify` 运行逻辑测试和隐藏窗口图形 smoke，
日志和截图位于 `build/forge/<mode>/`。游戏运行需要输出目录中的 `assets/`。
MSVC Release 使用动态 C 运行库，目标 Windows 机器需要 Visual C++ 2015–2022
x64 Redistributable。CI 下载包保留 EXE 与 assets 的相邻布局。
没有可用 OpenGL 显示环境时可加 `-SkipGraphics`；Windows CI 使用此选项，仅运行
构建、逻辑和内嵌图标检查，不把图形 smoke 标为通过。

也可以把整个 `games/forge_runner` 复制到另一个目录，使用已安装的 SDK：

```powershell
xmake f -p windows -a x64 --toolchain=msvc -m release --raygame_sdk=C:/SDK/raygame/windows-x64-release -y
xmake build -y
xmake run forge_runner
```

游戏构建只引用安装后的 `<raygame/...>` 头文件和库，不引用引擎源码、
examples 或 tests。Linux 分支保留，当前本地验收为 Windows/MSVC；Android
移植、触控、Linux 实机运行和发行包签名不在这一版本的验收范围。

## 操作

| 动作 | 键盘/鼠标 | 手柄 |
| --- | --- | --- |
| 移动 | A/D 或左右箭头 | 左摇杆 |
| 跳跃/二段跳 | Space / W，按住跳得更高 | A / 下方按钮 |
| 冲刺 | Shift / X | X / 左侧按钮 |
| 穿过单向平台 | S/下 + 跳跃 | 摇杆向下 + A |
| 暂停/返回 | Escape / P | Start |
| 重试检查点 | R，或暂停菜单 | 暂停菜单 |
| 全屏 | F11，或设置菜单 | 设置菜单 |
| 菜单 | 上下、Enter，或鼠标点击 | 方向键、A |
| 调整设置 | 左右，或点击设置行 | 方向键左右 |
| 制作信息 | 主菜单 C | — |

正常用户存档与日志位于 `%LOCALAPPDATA%/raygame.forge/`。
`settings.sav` 与 `campaign.sav` 使用引擎版本化校验和/原子写入与备份恢复。
关卡途中保存后继续会从最近检查点恢复，保留已收集电池；过关后继续进入下一关。
最终通关会保留关卡纪录，并可以开始新一轮。默认用户存档不会被验收脚本改动。

## 数据与资源

`assets/levels/*.level` 是有版本、字段范围和数量检查的纯文本关卡。
`solid`、`ledge`、`moving`、`hazard`、`laser`、`drone`、`cell`、`checkpoint`
和 `sign` 定义内容；可以修改平台位置/移动周期和电池分布，无需改引擎代码。
当前 campaign 固定为三关，增加第四关需要扩展游戏的关卡索引与进度 schema。

`tools/generate-assets.py` 生成原创 ICO/PNG、灯光纹理、八种音效和配乐；
需要 Pillow，正常构建使用已提交文件。角色和场景由 `Presentation.cpp` 绘制，
便于直接调整动画、材质和布局。字体及各自的 SIL OFL 许可在 `assets/fonts`。
第三方说明见 `THIRD_PARTY_NOTICES.md`。

## 验收边界

逻辑测试包括三关通过正常移动/跳跃/冲刺命令收集全部电池并到达出口的路线
搜索，未使用传送或修改碰撞几何；还检查跳高、二段跳、冲刺、单向/移动平台、
危险穿透、检查点和进度恢复。图形 smoke 检查菜单/关卡/结算渲染、暂停、
存档、4:3/超宽屏坐标及退出资源释放，并注入键盘/鼠标/手柄快照走通设置、
暂停恢复、重试、继续游戏与覆盖存档确认。每次验证在隔离目录使用默认起点，
不操作正常用户存档。截图另作人工视觉检查。

这些检查不替代人工完整通关、真实手柄/多 DPI/多显示器、音效主观试听和
长时间运行验收。游戏目前采用英文 UI，尚未实现中文字体/本地化、按键重绑、
无障碍选项或商用发行渠道；本轮目标是完整可玩的参考游戏。
