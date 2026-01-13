# Refactoring Log: Screen Capturer Modularization
**Date**: 2026-01-13
**Module**: Core / Screen Capture

## Motivation
The original `screen_capturer.cpp` was a monolithic file (>1300 lines) containing mixed logic for Qt integration, Windows specific APIs (WinRT, DXGI, GDI), and general frame handling. This made it difficult to maintain, test, and extend to other platforms (Linux, macOS).

## Changes

### 1. Modularization
The code has been split into a dedicated module `core/desktop_capture/` with a clear separation of concerns:

- **Public Interface**: Clean, abstract C++ interface decoupled from Qt.
- **Platform Implementations**: Isolated in `win/` subdirectory.
- **Frame Data**: Dedicated `DesktopFrame` class replacing ad-hoc QImage/raw pointer passing.

### 2. Core Abstractions
Introduced standard abstractions for:
- Geometry (`DesktopRect`, `DesktopSize`, `DesktopVector`)
- Frame buffers (`DesktopFrame`)
- Capture device lifecycle (`DesktopCapturer`)

### 3. Windows Refactoring
Extracted specific capture technologies into separate classes:
- `WgcCapturer` (WinRT): For modern, high-performance window capture.
- `DxgiDuplicator` (DirectX): For efficient screen capture and older window capture support.
- `GdiCapturer` (GDI): As a reliable fallback.
- `WindowUtils`: Centralized WIN32 API helper functions.

### 4. Qt Integration
The original `ScreenCapturer` class `core/screen_capturer.h` was preserved but refactored to be a thin adapter. It now:
- Delegates actual capture work to the `desktop_capture` module.
- Converts `DesktopFrame` to `QImage` for the Qt UI.
- Manages the `livekit::VideoSource` integration.
- Handles threading and error recovery (stalls, resets).

## Build Updates
- Updated `CMakeLists.txt` to compile the new module sources.
- Added platform-specific conditional compilation (`if(WIN32)`) to ensure cross-platform build safety.

## Verification
- **Compilation**: Verified build with MSVC 2022.
- **Functionality**:
    - Confirmed WGC works for standard windows.
    - Confirmed fallbacks (DXGI/GDI) function when primary methods fail.
    - Verified resource cleanup and thread safety during start/stop cycles.
