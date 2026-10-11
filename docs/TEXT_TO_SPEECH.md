# 系统文字转语音

游戏只调用 C++ API。Windows 使用 SAPI，Android 使用引擎附带的 Java/JNI 桥接，
无需为每个游戏创建 Java 模块。Linux/macOS 暂返回 `Unavailable`。

```cpp
#include "RayGame.h"
using namespace RayGame;

SpeechOptions options;
options.language = "zh-CN"; // 留空使用系统默认语言/声音
options.rate = 1.0f;       // 0.5–2.0
options.volume = 0.8f;     // 0–1，独立于 AudioManager
if (!TextToSpeech::Speak(u8"欢迎来到游戏", options)) {
    Diagnostics::Write(LogLevel::Warning, TextToSpeech::GetLastError());
}
// 用户跳过旁白时：
TextToSpeech::Stop();
```

- `Speak` 懒初始化并替换当前朗读，返回 true 表示请求已接受。Android 首次初始化异步，
  期间只保留最后一个请求；异步失败通过 `GetLastError()` 查询。
- `GetState()` 返回最近一次 Update 的状态：Uninitialized、Initializing、Ready、
  Speaking 或 Unavailable。`IsSupported()` 仅表示编译平台有后端，不保证设备安装了声音。
- 文本必须为合法 UTF-8，非空、无内嵌 NUL，最多 2000 个 UTF-16 单元；补充平面字符
  占两个单元。文本按普通文字朗读，Windows 禁用 XML/SSML 解析。参数错误返回 false。
- 从同一个游戏运行线程调用全部 API。`GameEngine` 每帧轮询，失焦、暂停、切场景时停止，
  Shutdown 时释放服务。单独使用 API 时自行周期调用 `Update()`，结束调用 `Shutdown()`。
- `Stop()` 清除待播请求；失败后的错误保留，成功新请求或 Shutdown 清除。
  初始化不可用后，调用 Shutdown 再 Initialize 可重新尝试。

Windows 依赖系统自带的 `sapi`/`ole32`，xmake 和安装后的桌面 SDK 自动传递链接库。
语速映射到 SAPI 的速率刻度，不保证各平台相同倍数有相同实际时长。指定语言按 SAPI
注册的声音匹配；Windows 设置中可见的部分 OneCore 声音未注册到 SAPI，可能不可用。

Android 继续使用 NativeActivity。`platform/android/java/org/raygame/SpeechBridge.java`
在主线程创建 TextToSpeech、设置语言和发起朗读；JNI 轮询状态，不从 Java 回调原生对象。
暂停停止、销毁释放，旧请求回调以编号过滤。默认系统 TTS 服务是否需要联网由其声音决定。
应用不下载语音包，不需要录音权限。缺失服务或语言数据时显示可读错误。

六个 APK 已统一接入 `raygame.android_tts`：在 xmake **3.1.1** 的 NativeActivity
打包后，用 JDK 17 javac 编译桥接、D8 生成 classes.dex，再进行 16 KiB ZIP 对齐和签名。
不需要 Gradle。该规则依赖 xmake 3.1.1 的 `packing/res_only.apk` 中间产物；升级 xmake
需重新验证打包顺序与增量重建。构建脚本检查 dex 内桥接类以及 manifest 的 hasCode 和
TTS_SERVICE 查询声明。自己建立 Android 目标也应添加该规则和两项 manifest 声明，
参照 `examples/engine_lab/android/AndroidManifest.xml`；仅链接静态库不能打包 Java 类。

用 [Engine Lab](../examples/engine_lab/README.md) 的 Services 页试听。默认逻辑测试只做
输入/错误契约检查，不访问设备；Windows `tts_smoke` 另做静音合成、替换、停止、恢复测试。
Android 运行、真实声音质量、系统无引擎/缺语言和后台恢复仍需设备验收。
