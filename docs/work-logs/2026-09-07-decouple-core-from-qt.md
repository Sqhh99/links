# 工作记录：把 `core/` 与 Qt 解耦

- **日期：** 2026-09-07
- **分支：** `refactor/decouple-core-from-qt`（基于 `main` 的 `9a30f88`）
- **关联文档：** [PR 记录](../pull-requests/2026-09-07-decouple-core-from-qt.md)

## 一、用户的请求

> For the next requirement, I need to decouple the `core` code from Qt. The `core` should be
> implemented using only native C++ features; the decoupling must be achieved without affecting
> functionality, as I do not want the `core` to depend on any classes, methods, or variables
> provided by Qt.

要求：让 `core/` 只用原生 C++ 实现，不依赖任何 Qt 的类、方法、变量，并且不能影响现有功能。

动手前与用户确认了三个决定：

1. **架构方式：端口与适配器（ports & adapters）。** `core/` 只定义纯 C++20 接口、POD 值类型和回调；
   真正“由 Qt 充当后端实现”的部分（摄像头、音频输入输出、主线程派发、定时器）搬到
   `ui/adapters/qt/`，去实现 `core/` 定义的端口。运行时跑的仍是同一份 Qt 代码，所以行为不变。
   这是在延续仓库里已有的做法：`core/image_types.h` + `core/platform_window_ops.h`（无 Qt）
   由 `ui/adapters/qt/qt_capture_adapter.h` 提供 Qt 侧转换。
2. **约束方式：新建不链接 Qt 的静态库目标**（`links_core_base` + `links_core`，且 `AUTOMOC OFF`）。
   这样 `core/` 里混入 Qt 头文件会直接编译失败，混入 `Q_OBJECT` 会链接失败——靠编译器保证，而不是靠自觉。
3. **交付方式：分阶段、分多次提交**，每个阶段都要能编译、测试通过。

## 二、制定的计划

完整计划见会话中的方案文档，分七个阶段，顺序上刻意把风险最高的两块（事件/线程、设备后端）
放在机械性改动之后：

- **阶段 0（本次提交）**：白捡的收益——搬走 / 删掉根本不需要改造的文件。
- 阶段 1：新增无 Qt 的基础设施（日志门面、执行器/定时器端口、视频帧类型）与对应 Qt 适配器。
- 阶段 2：值类型与纯逻辑文件去 Qt（`QString` → `std::string` 等）。
- 阶段 3：视频帧从 `QImage` 换成 `VideoFramePtr`。
- 阶段 4：去掉 `QObject`，信号槽换成观察者接口，`Qt::QueuedConnection` 换成 `TaskRunner`。
- 阶段 5：抽出音视频设备端口，Qt Multimedia 实现搬到 `ui/adapters/qt/`。
- 阶段 6：CMake 拆分与 CI 防回归检查。

**阶段 0 之后调整了顺序**：把 CMake 拆分从最后一阶段提前为阶段 1，且只装入
「本来就没有 Qt」的文件。理由是这样只花约一个文件的工作量，就能先把
「编译器强制不许用 Qt」这道闸门立起来，后面每一个文件搬进这个目标时都自带验证；
放在最后则整个过程都没有强制约束。

### 阶段 0 的依据

先用三组检索确认了三件事，才决定“搬走 / 删掉”而不是“改造”：

| 文件 | 结论 | 依据 |
| --- | --- | --- |
| `core/network_client.{h,cpp}`（955 行，其中 379 行涉及 Qt） | 搬到 `ui/backend/` | 全仓库只有 `ui/backend/{Auth,Login,Conference}Backend` 引用它；`core/`、`utils/`、`main.cpp`、`tests/` 里 **零引用**。它本来就是 UI 层的 REST 客户端，放错了目录 |
| `core/media_manager.{h,cpp}` | 删除 | 空的 `QObject` 占位类，函数体只有 TODO 注释，全仓库从未实例化 |
| `core/conference_manager.h`（6 行） | 删除 | 只是 `#include "conference/conference_manager.h"` 的转发壳；唯一命中的 `core/conference/conference_manager.cpp:1` 是同目录同名头文件，按 C++ 引号包含规则优先解析到自己那份，不经过这个壳 |

