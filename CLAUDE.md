# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build, test, run

```bash
cmake --preset release          # configure (Ninja, binaryDir = build/, LINKS_BUILD_TESTS=ON)
cmake --build --preset release  # build -> build/bin/links
ctest --preset release          # run all discovered GTest cases
```

Wrapper scripts do the same and additionally clone/bootstrap vcpkg into `third_party/vcpkg` if missing:
`./build.sh release|test|clean` (Linux/macOS), `build.cmd release|test|clean` (Windows).

Env vars honored by both scripts: `LINKS_SDK_ARCH` (`x64`/`arm64`, picks the prebuilt LiveKit + WebRTC-APM archive), `VCPKG_TARGET_TRIPLET`, `LINKS_ENABLE_LOCAL_RECORDING`.

Running one test:

```bash
ctest --preset release -R ParticipantMetadataParser --output-on-failure
build/desktop_capture_tests --gtest_filter='DesktopFrameTest.*'   # test binaries sit at build/ root
```

Test targets are declared individually in `tests/CMakeLists.txt`; each links `links_core` or
`links_core_base` rather than re-listing `core/*.cpp`, so a new test means a new `add_executable` +
`target_link_libraries` + `gtest_discover_tests` entry there. No test links Qt. `capture_platform_tests` and
`desktop_capture_integration_tests` only exist on macOS/Linux.

Configure is expensive (it downloads platform SDKs via `cmake/FetchLiveKitSDK.cmake` and
`cmake/FetchWebRTCAudioProcessing.cmake`); reconfigure only when CMake files change.

`CMakeLists.txt` hardcodes `D:/Qt/6.10.0/msvc2022_64` into `CMAKE_PREFIX_PATH` on Windows; CI uses
Qt 6.8.3 and relies on `qt_standard_project_setup(REQUIRES 6.8)`.

## Registering new files (most common build failure)

Everything is listed explicitly in the root `CMakeLists.txt`. A new file that is not registered
either fails to link or is silently absent from the QML resource tree at runtime:

- C++ in `core/` → `LINKS_CORE_BASE_SOURCES`/`_HEADERS` (no LiveKit) or `LINKS_CORE_SOURCES`/`_HEADERS` (LiveKit), inside the `links_core_base` / `links_core` blocks; platform capture sources go in the `if(WIN32)/APPLE/UNIX` branch there
- C++ in `main.cpp`, `ui/`, `utils/` → `SOURCES` / `HEADERS` on the `links` target
- `.qml` → `QML_FILES` inside `qt_add_qml_module`; images, `.js` → `RESOURCES`
- A new QML singleton also needs `set_source_files_properties(... QT_QML_SINGLETON_TYPE TRUE)`, as `ui/qml/theme/Theme.qml` has

The module is `qt_add_qml_module(URI Links RESOURCE_PREFIX / NO_RESOURCE_TARGET_PATH)`, so QML is
loaded by source-relative URL: `qrc:/ui/qml/conference/ConferenceWindow.qml`. QML imports are
`import Links` (components) and `import Links.Backend 1.0` (C++ types).

## Architecture

Layering is strict: **QML → `ui/backend/` bridge objects → `core/` → LiveKit C++ SDK**. Core code
never touches QML; QML never touches LiveKit types.

**`core/` contains no Qt at all** — no Qt headers, no `Q_OBJECT`, no dependency on `utils/` (which
is Qt). It is built as two static libraries, `links_core_base` and `links_core`, that deliberately
link no `Qt6::*` target and have `AUTOMOC OFF`, so a stray Qt include fails to compile and a stray
`Q_OBJECT` fails to link. CI enforces the same rule. Where core needs something Qt provides — the
main-thread hop, timers, a camera, audio in/out, device enumeration, logging — it declares an
interface (`core/base/`, `core/media/`, `core/platform_services.h`) that `ui/adapters/qt/`
implements. `QtPlatformServices` owns those implementations and hands core a `PlatformServices`
of non-owning pointers, so it must outlive the `ConferenceManager`.

Two consequences worth knowing before editing core: strings are UTF-8 `std::string` (convert at the
Qt boundary with `QString::fromStdString`/`toStdString`), and events are `links::core::Signal`
whose emit method is called **`notify()`** — `emit` is a Qt macro and core headers are included from
translation units that also include Qt.

**Startup / window flow (`main.cpp`).** Backends are registered with `qmlRegisterType` into
`Links.Backend` (`ThemeManager`, `AppearanceManager`, `LocalRecordingManager` as singletons), then
`main.qml` → `HomeWindow` loads. The conference window is *not* declared in QML: `LoginBackend::joinConference`
fires `createConferenceWindow()`, which instantiates `ConferenceWindow.qml` imperatively, sets its
properties, and calls `ConferenceBackend::initialize()` on a queued connection. Hiding that window
returns to the login window and deletes it — unless `ShareModeManager::isActive()`, where hiding is
deliberate (the window disappears while you share your screen). `setQuitOnLastWindowClosed(false)`
is what makes this work.

**Conference core.** `ConferenceManager` (`core/conference/`) is a façade composed of collaborators,
each independently unit-testable:

