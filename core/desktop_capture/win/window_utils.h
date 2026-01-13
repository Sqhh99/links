/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Windows Utility Functions
 */

#ifndef DESKTOP_CAPTURE_WIN_WINDOW_UTILS_H_
#define DESKTOP_CAPTURE_WIN_WINDOW_UTILS_H_

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "../desktop_geometry.h"

namespace links {
namespace desktop_capture {
namespace win {

// Get the bounding rectangle of a window
DesktopRect getWindowRect(HWND hwnd);

// Get the extended frame bounds (excludes invisible window borders on Win10+)
DesktopRect getWindowExtendedFrameBounds(HWND hwnd);

// Check if a window handle is valid
bool isWindowValid(HWND hwnd);

// Check if a window is minimized
bool isWindowMinimized(HWND hwnd);

// Check if a window is maximized
bool isWindowMaximized(HWND hwnd);

// Get the window title
std::wstring getWindowTitle(HWND hwnd);

// Get the monitor containing the largest portion of the window
HMONITOR getWindowMonitor(HWND hwnd);

// Get monitor information
struct MonitorInfo {
    HMONITOR handle;
    DesktopRect bounds;
    DesktopRect workArea;
    bool isPrimary;
    std::wstring deviceName;
};

// Enumerate all monitors
std::vector<MonitorInfo> enumerateMonitors();

// Get DPI for a specific window
DesktopVector getWindowDpi(HWND hwnd);

// Get DPI for a specific monitor
DesktopVector getMonitorDpi(HMONITOR monitor);

// Structure for enumerated windows
struct WindowInfo {
    HWND hwnd;
    std::wstring title;
    std::wstring className;
    DesktopRect bounds;
    bool isVisible;
    bool isMinimized;
};

// Enumerate all visible windows suitable for capture
std::vector<WindowInfo> enumerateCaptureWindows();

// Check if Windows Graphics Capture API is available
bool isWgcSupported();

// Check if DXGI Desktop Duplication is supported
bool isDxgiDuplicationSupported();

}  // namespace win
}  // namespace desktop_capture
}  // namespace links

#endif  // _WIN32
#endif  // DESKTOP_CAPTURE_WIN_WINDOW_UTILS_H_