搬走 `network_client` 一步就把 `Qt6::Network` 整块、以及 `QJsonArray`/`QUrlQuery`/`QNetworkReply`
等全部从 `core/` 拿掉了——**因此后续阶段不需要再设计 HTTP 端口**，这是计划里省掉的一大块工作。

## 三、具体改了哪些文件

| 文件 | 改动 | 对应问题 |
| --- | --- | --- |
| `core/network_client.{h,cpp}` → `ui/backend/network_client.{h,cpp}` | `git mv`，文件内容一字未改 | 它只被 UI 层使用，放在 `core/` 属于分层错误；搬走后 `core/` 不再需要 Qt Network |
| `ui/backend/{AuthBackend,ConferenceBackend,LoginBackend}.h` | `#include "../core/network_client.h"` → `#include "network_client.h"` | 头文件变成同目录 |
| `core/media_manager.{h,cpp}` | 删除 | 死代码 |
| `core/conference_manager.h` | 删除 | 无人使用的转发壳 |
| `CMakeLists.txt` | `SOURCES`/`HEADERS` 中删去上述三个文件；`network_client` 两项移到 “QML backends” 段；`ui/adapters/qt/qt_capture_adapter.{cpp,h}` 从 “Core modules” 段移出，单列 “Qt adapters” 段 | 保持列表与磁盘一致；适配器文件此前被错误地归在 “Core modules” 注释下 |

`ui/backend/network_client.cpp` 里的 `#include "../utils/logger.h"` 保持原样未动：
`ui/` 本身在 include 路径上（`target_include_directories` 里有 `${CMAKE_CURRENT_SOURCE_DIR}/ui`），
所以它解析为 `utils/logger.h`，而且这正是 `ui/backend/` 下其余 7 个文件一致的写法。

## 四、验证情况

**没有构建，没有跑测试。**

本次会话所在的 WSL 环境里没有任何 C++ 工具链——`cmake`、`ninja`、`make`、`g++`、`gcc`、
`clang++` 全部不存在，也没有 Qt6。仓库里的 `build/` 目录装的是在别处用 Windows 构建产出的
`.exe`，不能作为本次改动的验证依据。因此本阶段**一行代码都没有编译过**。

实际做了的是静态检查，四项全部通过：

1. `core/`、`tests/`、`main.cpp` 中对 `network_client` / `media_manager` / `MediaManager`
   的引用数为 0。
2. `CMakeLists.txt` 里列出的每个源文件都在磁盘上存在（无悬空条目）。
3. `core/`、`utils/`、`ui/` 下每个 `.cpp`/`.mm` 都被 `CMakeLists.txt` 或 `tests/CMakeLists.txt`
   列到（无遗漏条目）——这一条对应 `CLAUDE.md` 里点名的“最常见构建失败”。
4. `core/` 剩余的 Qt 头文件清点：`QNetworkAccessManager`、`QNetworkReply`、`QNetworkRequest`、
   `QUrlQuery`、`QJsonArray` 已全部消失；剩下的 `QJsonObject`/`QJsonDocument` 各 2 处，
   分别在 `participant_metadata_parser.cpp` 与 `conference_manager.cpp`，与计划一致。

**合并前必须由能跑 Windows 构建的人补跑：** `build.cmd release` 与 `build.cmd tests`。
按用户确认的协作方式，每个阶段结束后由用户在 Windows 上构建并回报错误，再进入下一阶段。

## 五、遗留事项

1. 阶段 1–6 尚未开始，仍是全部实质工作所在：`core/` 目前还有 9 个 `QObject` 子类、约 60 个信号、
   `QImage` 视频通路、Qt Multimedia 设备后端、`QTimer`/`QtConcurrent` 调度，以及
   `device_controller.cpp` 里 7 处直接读写 `QSettings`。
2. 本阶段未做任何行为改动，但**未经编译验证**，见第四节。
3. PR 记录（`docs/pull-requests/2026-09-07-decouple-core-from-qt.md`）等全部阶段完成后再写。
4. `conference_manager`、`device_controller`、`media_pipeline`、`room_event_delegate`、
   `participant_store`、`screen_capturer`、`camera_capturer` **目前零测试覆盖**，
   而它们正是后续阶段改动最大的文件。阶段 2 与阶段 4 是补单元测试的合适时机——
   端口抽出来之后，用假的 `TaskRunner` / `AudioInput` 就能第一次让这些类可测。

