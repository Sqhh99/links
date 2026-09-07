# PR：LiveKit C++ SDK 升级到 1.10.1

- **日期：** 2026-09-07
- **分支：** `chore/upgrade-livekit-1.10.1` → `main`
- **基线提交：** `c61702f`
- **关联记录：** [工作记录](../work-logs/2026-09-07-upgrade-livekit-1.10.1.md)

## 关联

把 LiveKit C++ SDK 从 0.3.4 升级到 1.10.1。这次跨越了上游的 1.0.0 大版本，
该版本明确列出了一批破坏性变更，需要逐一适配。

## 改了什么

**构建**

- `cmake/FetchLiveKitSDK.cmake`：默认 SDK 版本 `0.3.4` → `1.10.1`。
- 同一文件：Linux 产物名由上游已废弃的 `livekit-sdk-linux-*` 改为
  `livekit-sdk-ubuntu-24.04-*`（CI 跑在 `ubuntu-latest`，即 24.04；上游自 v1.9.0 起
  推荐使用发行版标识的产物，`ubuntu-22.04` 也可选）。

**适配 1.0.0 的破坏性变更**

- `Room::Connect` → `connect`，`Room::room_info()` → `roomInfo()`。
- `Room::localParticipant()` / `remoteParticipant()` / `remoteParticipants()` 改为返回
  `weak_ptr`。这一变化收敛在 `RoomController` 内部（`.lock()` 后对外提供 `shared_ptr`），
  因此 `ConferenceManager` 与 `DeviceController` 的调用点写法基本保持原样。
- `livekit::initialize()` 去掉第二个参数，`LogSink` 枚举被删除。
- `AudioFrame::sample_rate()/num_channels()` → `sampleRate()/numChannels()`。

**行为改进**

- 离会时先调用 1.10.1 新增的 `Room::disconnect(ClientInitiated)` 做优雅断开，
  再销毁 Room 对象；此前只能靠析构来断开。该调用只在主线程执行，
  不会在 `RoomDelegate` 回调内触发（SDK 文档说明那样会死锁）。

**修复升级带来的回归**

- 1.x 起 `livekit::initialize()` 必须先于任何 FFI 调用，否则创建 `AudioSource` 会失败并报
  "LiveKit is not initialized"。应用在 `main.cpp` 里已经调用，但测试进程没有，
  导致 4 个 `MicrophoneCapturerIntegrationTest` 失败。已在
  `tests/core/test_microphone_capturer.cpp` 中补上全局初始化环境。

**确认不需要改的部分**

`TrackKind` / `TrackSource` / `ConnectionState` / `ConnectionQuality` 的枚举值完全一致，
`DisconnectReason` 只在末尾新增 `AgentError`，所以项目里把枚举当 `int` 跨 Qt 队列连接传递的
写法仍然正确。`RoomDelegate` 的 12 个 override 签名不变，事件结构体、发布/取消发布接口、
`VideoStream`/`AudioStream`、两个 Source 的构造与 `captureFrame`、以及
`network_stats_aggregator` 用到的全部 `RtcStats` 字段都没有变化。

## 怎么验证

- [x] Windows 完整构建 + 链接通过（139/139 目标，零错误），`links.exe` 成功链接
      1.10.1 的导入库。这一项专门用来排除“1.10.1 改用 `__declspec(dllimport)` 导出、
      导入库从 5.8MB 缩到 376KB”可能引发的未解析符号问题。
- [x] `ctest` 全量通过：55/55，1 个跳过（`SwitchDevices` 需要两个麦克风，
      与 0.3.4 基线表现一致）。
- [x] 回归定位：用同一份源码针对 0.3.4 重建并运行麦克风集成测试作为基线对照，
      确认失败确实由 1.10.1 的初始化要求引入，而非环境问题。
- [ ] 联调冒烟：登录 / 入会 / 麦克风 / 摄像头 / 设备切换 / 屏幕共享 / 聊天 /
      网络指标 / 离会 / 重新入会。**未执行**，需要真实的 LiveKit 服务器与信令服务。
      重点确认新增的 `Room::disconnect()` 不会阻塞 UI 线程。
- [ ] Linux / macOS 构建。**未执行**，当前开发环境（WSL）没有编译器、CMake 和 Qt。
      需要由 CI（`.github/workflows/build.yml`）覆盖，重点是 `ubuntu-24.04` 产物的下载路径。

> 说明：Windows 上仓库默认的 `build.cmd release` 目前**无法完成**，原因与本 PR 无关——
> `core/desktop_capture/win/` 下两个文件经 C++/WinRT 引入 `<experimental/coroutine>`，
> 新版 MSVC STL（14.51）对该头文件加了 `static_assert` 硬报错。上面的构建与测试是在
> 临时定义 `_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS` 绕过后完成的，
> **该定义未包含在本 PR 中**，建议单独提 PR 处理。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（仓库中当前不存在该文件）
- [ ] UI / QML 改动附了截图（本 PR 无 UI / QML 改动，不适用）
- [x] 未改动翻译库、i18n、模型或打包资源；打包脚本通过通配符查找运行库，
      不含版本号字面量，因此无需同步修改
- [x] AI 使用披露：是。SDK 头文件 diff、全部代码修改、Windows 构建与测试执行、
      以及本记录均由 Claude Code 完成；所有勾选项都对应实际执行过的命令输出，
      未执行的项保持未勾选。
