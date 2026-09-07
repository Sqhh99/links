# 工作记录：LiveKit C++ SDK 从 0.3.4 升级到 1.10.1

- **日期：** 2026-09-07
- **分支：** `chore/upgrade-livekit-1.10.1`（基于 `main` 的 `c61702f`）
- **关联文档：** [PR 记录](../pull-requests/2026-09-07-upgrade-livekit-1.10.1.md)

## 一、用户的请求

> Please take a look at the project. I need to upgrade the LiveKit version from 0.3.4 to 1.10.1;
> I have already downloaded `livekit-sdk-windows-x64-1.10.1` into the `third_party` directory.
> This is a significant version jump, so there may be API changes or upgrades that require your
> attention to ensure compatibility and functional stability.

要求：把 LiveKit C++ SDK 从 0.3.4 升级到 1.10.1，处理这次跨大版本升级带来的 API 变化，
保证编译通过、功能行为不退化。

补充确认的三个决定（升级前询问用户）：

1. 范围：修复全部破坏性变更，并额外采用新增的 `Room::disconnect()`；暂不采用
   `Room::getStats()`、`Room::connectionState()`、`adaptive_stream`。
2. Linux 产物：从上游已废弃的 `livekit-sdk-linux-*` 切换到 `livekit-sdk-ubuntu-24.04-*`。
3. 分支：从 `main` 新建 `chore/upgrade-livekit-1.10.1`，与 `chore/upgrade-cpp20` 分开评审。

## 二、制定的计划

1. 逐个头文件 diff 两份 SDK（`third_party/livekit-sdk-windows-x64-0.3.4` 与 `-1.10.1`），
   确定哪些 API 变了、哪些没变，避免盲目改代码。
2. 核对上游 v1.0.0 的 breaking change 说明，与 diff 结果互相印证。
3. 升级构建配置里的版本号与 Linux 产物名。
4. 按 diff 结果修改受影响的源码。
5. 在 Windows 上实际编译、链接、跑测试，确认没有编译期/链接期/运行期回归。

### diff 得出的关键结论

**变了（必须改）：**

| 变化 | 影响文件 |
| --- | --- |
| `Room::Connect` → `connect`，`room_info()` → `roomInfo()` | `room_controller.cpp` |
| `localParticipant()` / `remoteParticipant()` / `remoteParticipants()` 改返回 `weak_ptr` | `room_controller`、`conference_manager`、`device_controller` |
| `livekit::initialize()` 去掉第二个参数，`LogSink` 枚举被删除 | `main.cpp` |
| `AudioFrame::sample_rate()/num_channels()` → `sampleRate()/numChannels()` | `media_pipeline.cpp` |
| Windows 导出改为 `LIVEKIT_API`（`__declspec(dllimport)`），导入库从 5.8MB 缩到 376KB | 链接期风险 |
| 1.x 起 `livekit::initialize()` 必须先于任何 FFI 调用，否则报 "LiveKit is not initialized" | `tests/core/test_microphone_capturer.cpp` |

**没变（确认后未改动）：** `TrackKind`、`TrackSource`、`ConnectionState`、`ConnectionQuality`
的枚举值完全一致，`DisconnectReason` 只在末尾新增 `AgentError`——因此项目里约 23 处
把枚举当 `int` 通过 Qt 队列连接传递的地方仍然正确。`RoomDelegate` 全部 12 个 override 签名不变
（1.10.1 只新增了 `onTokenRefreshed`）。事件结构体、`publishTrack`/`unpublishTrack`/`publishData`、
`VideoStream`/`AudioStream::fromTrack` 与 `read()`、`VideoSource`/`AudioSource` 构造函数与
`captureFrame`、`Track::setPublication`、`network_stats_aggregator` 用到的全部 `RtcStats`
字段路径，全都没有变化。

## 三、具体改了哪些文件

| 文件 | 改动 | 对应问题 |
| --- | --- | --- |
| `cmake/FetchLiveKitSDK.cmake` | 默认版本 `0.3.4` → `1.10.1`；Linux 平台标识 `linux` → `ubuntu-24.04` | 版本升级；上游 v1.9.0 起废弃 `linux` 产物，改用发行版标识的产物 |
| `main.cpp` | `livekit::initialize(LogLevel::Info, LogSink::kConsole)` → `initialize(LogLevel::Info)` | `LogSink` 枚举在 1.0.0 被删除 |
| `core/conference/room_controller.h/.cpp` | `Connect`→`connect`、`room_info()`→`roomInfo()`；`localParticipant()` 改返回 `shared_ptr`（内部 `.lock()`）；新增 `remoteParticipant(identity)`；`remoteParticipants()` 逐个 `lock()` 后返回，保持原签名；新增 `disconnectFromRoom(reason)` | 把 `weak_ptr` 的变化收敛在这一层，让调用方几乎不用改；同时提供 1.10.1 新增的显式断开接口 |
| `core/conference/conference_manager.cpp` | `remoteParticipant` 改走 `RoomController`；两处 `localParticipant()` 取值方式适配 `shared_ptr`；断开流程在 `reset()` 之前先调用 `disconnectFromRoom(ClientInitiated)` | 适配 `weak_ptr`；用 SDK 的优雅断开代替“靠析构 Room 断开” |
| `core/conference/device_controller.h/.cpp` | 新增私有 `localParticipant()`（`room_->localParticipant().lock()`），11 处直接调用改为走它；两个文件内静态辅助函数的形参由 `LocalParticipant*` 改为 `const std::shared_ptr<LocalParticipant>&` | 适配 `weak_ptr`，且调用点写法基本不变 |
| `core/conference/media_pipeline.cpp` | 9 处 `frame.sample_rate()/num_channels()` 改为 `sampleRate()/numChannels()` | `AudioFrame` 访问器改名 |
| `tests/core/test_microphone_capturer.cpp` | 新增全局 `LiveKitEnvironment`，在跑测试前调用 `livekit::initialize(LogLevel::Warn)` | 1.10.1 强制要求先初始化，否则创建 `AudioSource` 失败（见第四节） |