## AI 使用披露

本阶段由 Claude Code 完成：代码检索、文件搬移与删除、`CMakeLists.txt` 调整、本记录撰写。
第四节列出的静态检查是实际执行的命令输出；构建与测试**未执行**，已如实说明。


---

# 阶段 1：建立 `links_core_base` 目标（编译器强制的无 Qt 边界）

- **日期：** 2026-09-07
- **提交：** 见分支 `refactor/decouple-core-from-qt`

## 一、这一阶段做什么

只做构建系统改动，**不动任何一行 C++**。新建静态库目标 `links_core_base`，
里面**只放已经确认零 Qt 的文件**，并且这个目标**绝不链接任何 `Qt6::*`**。

这样做的意义：Qt 头文件只能通过 `Qt6::*` 的 usage requirements 进入编译器搜索路径。
不链接 Qt，`#include <QString>` 就是「找不到文件」的硬编译错误，而不是悄悄编译通过。
同时把 `AUTOMOC` 关掉——`qt_standard_project_setup()` 会全局打开它——于是混进一个
`Q_OBJECT` 会变成链接期的未解析符号错误。约束由编译器保证，不再靠人自觉。

## 二、具体改了哪些文件

| 文件 | 改动 | 对应问题 |
| --- | --- | --- |
| `CMakeLists.txt` | 新增 `links_core_base` 静态库：`platform_window_ops`、`thumbnail_service`、`desktop_capture/**`（含三个平台后端）、`recording/**`，以及 `image_types.h`/`window_types.h`。设置 `AUTOMOC/AUTOUIC/AUTORCC OFF`、`cxx_std_20`、`POSITION_INDEPENDENT_CODE ON` | 立起无 Qt 边界 |
| 同上 | 这些文件从 `SOURCES`/`HEADERS` 中移除；三个平台的 `list(APPEND SOURCES ...)` 块整体移入新目标 | 避免重复编译 |
| 同上 | `find_library`（macOS 六个 framework + ScreenCaptureKit）与 `find_package(X11)` 从 `qt_add_executable` **之后**上提到 `add_library` **之前**；平台库改为挂在 `links_core_base` 上并用 `PUBLIC` | 库要在使用它的目标之前声明；`links` 通过传递依赖照样拿到 |
| 同上 | Windows 下 `d3d11 dxgi windowsapp dwmapi Shcore` 移到 `links_core_base`；`ntdll winmm msdmo dmoguids strmiids wmcodecdspuuid` 留在 `links` | 前五个是桌面捕获用的，后六个注释写明是 livekit_ffi 需要的 |
| 同上 | `target_link_libraries(links PRIVATE links_core_base ...)` | 主程序链接新库 |
| 同上 | MSVC 下给 `links_core_base` 加 `/utf-8` | 见第三节 |
| `tests/CMakeLists.txt` | `desktop_capture_tests`、`recording_core_tests`、`capture_platform_tests`、`desktop_capture_integration_tests` 不再逐个重列 `core/*.cpp`，改为链接 `links_core_base` | 消除源文件清单重复；这四个目标本来就不链 Qt |
| 同上 | 删除 `CAPTURE_PLATFORM_OPS_SOURCES` / `CAPTURE_PLATFORM_CAPTURER_SOURCES` / `CAPTURE_PLATFORM_LIBS` 三个变量及其 `find_library`/`find_package(X11)` 重复声明 | 这些内容现在全部由 `links_core_base` 提供 |
| `.github/workflows/build.yml` | 在 checkout 之后加一步 `Check Qt-free core files`，对已经无 Qt 的文件 grep `#include <Q...>` 与 `Q_OBJECT` | 编译错误已经能拦住，这一步只是失败得更早、报错更清楚 |

`core/audio_processing_module.cpp` **没有**放进新目标：它目前仍然
`#include "../utils/logger.h"`（Qt），只有在定义了 `AUDIO_PROCESSING_TESTS` 时才不包含。
要等阶段 2 换成无 Qt 的日志门面之后才能搬。

## 三、顺带修掉的一个隐患：`/utf-8`

