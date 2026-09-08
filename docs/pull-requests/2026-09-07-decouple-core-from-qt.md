# PR：把 `core/` 与 Qt 解耦

- **日期：** 2026-09-07
- **分支：** `refactor/decouple-core-from-qt` → `main`
- **基线提交：** `9a30f88`
- **关联记录：** [工作记录](../work-logs/2026-09-07-decouple-core-from-qt.md)

## 关联

`CLAUDE.md` 写明分层是 **QML → `ui/backend/` → `core/` → LiveKit SDK**，但实际上 Qt 已经渗透到最底层：
`core/` 里有 9 个 `QObject` 子类、约 60 个信号，`QString`/`QMap`/`QList` 出现在值类型里，
`QImage` 是视频通路的载体，`QAudioSink`/`QAudioSource`/`QCamera` 就是设备后端本身，
`QNetworkAccessManager` 做 REST，`QTimer`/`QtConcurrent` 做调度，
`screen_capturer.cpp` 里甚至直接调 `QGuiApplication::primaryScreen()`，
`DeviceController` 直接读写 `QSettings`。

目标：`core/` 只用原生 C++ 实现，不依赖任何 Qt 的类、方法、变量，且不影响功能。

## 改了什么

### 架构：端口与适配器

`core/` 只定义纯 C++20 接口、POD 值类型和回调；真正「由 Qt 充当后端实现」的部分
搬到 `ui/adapters/qt/` 去实现这些接口。**运行时跑的仍然是同一份 Qt 代码**，所以行为不变。
这是在延续仓库里已有的做法（`core/image_types.h` + `ui/adapters/qt/qt_capture_adapter.h`）。

新增的端口：`TaskRunner`、`BackgroundExecutor`、`Timer`/`TimerFactory`、`LogSink`、
`VideoInput`、`AudioInput`、`AudioPlayer(+Factory)`、`MediaDeviceRegistry`。
`QtPlatformServices` 持有全部 Qt 实现，向 core 交出一份 `PlatformServices`（非拥有指针）。

**刻意只把设备管道搬走**：WebRTC APM、10ms 分帧、重采样、推送给 LiveKit 的逻辑全部留在 core。

### 约束由编译器保证

新增 `links_core_base` 与 `links_core` 两个静态库，**绝不链接任何 `Qt6::*`**，且 `AUTOMOC OFF`。
Qt 头文件只能通过 `Qt6::*` 的 usage requirements 进入搜索路径，因此 `core/` 里写
`#include <QString>` 是硬编译错误，写 `Q_OBJECT` 是链接错误。CI 另加一道 grep 检查
（Qt 头、Qt 关键字宏、对 `utils/` 的依赖）。

拆成两个库是为了让原本不依赖 LiveKit 的测试目标（桌面捕获、录制）不要凭空多出 LiveKit 运行期依赖。

### 用户可观察到的变化

正常情况下**没有**。这是一次纯结构重构，不改任何功能。以下几处是实现层面的等价替换，
但属于「改错了会被用户感知」的地方，列出来便于评审重点关注：

- 视频帧从 `QImage` 换成 `core::VideoFrame`（`shared_ptr<const RawImage>`），保持
  `QImage` 隐式共享那样的 O(1) 扇出。屏幕共享路径反而**少了一次整帧拷贝**。
- 远端音频播放改为经 `AudioPlayer` 端口，Qt 侧仍是 `QAudioSink`，
  且刻意不设置 buffer size，保持 Qt 默认值。
- AEC 反向参考信号的位置与内容一字未改。
- 所有定时器仍然跑在 Qt 事件循环上，周期不变（屏幕共享去抖 500ms、待发布重试 500ms、
  网络统计 1s、订阅后 100ms）。
- `network_client` 从 `core/` 搬到 `ui/backend/`（它本来就只被 UI 层使用），
  一步去掉了 core 对 `Qt6::Network` 的依赖。
- 删除死代码 `core/media_manager.*` 与转发壳 `core/conference_manager.h`。

### 顺带修掉的两个既有缺陷

