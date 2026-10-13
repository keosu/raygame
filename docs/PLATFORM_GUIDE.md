# 跨平台开发：边界与实用工具

Windows/MSVC 是本机验证入口，Linux 由 CI 构建，Android 使用 NDK/NativeActivity。
“可以编译”不等于设备上的触控、生命周期、音频和显示都通过验收。

## 需要明确区分的行为

| 边界 | 当前做法 / 应使用的入口 | 仍需考虑 |
| --- | --- | --- |
| 只读资源与可写数据 | APK 资源用 `FileSystem::ReadAssetText` / `AssetExists`；存档用 `UserDataDirectory` + `WriteAtomic` / `SaveSystem` | APK assets 不是普通目录，不能用 `ifstream` 或 `filesystem::exists` 读取；不要向安装目录写配置 |
| 路径与编码 | 源文件、文本资源 UTF-8；桌面路径用 `filesystem::path` | Linux/Android 区分大小写；文件名大小写、工作目录、中文用户目录需要验证。不要把盘符、反斜杠或开发机绝对路径写进资源 ID |
| 画布与屏幕 | `CanvasViewport` 统一缩放/坐标映射；`CanvasLayout` 放置边缘按钮和居中菜单 | `Fit` 会留边，`Expand` 增加可见区域；物理尺寸不应跟随窗口拉伸。刘海/系统手势区域不等于画布边界 |
| 触摸与鼠标 | `TouchTracker` 跟踪 ID、起点、按下、释放、取消；Forge 保留自己的动作缓冲 | 不要假设触点数组第 0 项始终是同一根手指；不要把 Android 合成鼠标与触摸同时用于同一操作；释放时确认起点/终点属于同一按钮 |
| 帧输入与固定更新 | 每渲染帧采样一次触摸；跳跃/冲刺按下缓冲到固定 tick，仅消费一次 | 同一帧可能零次或多次物理更新。短按不能丢失，按下不能被每个 tick 重复触发；持续按住和按下事件是两种状态 |
| 失焦、暂停与退出 | Forge 切页/失焦清输入、保存并暂停；Bullet Hell 暂停时取消手势 | Android 可直接回收进程，不能只在析构时保存；锁屏、后台恢复、音频恢复、GPU 上下文重建仍需实机验证 |
| 图形与设备能力 | Android GLES 2.0、静态 C++ 运行库、16 KiB 对齐已有构建检查 | 桌面 GLSL 不一定可用于 GLES；纹理大小、字体图集、DPI、低内存、帧率与耗电要在目标设备测量 |
| 原生功能与发布 | Windows 标题栏/图标走桌面分支；APK 图标来自 manifest/res；日志写私有数据目录 | 系统 Back、软键盘/IME、剪贴板、震动、权限和签名属于平台功能，按需求单独接入；签名一致才能覆盖安装 |

## 资源与配置文件

```cpp
std::string level;
// Android：相对于 APK assets 根目录。桌面：传入解析后的资源文件路径。
if (!RayGame::FileSystem::ReadAssetText(assetRoot + "/levels/01.level", level)) {
    // 报告缺失/过大/读取失败；失败时 level 保持原值。
}

auto settings = RayGame::FileSystem::UserDataDirectory("my.company.game") / "settings.txt";
RayGame::FileSystem::WriteAtomic(settings, serializedSettings);
```

`ReadAssetText` 的资源根仍由游戏确定：FORGE 在 Android 使用 `forge`，桌面使用
可执行文件旁的 `assets`。它没有把一个 APK 虚拟路径伪装成可写文件路径。

## 触摸工具

`TouchTracker` 是每个输入上下文持有的实例，不使用全局触摸角色。每渲染帧调用
`Update(Input::GetFrame(), Display::GetViewport())`，然后读取：

- `Contacts()`：最多八个稳定槽位，`id`、画布坐标 `start/position` 和本帧 `pressed`。
  只处理 `Active()` 的触点；取消或从画布外开始的触点会阻塞到抬起。
- `Released()`：本帧抬起的触点及最后位置。取消、失焦、映射变化不会伪装成点击。
- `IsActive(id)`：捕获某根手指的控件，用它检查捕获是否仍有效。
- `Cancelled()`：失焦/恢复焦点或画布映射变化，本帧应清除控件状态与待消费动作。
- `Reset(frame)`：切页、暂停、打开模态框时主动调用。现有手指必须抬起才能再次操作。