`core/screen_capturer.cpp:198` 有一个中文字面量 `"窗口已关闭，停止共享"`，
而这个文件**没有 UTF-8 BOM**（已用 `od -c` 确认开头是 `/ * \n`）。
它今天能正常工作，是因为 `QString(const char*)` 等价于 `QString::fromUtf8`，
MSVC 在没有 BOM 时按当前代码页解释窄字符串字面量——一旦这个文件改用 `std::string`，
这层保护就没了，中文会变成乱码。因此现在就给 `links_core_base` 加上 `/utf-8`（仅 MSVC），
等 `screen_capturer.cpp` 后续搬进来时约束已经就位。这是 `core/` 里唯一一个非 ASCII 源文件
（已全仓库检索确认）。

## 四、验证情况

**仍然没有构建，没有跑测试**，原因同阶段 0：本会话的 WSL 环境没有 C++ 工具链，也没有 Qt6。

实际执行的静态检查：

1. 两个 `CMakeLists.txt` 中列出的每个路径都在磁盘上存在（`tests/` 下的
   `core/test_*.cpp` 是相对 `tests/` 的路径，已单独确认存在）。
2. 每个 `.cpp`/`.mm` 的列出次数符合预期：只有 `audio_processing_module.cpp`（3 次）、
   `network_stats_aggregator.cpp`、`participant_metadata_parser.cpp`、
   `microphone_capturer.cpp`、`logger.cpp`（各 2 次）重复，这些都是测试目标
   刻意重新编译源文件的既有做法，不是本次引入的。
3. **逐个检查了搬入 `links_core_base` 的全部文件的 `#include`**：只有相对路径
   （都在 `core/` 内解析）、平台 SDK 头（Windows/CoreFoundation/X11）和标准库，
   **没有 livekit、没有 webrtc、没有 Qt、没有 utils**。因此把 include 路径收窄成
   `${CMAKE_CURRENT_SOURCE_DIR}` + `${CMAKE_CURRENT_SOURCE_DIR}/core` 是够用的——
   这是本阶段最容易出错的地方，专门核对过。
4. `links_core_base` 那段 CMake 里除注释外没有出现 `Qt6::`。
5. 新加的 CI 检查在当前代码树上实际跑过：通过；并验证了它确实能匹配到
   `#include <QString>` 这样的行。

**合并前仍需在 Windows 上补跑** `build.cmd release` 与 `build.cmd tests`。
本阶段改动集中在构建系统，是最可能出现「本地静态检查看不出来」的问题的一类改动，
尤其是三个平台的条件分支只有各自平台能验证。

## 五、遗留事项

1. 阶段 2（无 Qt 日志门面 + 执行器/定时器端口 + `VideoFrame`）尚未开始。
2. macOS 与 Linux 分支的 CMake 改动**完全没有验证过**——本机只能做文本层面的核对。
   CI 的三平台矩阵（`.github/workflows/build.yml`）是第一个真正的验证点。
3. `links_core_base` 是静态库，Windows 上对于不引用 LiveKit 符号的测试目标
   （`desktop_capture_tests`、`recording_core_tests`）不应产生 `livekit.dll` 的运行期依赖。
   这一点**需要在第一次 Windows 构建时实测确认**；如果确实需要，`tests/CMakeLists.txt`
   里现成的 `copy_runtime_if_exists()` 一行即可解决。
4. 设计评审中发现、将在后续阶段生效的一条硬约束：`emit`、`signals`、`slots`、
   `foreach`、`forever` 都是 Qt 宏，而 `ui/backend/*.cpp` 会同时包含 core 头文件和 Qt。
   因此 core 的多播事件类型**其成员函数不能叫 `emit()`**，必须用 `notify()`。
   这条在阶段 4 设计 `Signal` 时必须遵守。

## AI 使用披露

本阶段由 Claude Code 完成：CMake 目标拆分、测试目标改写、CI 检查、UTF-8 隐患定位、本记录撰写。
第四节列出的静态检查均为实际执行的命令输出；构建与测试**未执行**。

---

# 阶段 2–4：core 彻底去 Qt（端口 + 适配器）

- **日期：** 2026-09-07
- **提交：** `88865f3`（基础设施）、`e4e2de4`（主体改造）、`1a5a039`（修复脚本转换引入的缺陷）

