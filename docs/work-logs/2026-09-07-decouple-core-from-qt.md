# 工作记录：把 `core/` 与 Qt 解耦

- **日期：** 2026-09-07
- **分支：** `refactor/decouple-core-from-qt`（基于 `main` 的 `9a30f88`）
- **关联文档：** PR 记录待补（全部阶段完成后再写）

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