同一控件的角色由游戏按 **按下起点** 分配，不随触点排序或滑到其他按钮改变。
持续拖动需要按 ID 找触点；不要每帧重新指定第一根手指。Forge 的 `TouchControls`
演示多动作与固定 tick 缓冲；Bullet Hell 演示移动手指 + 独立 FOCUS 手指。
通用 `UI::Button` 尚未整体迁移到多指路由，其他小示例仍有合成鼠标路径。

## 自适应布局工具

```cpp
RayGame::CanvasLayout layout{
    {1280, 720},
    {0, 0, RayGame::Display::GetWidth(), RayGame::Display::GetHeight()}
};
Rectangle jump = layout.Place(Rectangle{1112, 516, 112, 120}, {1, 1});
Vector2 menuOffset = layout.Offset({0.5f, 0.5f});
```

锚点 `{0,0}` 为左上，`{1,1}` 为右下，`{0.5,0.5}` 为居中。工具只平移设计坐标，
不会拉伸按钮。世界相机显示整个 Expand 画布，菜单使用居中偏移；命中检测必须
使用与绘制相同的偏移。`content` 也能接收已换算成画布坐标的安全内容矩形。
**目前没有自动查询 Android WindowInsets/刘海区域**，本工具不声称解决系统级安全区。

## C++ 格式

紧凑源码不是 C++ 的必要格式，也不会因此更快。先前 FORGE 有多语句合行及大量
单行函数，现在已按 `.clang-format` 展开：四空格、100 列、附着式花括号，控制流
和非空函数体换行；`.editorconfig` 固定 UTF-8/LF。

安装 `clang-format 19.1.5`，在仓库根目录运行：

```sh
python -m pip install clang-format==19.1.5
python scripts/format-code.py
python scripts/format-code.py --check
```

也可以用 `--clang-format /path/to/clang-format` 指向已有的 LLVM/Visual Studio
工具。默认范围是 FORGE 的源码/测试、两个本次更新的小示例以及新增工具/测试，
CI 检查同一范围。其他历史文件逐步迁移；显式传入路径可格式化更多文件，例如
`python scripts/format-code.py include/FileSystem.h`。编辑器的 Format Document 也会
读取根目录配置。格式调整不改变接口、存档或关卡数据。

## 拖拽、按钮绘制与画质

`PointerGesture` 统一鼠标与单个触屏指针捕获。每渲染帧调用
`Update(Input::GetFrame(), Display::GetViewport())`，读取画布坐标 `start/position`、
`pressed/held/released/dragging/cancelled` 和 `IsTouch()`；移动超过 dragThreshold
（默认 8 个画布单位）开始拖动。多指不提升已有第二根手指；失焦/映射变化/非法
位置取消且不生成 release。切页调用 Reset(frame)，已有手指必须抬起再按。
释放边沿仅保留一帧，固定 tick 命令仍需缓存。花园示例演示种子拖放、预览及取消。

`ButtonPainter::Draw(rect, state, style)` 只绘制表面（阴影、边缘、高光、按压/选中/
禁用），不读取输入；`Icon` 提供箭头、跳跃、冲刺、暂停等矢量图标。
UI::Button、campaign 菜单和 Forge 使用同一绘制接口，各自保留输入路由。
`UI::unicodeFont/unicodeHeadingFont` 是含所需 Unicode 字形的可选字体句柄；
测量和绘制一致，Shutdown 清空。框架不会自动翻译或替使用者补齐字体字形。

`Display::SetQuality(CanvasQuality::Smooth)` 在原生像素密度之上使用两倍采样，
然后双线性缩放；Standard 为原有密度。纹理边长最多 4096，失败保留旧画布。
新 campaign 可在设置切换，Forge 使用 Smooth；这不是自动硬件性能分档，也不
代替透明边缘素材与足够分段的几何。高画质的设备显存/帧时间仍需真机验证。

自带菜单的游戏调用 `SetDefaultShortcutsEnabled(false)` 后，`SetPaused` 只暂停
模拟，不叠加引擎默认暂停 UI。派生类的图形检查可以调用受保护的 `Render()`，
使用与正常 `Run()` 相同的场景、菜单和引擎叠层绘制流程。