## 一、用户中途调整的要求

> Next, go ahead and write the code for all the stages directly; there's no need to wait for me to
> perform a build. I'll run the build once you've finished writing the code for every stage.

原计划是「每阶段结束后由用户在 Windows 上构建、回报错误，再进入下一阶段」。用户改为
一次性写完全部阶段、最后统一构建。**因此从阶段 2 起，所有代码都没有经过任何编译验证**，
这一点在第四节如实展开。

## 二、阶段 2：无 Qt 基础设施（`88865f3`）

纯新增，core 里没有任何文件引用它们，因此不改变行为。

| 新文件 | 作用 | 关键设计取舍 |
| --- | --- | --- |
| `core/base/log.{h,cpp}` | 日志门面 + 可安装 sink | 刻意做成进程级全局而不是注入引用：core 里有约 190 个调用点，其中若干在匿名命名空间的自由函数里，为一个横切关注点把 logger 引用穿过所有签名不值得。测试装一个捕获用的 sink 即可 |
| `core/base/signal.h` | 类型化多播 + RAII `Connection`/`ConnectionBag` | **发射方法必须叫 `notify()` 而不是 `emit()`**——这个头文件会被同时包含 Qt 的翻译单元（每个 `ui/backend/*.cpp`）引用，而 `emit`/`signals`/`slots`/`foreach`/`forever` 都是 Qt 宏。`Connection`/`ConnectionBag` 复刻 Qt 的「接收者 QObject 析构即自动断开」，这是丢掉 QObject 之后最大的安全网缺口 |
| `core/base/executor.h` | `TaskRunner`、`BackgroundExecutor`、`LifetimeToken`、`postGuarded` | `post()` 注释里写明**必须始终异步**，即使已经在主线程也不能直接调用：`room_controller.cpp:41-43` 记录了一个死锁，靠的正是 SDK 线程的调用栈先展开 |
| `core/base/timer.h` | `Timer`/`TimerFactory` | 让 core 的定时器仍然跑在 Qt 事件循环上，保持现有时序 |
| `core/base/strings.{h,cpp}` | `QString::arg` 的替代 | `cat()` 故意**不提供 float/double 重载**，强制调用方通过 `num(value, decimals)` 指定精度——`QString::arg(d, 0, 'f', 1)` 本来就要求指定，`std::to_string(double)` 固定 6 位小数会静默改变输出。大小写函数一律 ASCII-only 并注释说明原因：字符串是 UTF-8，对 UTF-8 字节跑 `std::toupper` 会破坏多字节序列；现有调用方处理的都是 RTP/ICE 协议标识符。`trimWhitespace` 额外剥离 U+00A0 与 U+3000，因为 `QString::trimmed()` 是 Unicode-aware 的，聊天消息的空串判断依赖这一点 |
| `core/media/video_frame.{h,cpp}` | `shared_ptr<const RawImage>` 句柄 | 见第三节「视频通路」 |
| `ui/adapters/qt/qt_{task_runner,timer_factory,log_sink}.*` | Qt 侧实现 | `QtTaskRunner` 用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`，`QtBackgroundExecutor` 用 `QtConcurrent::run` 同一个全局线程池 |

`vcpkg.json` 增加 `nlohmann-json`（header-only），供 core 仅剩的两处 JSON 使用。

## 三、阶段 3–4：core 主体改造（`e4e2de4`）

### 值类型与字符串

`conference_types.h`、`participant_store`、`room_controller`、`network_stats_aggregator`
全部改为 `std::string` / `std::vector` / `std::map` / `std::int64_t`。

**编码是安全的，而且实际上更好**：LiveKit SDK 本来就是 UTF-8 的 `std::string`，
`room_event_delegate.cpp` 过去要做约 40 次 `QString::fromStdString`，出去时再 `.toStdString()`。
改成 `std::string` 是**去掉**了一次 UTF-8↔UTF-16 往返，而不是增加。边界契约：
core 内部一律 UTF-8，适配器层用 `QString::fromStdString`/`toStdString`（Qt 定义这两个就是 UTF-8）。

`Q_DECLARE_METATYPE` 与三处 `qRegisterMetaType` 全部删除。核对过：`ConferenceBackend`
自己发给 QML 的信号只携带 `QString`/`QImage`/基本类型，core 的结构体从不穿越队列连接。

一处**行为差异**如实记录：`std::map` 按 UTF-8 字节序排序，`QMap` 按 UTF-16 码元排序，
两者仅在 U+FFFF 以上的字符处不同。participant identity 由服务端下发且是 ASCII，实际不受影响。

### 事件机制

9 个 `QObject` 子类、约 60 个信号全部改成 `links::core::Signal`。每个订阅方持有
`ConnectionBag`，并在析构函数**第一行**调用 `clear()`。这是丢掉 QObject 之后必须靠纪律维持的不变式。

### 线程

`RoomEventDelegate` 契约不变：在 SDK 线程上就地把值读出来，然后 `postGuarded` 到主线程。
`LifetimeToken` 声明为**最后一个成员**，保证它最先析构、使在途的 post 全部失效。
`MediaPipeline` 的 reader 线程同样处理。`QTimer`→`core::Timer`（仍是 Qt 事件循环，
500ms / 1s 周期不变），`QElapsedTimer`→`steady_clock`，
`QtConcurrent`+`QFutureWatcher`→`BackgroundExecutor` + guarded post，
并保留原有的 `networkStatsPollSeq_` 陈旧结果判别。

### 视频通路

`QImage` → `core::VideoFrame`。**不能**用 `RawImage` 传值：`QImage` 是隐式共享的，
今天的成本是「创建时一次深拷贝，之后每一跳免费」；改成传值会在
`MediaPipeline → DeviceController → ConferenceManager → ConferenceBackend → VideoRenderer`
（外加 `LocalRecordingManager`）每一跳都做一次整帧 memcpy，每个参会者、每秒 15–30 帧。
`toQImage()` 对 RGBA 是零拷贝（QImage 借用缓冲区，用 cleanup 函数持有 `shared_ptr`）。
`ScreenCapturer` 现在从 `DesktopFrame` 直接生成 `VideoFrame`，比原来
`DesktopFrame → QImage → std::vector` 少一次整帧拷贝。

`copyFrom`/`toPackedRgba` 会把带 padding 的行重新紧凑排列——这是 `QImage::copy()` 原本
默默做掉的事，DXGI 返回的 stride 可能大于 `width*4`，漏掉这一步屏幕共享会撕裂/错切。

### 设备后端

新增端口 `VideoInput`、`AudioInput`、`AudioPlayer(+Factory)`、`MediaDeviceRegistry`。
**刻意只把设备管道搬到 Qt 侧**：APM、10ms 分帧、重采样、推送给 LiveKit 全部留在 core。
AEC 反向参考信号的位置与内容**一字未改**——仍然喂**未重采样**的源采样率数据，
且仍然在 `write()` **之前**调用。这一点改错了不会崩，只会让回声消除悄悄变差。

### 屏幕目标

`QScreen*`/`WId` → `core::MonitorId`/`WindowId`。`QScreen` → `HMONITOR` 的匹配逻辑
搬到 `qt_capture_adapter::resolveMonitorId()`，core 新增 `enumerateMonitors()`。
这一步才真正去掉了 `QGuiApplication::primaryScreen()`，也就去掉了 core 对 `Qt::Gui` 的依赖。

### 设置

`DeviceController` 不再读 `QSettings`，改为构造时接收 `AudioProcessingConfig` +
`DeviceSelection`；设备切换后通过 `preferredCamera/MicrophoneChanged` 向上报告，
由 `ConferenceBackend` 负责持久化。

### 顺带修掉的两个既有缺陷

1. `DeviceController::connectScreenSignals()` 里的
   `static QMetaObject::Connection screenConn` 是函数级 static，意味着第二个
   `DeviceController` 实例会把第一个实例的处理器断开。改为每实例持有。
2. `MediaPipeline` 用裸 `new std::atomic<bool>` 管理 stop flag，`stopTrack` 路径上会泄漏。
   改为 `shared_ptr`，reader 线程持强引用。

## 四、验证情况

**完全没有构建，没有跑测试。** 本会话的 WSL 环境没有 cmake / ninja / 任何 C++ 编译器 / Qt6，
用户也已确认改为最后统一构建。这是一次约 6600 行、涉及线程与音视频通路的重构，
**在没有编译反馈的前提下写成，必然存在编译错误**，请以第一次构建的报错为准。

实际执行的静态检查（全部通过）：

1. `core/` 中不存在 `#include <Q...>`、`Q_OBJECT`、`Q_DECLARE_METATYPE`，
   不存在被当作标识符使用的 `emit`/`signals`/`slots`，也不再包含 `utils/`。
