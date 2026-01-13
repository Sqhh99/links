# Desktop Capture Module Architecture

## Overview

The `desktop_capture` module provides a cross-platform, modular interface for capturing screen and window content. It is designed to replace the legacy monolithic `ScreenCapturer` implementation with a structured, extensible architecture inspired by WebRTC.

## Directory Structure

```
core/desktop_capture/
├── desktop_capturer.h       # Abstract public interface
├── desktop_capturer.cpp     # Factory implementation
├── desktop_frame.h          # Frame data container
├── desktop_geometry.h       # Geometry primitives (Point, Size, Rect)
├── capture_options.h        # Configuration options
└── win/                     # Windows-specific implementations
    ├── wgc_capturer.h/cpp   # Windows Graphics Capture (WinRTC)
    ├── dxgi_duplicator.h/cpp# DXGI Desktop Duplication
    ├── gdi_capturer.h/cpp   # GDI Fallback
    └── window_utils.h/cpp   # Windows API helpers
```

## Core Components

### 1. `DesktopCapturer` (Interface)

The main entry point for consumers. It defines the contract for starting, stopping, and controlling capture sessions.

- **Key Methods**:
  - `start(Callback*)`: Begin capture and provide a callback for frames.
  - `captureFrame()`: Request a new frame (pull model).
  - `getSourceList(SourceList*)`: Enumerate available screens or windows.
  - `selectSource(SourceId)`: Choose a target to capture.

- **Factory Methods**:
  - `createScreenCapturer(options)`: Returns a suitable screen capturer for the platform.
  - `createWindowCapturer(options)`: Returns a suitable window capturer for the platform.

### 2. `DesktopFrame`

A platform-agnostic container for raw video frame data.

- **Format**: RGBA (32-bit), compatible with QImage::Format_RGBA8888.
- **Features**:
  - Direct access to raw pixel buffer.
  - Geometry information (size, stride).
  - Capture metadata (timestamp, DPI).
  - `BasicDesktopFrame`: concrete implementation managing its own memory.

### 3. Windows Implementations

The Windows module implements a fallback chain to ensure maximum compatibility:

1.  **WGC (Windows Graphics Capture)**:
    -   **Requirement**: Windows 10 1903 (Build 18362)+.
    -   **Pros**: High performance, captures hardware-accelerated windows, works with minimized windows (sometimes), modern API.
    -   **Cons**: Async-only, requires WinRT interop.

2.  **DXGI (Desktop Duplication)**:
    -   **Requirement**: Windows 8+.
    -   **Pros**: High performance, zero-copy possible (GPU).
    -   **Cons**: Screen-oriented (window capture requires cropping), can't capture occluded windows.

3.  **GDI (PrintWindow)**:
    -   **Requirement**: All Windows versions.
    -   **Pros**: Universal compatibility.
    -   **Cons**: Slow (CPU copy), may flicker, can't capture hardware-accelerated content well.

## Usage Example

```cpp
#include "desktop_capture/desktop_capturer.h"

class MyObserver : public links::desktop_capture::DesktopCapturer::Callback {
    void onCaptureResult(Result result, std::unique_ptr<DesktopFrame> frame) override {
        if (result == Result::SUCCESS) {
            // Process captured frame
        }
    }
};

// ...
links::desktop_capture::CaptureOptions opts;
opts.targetFps = 30;
auto capturer = links::desktop_capture::DesktopCapturer::createWindowCapturer(opts);

capturer->start(new MyObserver());
capturer->selectSource(windowId);

// In a timer loop:
capturer->captureFrame();
```

## Future Extension

To add support for Linux or macOS:
1.  Create `linux/` or `mac/` subdirectories.
2.  Implement `DesktopCapturer` subclass (e.g., `X11Capturer`, `CgCapturer`).
3.  Update `DesktopCapturer::create*Capturer` factory methods in `desktop_capturer.cpp` to instantiate the new classes based on compile-time OS defines.
