# Links

Links is a desktop client for real-time audio/video meetings, built with Qt 6 / QML and a C++20 core on top of the [LiveKit](https://livekit.io) C++ SDK. It runs on Windows, macOS and Linux. The UI is in Simplified Chinese.

Its backend, [links-sig-server](https://github.com/Sqhh99/links-sig-server), lives in [`server/`](server) as a Git submodule. The backend handles accounts and meetings and issues LiveKit tokens. Links is designed to be self-hosted: one `docker compose up` runs the backend, PostgreSQL and LiveKit.

## Features

- **Accounts:** log in with a username and password. The first login creates the account; there is no sign-up form or email verification.
- **Meetings:**
  - Start a meeting right away, schedule one, or join by 9-digit meeting number.
  - Meetings can have a password and can allow guests.
  - The home page lists your past and hosted meetings.
- **In a meeting:**
  - Camera and microphone, with device selection.
  - Screen or window sharing.
  - Chat and a participant list; the host can remove participants and end the meeting.
  - Live network statistics.
- **Audio processing:** WebRTC echo cancellation, noise suppression and automatic gain control, all configurable in settings.
- **Local recording:** built in by default on Windows. Other platforms can turn it on with `LINKS_ENABLE_LOCAL_RECORDING`.
- **Appearance:** light and dark themes.

## Quick start

### 1. Clone with the submodule

```bash
git clone --recurse-submodules https://github.com/Sqhh99/links.git
# in an existing checkout:
git submodule update --init server
```

### 2. Start the backend

This needs Docker with Compose v2.

```bash
cd server/docker
cp .env.example .env        # fill in JWT_SECRET and LIVEKIT_API_SECRET (openssl rand -hex 32)
docker compose up -d --build
```

For logs, configuration, and access from other machines, see the [server README](https://github.com/Sqhh99/links-sig-server#readme) and [`docker/README.md`](https://github.com/Sqhh99/links-sig-server/blob/main/docker/README.md) (in Chinese).

### 3. Build and run the client

```bash
./build.sh release      # Windows: build.cmd release
```

The binary is written to `build/bin/links` (`links.exe` on Windows). See [Building](#building) for the prerequisites.

### 4. Point the client at your backend

In the client, open **设置 → 网络 → 信令地址** (Settings → Network → Signaling address). Enter `http://127.0.0.1:8081`, or `http://<host>:8081` if the backend runs on another machine, then save. Out of the box the client points at a hosted server, not at your own.

## Building

### Requirements

- CMake 3.21+ (for the presets) and Ninja
- A C++20 compiler:
  - Windows: Visual Studio 2022 (MSVC)
  - macOS: Xcode
  - Linux: GCC or Clang
- Qt 6.8 or newer (CI uses 6.8.3) with these modules:
  - `Core`, `Gui`, `Network`, `Multimedia`, `Quick`, `QuickControls2`, `Qml`, `Concurrent`, `Svg`
- Git
- vcpkg, for GoogleTest and nlohmann-json. The build scripts clone and bootstrap it into `third_party/vcpkg` if it is missing.

Platform extras:

| Platform | Needed for | Tools |
| --- | --- | --- |
| Windows | Packaging | `windeployqt`; Inno Setup for the installer |
| Linux | Building | X11 and PulseAudio/ALSA development packages |
| Linux | Packaging | `patchelf`, `linuxdeploy` and `linuxdeploy-plugin-qt` |
| macOS | Packaging | `macdeployqt` |

On the first configure, CMake downloads prebuilt LiveKit C++ SDK and WebRTC Audio Processing archives for your platform (see [`cmake/`](cmake)). Configuring is slow because of this, so only re-run it after changing CMake files.

**Windows:** the top-level [`CMakeLists.txt`](CMakeLists.txt) adds `D:/Qt/6.10.0/msvc2022_64` to `CMAKE_PREFIX_PATH`. Change it if your Qt is installed elsewhere.

### Commands

With the helper scripts:

```bash
./build.sh release   # or: test, clean        (Linux / macOS)
build.cmd release    # or: test, clean        (Windows)
```

Or with the CMake presets directly:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

### Build options

The scripts read these environment variables:

| Variable | Effect |
| --- | --- |
| `LINKS_SDK_ARCH` | `x64` (default) or `arm64`. Picks which prebuilt SDK archives are downloaded |
| `VCPKG_TARGET_TRIPLET` | Overrides the vcpkg triplet |
| `LINKS_ENABLE_LOCAL_RECORDING` | `ON`/`OFF`. Defaults to `ON` on Windows and `OFF` elsewhere |

```bash
LINKS_SDK_ARCH=arm64 LINKS_ENABLE_LOCAL_RECORDING=OFF ./build.sh release
```

## Tests

The tests use GoogleTest and cover the Qt-free core. That includes:

- audio processing and microphone capture;
- desktop frame and geometry logic;
- network statistics;
- participant metadata parsing;
- recording helpers;
- screen-capture smoke tests.

Every test target is declared in [`tests/CMakeLists.txt`](tests/CMakeLists.txt).

```bash
ctest --preset release                                             # everything
ctest --preset release -R ParticipantMetadataParser --output-on-failure
```

The backend has its own test suite; see the [server README](https://github.com/Sqhh99/links-sig-server#readme).

## Architecture

```
QML (ui/qml)  →  bridge objects (ui/backend)  →  core (core/)  →  LiveKit C++ SDK
```

- **`core/` contains no Qt at all.** It builds as two static libraries that link no Qt target, so a stray Qt include fails to compile.
  - Where core needs a timer, camera, audio device, main-thread hop or logger, it declares an interface.
  - `ui/adapters/qt/` implements those interfaces with Qt.
- **`ConferenceManager`** is a façade over smaller parts that can each be tested on their own:
  - the room connection;
  - the participant store;
  - the media pipeline, which reads remote tracks and feeds the echo canceller;
  - device control.
- **Threading:** LiveKit callbacks arrive on SDK threads. They are forwarded to the main thread before touching any state.
- **Screen capture** is platform-specific:
  - Windows: WGC, falling back to DXGI and then GDI;
  - macOS: ScreenCaptureKit;
  - Linux: X11.

[`CLAUDE.md`](CLAUDE.md) describes the architecture and conventions in more detail.

## Repository layout

| Path | Contents |
| --- | --- |
| [`core/`](core) | Conference control, capture, media pipeline, desktop capture, recording (Qt-free) |
| [`ui/`](ui) | QML pages and components, Qt bridge classes, Qt adapters for core interfaces |
| [`utils/`](utils) | Logging, persisted settings, shared Qt helpers |
| [`tests/`](tests) | GoogleTest targets |
| [`cmake/`](cmake) | SDK download and configuration helpers |
| [`res/`](res) | Icons and other packaged resources |
| [`tools/`](tools) | Packaging assets and scripts |
| [`third_party/`](third_party) | Vendored dependencies and downloaded SDKs |
| [`server/`](server) | The backend ([links-sig-server](https://github.com/Sqhh99/links-sig-server) submodule) |
| [`docs/`](docs) | Server API reference (`docs/server-api/`), work logs, PR and review records |

## Contributing

- **Register new files in [`CMakeLists.txt`](CMakeLists.txt).** Every source, QML file and resource is listed there explicitly. An unregistered file either fails to link or is silently missing from the QML resources at runtime.
- **Commits** follow the conventional style: `feat(scope): …`, `fix: …`, `docs: …`, `chore: …`.
- **CI:** [`build.yml`](.github/workflows/build.yml) builds and tests on Windows, Linux and macOS for pushes and pull requests to `main` and `develop`.
- **Releases:** pushing a `v*` tag runs [`package.yml`](.github/workflows/package.yml), which builds:
  - a Windows portable zip and installer;
  - a macOS app zip and DMG;
  - a Linux AppImage.

## License

[Apache License 2.0](LICENSE). See also [NOTICE](NOTICE).
