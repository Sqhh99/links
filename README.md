# Links

`Links` is a Qt 6 desktop client for real-time audio/video meetings. The project combines a QML-based UI, a C++ conference core, LiveKit-based media transport, device capture, chat, screen sharing, and local recording support.

## Current Scope

The repository currently contains:

- A Qt Quick desktop client under active development
- Core conference/media control logic in C++
- Cross-platform build scripts for Windows, Linux, and macOS
- GitHub Actions workflows for build validation and packaging
- GoogleTest-based unit and integration-style tests for core modules

The repository also contains a `server/` directory in the project layout notes, but the main actively built target in the current top-level CMake project is the desktop client application.

## Main Capabilities

- Email/password login and meeting join/start flows
- Real-time conferencing with LiveKit
- Microphone and camera device control
- Screen sharing
- In-meeting chat and participant state updates
- Local recording pipeline
- Theme, appearance, audio, video, and network settings

## Repository Layout

Key directories:

- [core](/mnt/d/workspace/cpp-workspace/links/core): conference control, capture, media pipeline, desktop capture, recording core
- [ui](/mnt/d/workspace/cpp-workspace/links/ui): QML pages/components and Qt backend bridge classes
- [utils](/mnt/d/workspace/cpp-workspace/links/utils): logging, settings, shared helpers
- [tests](/mnt/d/workspace/cpp-workspace/links/tests): GoogleTest targets and test registration
- [cmake](/mnt/d/workspace/cpp-workspace/links/cmake): SDK fetch/config helpers
- [res](/mnt/d/workspace/cpp-workspace/links/res): icons and packaged resources
- [tools](/mnt/d/workspace/cpp-workspace/links/tools): packaging assets and scripts
- [third_party](/mnt/d/workspace/cpp-workspace/links/third_party): vendored dependencies and downloaded SDKs

## Build Requirements

Minimum tooling reflected by the current build files:

- CMake 3.21+ for presets
- C++17 compiler
- Ninja
- Qt 6.8.x or 6.10.x with at least:
  - `Core`
  - `Gui`
  - `Network`
  - `Multimedia`
  - `Quick`
  - `QuickControls2`
  - `Qml`
  - `Concurrent`
- Git
- vcpkg

Additional platform tools currently used by the repository:

- Windows:
  - Visual Studio C++ toolchain
  - `windeployqt`
  - Inno Setup for installer packaging
- Linux:
  - `patchelf`
  - X11 and Pulse/ALSA development libraries
  - `linuxdeploy` + `linuxdeploy-plugin-qt` during packaging
- macOS:
  - Xcode toolchain
  - `macdeployqt`

## Third-Party SDKs

The project uses platform-specific prebuilt SDKs that are resolved by CMake fetch modules:

- LiveKit C++ SDK
- WebRTC Audio Processing

Architecture is selected through `LINKS_SDK_ARCH`:

- `x64`
- `arm64`

The current CI and packaging configuration uses:

- Windows: `x64`
- Linux: `x64`
- macOS: `x64`

## Build Commands

### CMake presets

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

### Helper scripts

Windows:

```bat
build.cmd release
build.cmd test
build.cmd clean
```

Linux / macOS:

```bash
./build.sh release
./build.sh test
./build.sh clean
```

## Important Build Variables

The current scripts and workflows support these inputs:

- `LINKS_SDK_ARCH`
  - Selects SDK architecture, for example `x64` or `arm64`
- `VCPKG_TARGET_TRIPLET`
  - Overrides the vcpkg triplet
- `LINKS_ENABLE_LOCAL_RECORDING`
  - Controls whether local recording is compiled in

Examples:

```bash
LINKS_SDK_ARCH=x64 ./build.sh release
LINKS_SDK_ARCH=x64 LINKS_ENABLE_LOCAL_RECORDING=OFF ./build.sh test
```

```bat
set LINKS_SDK_ARCH=x64
set LINKS_ENABLE_LOCAL_RECORDING=ON
build.cmd release
```

## Platform Notes

- Local recording is enabled by default on Windows in the top-level CMake configuration.
- Local recording is disabled by default on non-Windows platforms unless explicitly enabled.
- The desktop capture implementation is platform-specific:
  - Windows: WGC / DXGI / GDI paths
  - macOS: ScreenCaptureKit-based path
  - Linux: X11 path
- The top-level `CMakeLists.txt` currently prepends a Windows Qt path (`D:/Qt/6.10.0/msvc2022_64`) when building on Windows. Adjust this locally if your environment differs.

## Tests

The current test suite includes coverage for several core areas, including:

- Audio processing
- Microphone capturer
- Desktop frame / geometry logic
- Conference network stats aggregation
- Participant metadata parsing
- Recording core helpers
- Cross-platform capture code on supported Unix platforms

Tests are registered from [tests/CMakeLists.txt](/mnt/d/workspace/cpp-workspace/links/tests/CMakeLists.txt).

## CI and Packaging

GitHub Actions workflows live under [.github/workflows](/mnt/d/workspace/cpp-workspace/links/.github/workflows):

- `build.yml`
  - Cross-platform build and test validation
- `package.yml`
  - Windows portable zip + installer
  - macOS app zip + DMG
  - Linux AppImage

Release packaging is currently triggered by tags matching:

```text
v*
```

## Development Notes

- QML files and backend sources must be registered in [CMakeLists.txt](/mnt/d/workspace/cpp-workspace/links/CMakeLists.txt).
- This repository uses Qt `AUTOMOC` and `qt_add_qml_module`, so missing source registration is a common cause of build errors.
- The fetch modules under [cmake](/mnt/d/workspace/cpp-workspace/links/cmake) are part of the active build path and should be kept in sync with CI architecture settings.

## License

See [LICENSE](/mnt/d/workspace/cpp-workspace/links/LICENSE) and [NOTICE](/mnt/d/workspace/cpp-workspace/links/NOTICE).