- `DeviceController::connectScreenSignals()` 里的 `static QMetaObject::Connection` 是函数级
  static，第二个实例会断开第一个实例的处理器。改为每实例持有。
- `MediaPipeline` 用裸 `new std::atomic<bool>` 管理 stop flag，`stopTrack` 路径上泄漏。
  改为 `shared_ptr`。

### 测试

所有测试目标不再链接 Qt。`test_microphone_capturer.cpp` 改写为针对假的
`core::AudioInput` 编写——它现在不需要 `QCoreApplication`、不需要真实麦克风、
也不会再因为没有硬件而 skip，并且第一次可以确定性地测到分帧行为。
`audio_processing_tests` 去掉了 `AUDIO_PROCESSING_TESTS` 宏（那 12 处 `#ifndef` 只包住
日志调用，从未 stub 掉 APM 本身）。

## 怎么验证

- [ ] `build.cmd release`
- [ ] `build.cmd tests`
- [ ] 登录 / 入会 / 离会 / 重新入会
- [ ] 摄像头开关、通话中切换摄像头
- [ ] 麦克风开关、通话中切换麦克风
- [ ] 屏幕共享（整屏 / 指定窗口），确认选中的是正确的显示器
- [ ] 快速反复切换屏幕共享 10 次（验证 500ms 去抖）
- [ ] **用扬声器（不是耳机）做 3 人 10 分钟通话**，听回声与爆音
- [ ] 中文昵称 + 含 emoji 的聊天消息，与未改动的旧版本互通
- [ ] 网络指标面板持续更新
- [ ] Windows 本地录制

**以上全部未执行，且当前分支还编译不过。**

改动本身是在没有 C++ 工具链的 WSL 环境中完成的（没有 cmake、没有编译器、没有 Qt6，
仓库里的 `build/` 是在别处产出的 Windows 产物）。作者随后在 Windows 上跑了第一次
`build.cmd release`：CMake 配置成功（`links_core_base` / `links_core` 两个目标正确生成，
`nlohmann_json` 正确解析），但编译在 **23/221** 处中断，报出 4 个错误，已在
`67cbb06` 修复：

| 错误 | 原因 |
| --- | --- |
| `win::enumerateMonitors` 不是 `links::core::win` 的成员 | 该函数在 `links::desktop_capture::win`，而调用点位于 `namespace links::core` 内，裸写 `win::` 解析到了另一个同名且真实存在的命名空间 |
| `std::function` 目标不可拷贝构造 | `livekit::VideoFrame` 的拷贝构造被 `= delete`，因此 `VideoFrameEvent` 是 move-only，捕获它的 lambda 也是 move-only，无法放进 `std::function`。已改为在 `postGuarded` 内部用 `shared_ptr` 持有可调用对象——这条影响所有 move-only 负载，不止这一处 |
| `std::set::reserve` 不存在 | 从 `QSet` 沿用下来 |
| 3 处 `.isEmpty()` | 这些字段已经是 `std::string` |

**剩余 198 个编译单元、`ui/`、适配器层与全部测试尚未经过编译器检验，预期还有更多错误。**
合并前需要在 Windows 上反复构建修复至通过，再执行上面的手工联调清单。

已执行的只有静态检查：core 无 Qt 依赖、每个 `notify()` 的信号都有声明、
每个定义的成员函数都有声明、CMake 清单与磁盘一致、搬迁文件的 include 可解析、
以及逐条比对脚本化日志改写前后的格式化占位符（据此发现并修复了 6 处参数丢失）。

macOS / Linux 的 CMake 分支与 `enumerateMonitors()` 的非 Windows 实现完全未验证，
CI 的三平台矩阵是第一个真正的检验点。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（仓库中当前不存在该文件）
- [x] UI / QML 改动附了截图（不适用：本次未改动任何 QML）
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明（`vcpkg.json` 新增 header-only 的 `nlohmann-json`）
- [x] AI 使用披露：是。全部改动由 Claude Code 完成，包括接口设计、代码改写、
      CMake 与测试调整、CI 检查与文档更新。**代码未经任何编译验证**，已在上文如实说明。