2. 每个在 `.cpp` 里 `notify()` 的信号都在对应头文件中有声明（脚本比对，9 个类）。
3. 每个在 `.cpp` 里定义的成员函数都在头文件中有声明（脚本比对，9 个类）。
4. 两个 `CMakeLists.txt` 列出的路径都存在；`core`/`ui`/`utils` 下每个 `.cpp`/`.mm` 都被列到。
5. 搬进 `links_core_base` 的文件的 `#include` 全部可在收窄后的 include 路径下解析。
6. **逐条比对了脚本转换前后的格式化字符串**：把原始 `QString(...).arg(...)` 的占位符数量
   与转换结果对照，发现并修复了 6 处参数丢失（见下）。

### 脚本转换引入、随后被发现并修复的缺陷（`1a5a039`）

日志改写用脚本完成（189 处），复查时发现两类错误：

- **`QString::arg` 的多参数重载**：`.arg(a, b)` 一次填 `%1` 和 `%2`，脚本当成单参数只取了第一个，
  导致 5 处消息里残留字面量 `"%2"` 且第二个值被丢弃。
- **两位数占位符**：网络统计那行用了 `%10`/`%11`/`%12`，`%1` 先匹配，展开成「参数 1 + 字面数字 0」。
  按原文手工重写。

另外发现 `ConferenceManager::applyAudioSettings()` 在设置反转过程中丢了参数，无法把配置传下去。
以及脚本遗留的 `Qt::CaseInsensitive` 版 `contains`、`.toLower()`、`QStringList::join`，一并替换。