- `RoomController` — owns the `livekit::Room` and its delegate lifetime
- `ParticipantStore` — participant records plus trackSid → kind/source maps (LiveKit events carry only sids)
- `MediaPipeline` — one reader thread per remote video/audio stream; converts frames to `core::VideoFrame`, plays audio through a `core::AudioPlayer` (Qt adapter wraps `QAudioSink`), and feeds far-end audio back through `ReverseAudioCallback` so the AEC has a reference signal. The reverse tap is fed the un-resampled source-rate samples, before the write — do not move it
- `DeviceController` — local capture, publish/unpublish (with pending-unpublish retry and screen-share debounce), and all `AudioProcessingModule` settings
- `NetworkStatsAggregator`, `ParticipantMetadataParser` — pure logic, no Qt/LiveKit coupling

**Threading rule.** LiveKit SDK callbacks arrive on SDK threads. `RoomEventDelegate` implements
`livekit::RoomDelegate` and does nothing but read owned values off the event and post them to the
main thread through `core::TaskRunner` (`core/base/executor.h`), where the matching
`core::Signal` is notified. Never call into `ParticipantStore` or UI from an SDK callback — route
it through the delegate. Two rules make this safe: capture only owned values into a posted task
(never a reference into an SDK event), and declare the object's `LifetimeToken` last so it is
destroyed first and cancels in-flight posts.

**Video path.** `core::VideoFrame` (a `shared_ptr<const RawImage>`, `core/media/video_frame.h`)
travels `MediaPipeline`/capturers → `ConferenceManager` signals → `ConferenceBackend`, which converts
it once with `links::qt_adapter::toQImage()` (zero-copy for RGBA) and emits `localVideoFrameReady`,
`remoteVideoFrameReady`, `*ScreenFrameReady` → QML, which calls `updateFrame()` on a `VideoRenderer`.
`VideoRenderer` wraps the image into a `QVideoFrame` pushed to the `QVideoSink` of a QML
`VideoOutput` — that is why frame delivery looks like a function call from QML rather than a binding.

**Desktop capture.** `core/desktop_capture/` is the platform-agnostic layer (`DesktopCapturer`
factory, RGBA `DesktopFrame`, `desktop_geometry.h`); `core/screen_capturer.h` is the Qt-free LiveKit
wrapper that turns captured frames into a `livekit::VideoSource`; it selects a monitor by
`core::MonitorId`, which `ui/` resolves from a `QScreen` via `qt_adapter::resolveMonitorId()`. Backends: Windows WGC → DXGI → GDI
fallback chain, macOS ScreenCaptureKit, Linux X11. See `core/desktop_capture/README.md`.
`core/platform_window_ops.h` + `ui/adapters/qt/qt_capture_adapter.h` keep window enumeration and
thumbnails free of Qt in the core layer.

**Server side.** `ui/backend/network_client.*` is the REST client (token, auth, meeting
create/join/cancel, records) driven by `LoginBackend`/`AuthBackend`; endpoints are documented in
`docs/server-api/`. It lives in `ui/` because only the UI layer uses it. `Settings` (a `QSettings`
singleton in `utils/`) persists server/API URLs, auth token, device selection, audio-processing
options, and theme — `core/` never reads it; `ConferenceBackend` passes
`core::AudioProcessingConfig` / `core::DeviceSelection` down instead.

**Local recording** is compiled out by `LINKS_ENABLE_LOCAL_RECORDING` (ON only on Windows by default);
`LocalRecordingManager.cpp` provides stubbed behavior when it is 0, so guard new recording code the
same way and keep `recordingAvailable` accurate in QML.

## Conventions

- C++20, 4-space indent, braces on the same line. Classes `PascalCase`, methods `camelCase`.
- File naming splits by layer: `core/` and `utils/` use `snake_case.cpp/h`; `ui/backend/` classes and
  QML components use `PascalCase`.
- Tests live in `tests/core` and `tests/integration`, named `test_*.cpp`.
- Commits follow conventional style (`feat(gallery): ...`, `fix:`, `chore:`, `docs:`, `ci:`).

## Mandatory records

This repo requires permanent records under `docs/`, each named `YYYY-MM-DD-<topic>.md`:

| Trigger | Directory | Language |
| --- | --- | --- |
| Any request that changes files | `docs/work-logs/` | Chinese |
| Any code review | `docs/code-reviews/` | English |
| Before opening a PR (written first, then used as the PR body) | `docs/pull-requests/` | Chinese |

Read `agents/skills/project-records/SKILL.md` for the templates and cross-linking rules before
writing one. Note that `agents/` is gitignored, so that file is local-only and may be absent; the
`CONTRIBUTING.md` and `.github/pull_request_template.md` it references do not currently exist in the
repo — follow the structure in the skill (or an existing record such as
`docs/pull-requests/2026-09-05-upgrade-cpp20.md`) instead.

Never tick a verification checkbox for a command you did not run. State verification status
explicitly, including "not built, not tested" — WSL sessions cannot run the Windows build.

`docs/README.md` is stale (it describes a widget-based `QtConference` binary that no longer exists);
prefer the root `README.md`.
