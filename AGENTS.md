# Repository Guidelines

## Project Structure & Module Organization
This repo contains a Qt-based client app plus core SDK integrations.
- `core/`: C++ application logic (conference, media, desktop capture).
- `ui/`: QML UI plus C++ backends in `ui/backend/`.
- `utils/`: shared helpers (logging, settings).
- `tests/`: GoogleTest suite and test CMake configuration.
- `server/`: signaling server code and static assets.
- `res/`: UI icons and resources referenced by QML.
- `third_party/`: third-party dependencies (vcpkg, LiveKit SDK).
- `docs/`: app documentation.
- `build/`: generated build artifacts (do not edit).

If you add new sources or QML files, update `CMakeLists.txt` to register
them in the build and resources lists.

## Build, Test, and Development Commands
This repo uses CMake presets (see `CMakePresets.json`).
- `cmake --preset release`: configure the Release build.
- `cmake --build --preset release`: build the app.
- `ctest --preset release`: run the test suite.

Windows helper script:
- `build.cmd release`: configure and build Release.
- `build.cmd test`: build and run tests.
- `build.cmd clean`: remove `build/`.

## Coding Style & Naming Conventions
- C++17 is required (see `CMakeLists.txt`).
- Indentation: 4 spaces, braces on the same line.
- Classes use `PascalCase` (for example, `DesktopFrame`).
- Methods use `camelCase` (for example, `copyPixelsFrom`).
- File names use `snake_case` in C++ and `PascalCase` for QML components.
- No formatter is configured in-repo; keep style consistent with nearby code.

## Testing Guidelines
- Framework: GoogleTest (`GTest::gtest` and `gtest_discover_tests`).
- Place tests under `tests/` and name files `test_*.cpp`.
- Register new tests in `tests/CMakeLists.txt`.
- `LINKS_BUILD_TESTS` controls test builds; it is ON by default.

## Commit & Pull Request Guidelines
Commit messages follow a conventional style in history:
`feat:`, `fix:`, `refactor:`, `chore:`, `style:` with optional scopes
like `feat(gallery): ...`.

For PRs:
- Provide a short summary and list the tests you ran.
- Include screenshots or short clips for UI/QML changes.
- Note any changes to paths or environment assumptions.

## Configuration Notes
`CMakeLists.txt` currently sets Windows-specific paths for Qt and the
LiveKit SDK (for example, `D:/Qt/...` and `third_party/livekit-sdk-windows`).
Adjust these locally or pass cache variables if your setup differs.