**这说明脚本化改写必须逐条复查**——如果只看「是否还有 Qt 符号」，这 6 处都能通过检查。

**合并前必须由能跑构建的人补跑**：`build.cmd release`、`build.cmd tests`，
以及第五节列出的手工联调项。

## 五、遗留事项 / 必须人工验证的点

1. **编译错误**：见第四节，预期存在，需要按报错逐个修。
2. **macOS / Linux 分支完全未验证**：CMake 的三平台条件分支、`enumerateMonitors()`
   目前只在 Windows 下有实现（mac/X11 返回空，因为那两个后端本来就按 index/display id 选屏），
   需要在对应平台上确认屏幕共享仍能选中正确的显示器。
3. **音频回归风险最高**：`QtAudioPlayer` 必须复刻原来的重建条件与缓冲行为
   （刻意没有设置 buffer size，保持 Qt 默认值）。需要**用扬声器而不是耳机**做 10 分钟
   3 人通话，听是否有回声（AEC 参考信号）和爆音（sink 重建抖动）。
4. **回调晚于析构**：这是本次改造引入的唯一新增安全风险。建议在 Linux 上跑一次
   ASan 构建，重复「通话中断开」20 次。
5. **时序**：屏幕共享 500ms 去抖、待发布重试 500ms、网络统计 1s 轮询、
   订阅后两处 100ms singleShot——都改成了 `core::Timer`，需要实际验证。
6. **非 ASCII 往返**：用中文昵称 + 含 emoji 的聊天消息，与一个**未改动的旧版本**互通，
   确认双向都正常（聊天走 JSON 线格式，`nlohmann` 与 `QJsonDocument` 的输出需要互相兼容）。
7. `docs/` 与 `CLAUDE.md` 已同步更新架构描述；`docs/README.md` 本来就过时，未处理。

## AI 使用披露

阶段 2–4 全部由 Claude Code 完成：接口设计、代码改写（含一个自写的日志转换脚本）、
CMake 与测试调整、CI 检查、`CLAUDE.md` 更新、本记录撰写。
第四节列出的静态检查是实际执行的命令输出；**构建与测试未执行**，
且本阶段代码从未经过编译器检验。