`room_event_delegate`、`participant_store`、`network_stats_aggregator`、三个 capturer、
`ConferenceBackend` 经 diff 确认无需改动，实际编译也验证了这一点。

## 四、验证情况

验证在 Windows 上真实执行（通过 WSL 调用 `cmd.exe`），工具链为 Visual Studio 18 Community
(MSVC 14.51.36231) + Qt 6.10.0 + Ninja。

**已执行且通过：**

1. `build.cmd release`（仓库默认路径）：CMake 配置成功，正确解析到
   `third_party/livekit-sdk-windows-x64-1.10.1`。**所有涉及 LiveKit 的编译单元全部编译通过**，
   包括本次改动的 `room_controller.cpp`、`device_controller.cpp`、`media_pipeline.cpp`、
   `conference_manager.cpp`、`main.cpp`，以及未改动的 `room_event_delegate.cpp` 和三个 capturer。
   `microphone_capturer_tests.exe` 也成功链接到 1.10.1 的导入库。
2. 完整构建 + 链接：用一个临时构建目录（`out/verify-cpp20`，见下）跑通了
   **139/139 全部目标，零错误**，`bin/links.exe` 成功链接 1.10.1。这一步专门用来验证
   “导入库从 5.8MB 缩到 376KB、改用 `__declspec(dllimport)` 导出”是否会导致未解析符号——
   结论是不会。产物旁的 `livekit.dll` 经 SHA256 校验与 SDK 目录中的文件一致。
3. `ctest`：**55/55 全部通过**，1 个跳过（`MicrophoneCapturerIntegrationTest.SwitchDevices`
   需要两个麦克风设备，与 0.3.4 基线上的表现一致）。

**过程中发现并已修复的真实回归：**

首次跑测试时 4 个 `MicrophoneCapturerIntegrationTest` 失败，错误为
`Failed to create AudioSource: FfiClient::sendRequest failed: LiveKit is not initialized`。
为确认是不是升级导致，用同一份源码针对 0.3.4 重新构建 `microphone_capturer_tests` 并运行，
结果 4 个测试全部通过（日志显示 `LiveKit AudioSource created for microphone`）。
说明 0.3.4 会惰性初始化 FFI，而 1.10.1 强制要求先调用 `livekit::initialize()`。
应用本身在 `main.cpp` 里已经调用，只有测试进程没有，因此在测试文件里补了全局初始化环境。
修复后 55/55 通过。

**未执行（如实记录）：**

- **没有做联调冒烟测试**：登录 / 入会 / 麦克风 / 摄像头 / 设备切换 / 屏幕共享 / 聊天 /
  网络指标 / 离会 / 重新入会这条链路需要连接真实的 LiveKit 服务器与信令服务，本次没有跑。
  尤其是新加的 `Room::disconnect()` 是否会阻塞 UI 线程，必须在真机上确认。
  `out/verify-cpp20/bin/links.exe` 是用 1.10.1 构建好的可执行文件，可直接用来做这项测试。
- **没有在 Linux / macOS 上构建**：当前 WSL 环境没有任何 C++ 编译器、CMake 或 Qt，
  无法验证 `ubuntu-24.04` 产物的下载路径与 GCC/Clang 下的编译。已确认 v1.10.1 的 release
  资产中确实存在 `livekit-sdk-ubuntu-24.04-x64-1.10.1.tar.gz` 和 macOS x64/arm64 产物，
  但实际下载与编译需要依赖 CI（`.github/workflows/build.yml`）来验证。

## 五、遗留事项

1. **`main` 分支在 MSVC 14.51 下本来就编译不过**，与本次升级无关：
   `core/desktop_capture/win/wgc_capturer.cpp` 和 `platform_window_ops_win.cpp` 通过
   C++/WinRT 引入 `<experimental/coroutine>`，而新版 MSVC STL 对该头文件加了
   `static_assert` 硬报错（STL1011）。这两个文件本次没有改动，且切到 C++20 也不能解决
   （C++/WinRT 仍然包含该头文件）。上面第 2、3 项验证是通过临时定义
   `_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS` 绕过后完成的，
   **这个定义没有提交**。需要单独提一个 PR 处理，否则本机 `build.cmd release` 无法完成。
2. 联调冒烟测试待补（见第四节）。
3. 未采用的 1.10.1 新能力，可另行评估：`Room::getStats()`（房间级 `SessionStats`）、
   `Room::connectionState()`、`RoomOptions::adaptive_stream`、`RoomDelegate::onTokenRefreshed`。
4. `third_party/` 下仍留有 0.2.7 / 0.3.1 / 0.3.4 的旧 SDK 目录，可择机清理
   （该目录已被 `.gitignore` 忽略，不影响仓库）。
5. `out/verify-cpp20/` 是本次验证用的临时构建目录（已被 `.gitignore` 忽略），
   保留是为了方便做冒烟测试，用完可直接删除。

## AI 使用披露

本次改动由 Claude Code 完成：SDK 头文件 diff、代码修改、Windows 构建与测试执行、本记录撰写。
所有“已通过”的结论均来自实际执行的命令输出；未执行的部分已在第四节明确列出。
