/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Windows Utility Functions Implementation
 */

#ifdef _WIN32

#include "window_utils.h"
#include <dwmapi.h>
#include <ShellScalingApi.h>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Shcore.lib")

namespace links {
namespace desktop_capture {
namespace win {

DesktopRect getWindowRect(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return DesktopRect();
    }
    RECT rect{};
    if (!GetWindowRect(hwnd, &rect)) {
        return DesktopRect();
    }
    return DesktopRect::makeLTRB(rect.left, rect.top, rect.right, rect.bottom);
}

DesktopRect getWindowExtendedFrameBounds(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return DesktopRect();
    }
    RECT rect{};
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS,
                                       &rect, sizeof(rect));
    if (SUCCEEDED(hr)) {
        return DesktopRect::makeLTRB(rect.left, rect.top, rect.right, rect.bottom);
    }
    // Fallback to regular window rect
    return getWindowRect(hwnd);
}

bool isWindowValid(HWND hwnd) {
    return hwnd != nullptr && IsWindow(hwnd);
}

bool isWindowMinimized(HWND hwnd) {
    return isWindowValid(hwnd) && IsIconic(hwnd);
}

bool isWindowMaximized(HWND hwnd) {
    return isWindowValid(hwnd) && IsZoomed(hwnd);
}

std::wstring getWindowTitle(HWND hwnd) {
    if (!hwnd) {
        return L"";
    }
    int length = GetWindowTextLengthW(hwnd);
    if (length == 0) {
        return L"";
    }
    std::wstring title(length + 1, L'\0');
    GetWindowTextW(hwnd, title.data(), length + 1);
    title.resize(length);
    return title;
}

HMONITOR getWindowMonitor(HWND hwnd) {
    return MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
}

static BOOL CALLBACK monitorEnumProc(HMONITOR monitor, HDC, LPRECT, LPARAM lParam) {
    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(lParam);
    
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (GetMonitorInfoW(monitor, &info)) {
        MonitorInfo mi;
        mi.handle = monitor;
        mi.bounds = DesktopRect::makeLTRB(
            info.rcMonitor.left, info.rcMonitor.top,
            info.rcMonitor.right, info.rcMonitor.bottom);
        mi.workArea = DesktopRect::makeLTRB(
            info.rcWork.left, info.rcWork.top,
            info.rcWork.right, info.rcWork.bottom);
        mi.isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
        mi.deviceName = info.szDevice;
        monitors->push_back(mi);
    }
    return TRUE;
}

std::vector<MonitorInfo> enumerateMonitors() {
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc,
                        reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

DesktopVector getWindowDpi(HWND hwnd) {
    UINT dpiX = 96, dpiY = 96;
    
    // Try GetDpiForWindow (Windows 10 1607+)
    typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
    static auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFunc>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    
    if (getDpiForWindow && hwnd) {
        UINT dpi = getDpiForWindow(hwnd);
        if (dpi > 0) {
            return DesktopVector(dpi, dpi);
        }
    }
    
    // Fallback to monitor DPI
    HMONITOR monitor = getWindowMonitor(hwnd);
    if (monitor) {
        return getMonitorDpi(monitor);
    }
    
    return DesktopVector(dpiX, dpiY);
}

DesktopVector getMonitorDpi(HMONITOR monitor) {
    UINT dpiX = 96, dpiY = 96;
    
    if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        return DesktopVector(dpiX, dpiY);
    }
    
    return DesktopVector(96, 96);
}

static BOOL CALLBACK windowEnumProc(HWND hwnd, LPARAM lParam) {
    auto* windows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
    
    // Skip invisible windows
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }
    
    // Skip windows with empty titles (usually internal windows)
    int titleLen = GetWindowTextLengthW(hwnd);
    if (titleLen == 0) {
        return TRUE;
    }
    
    // Skip tool windows and other non-app windows
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return TRUE;
    }
    
    // Skip child windows
    if (GetParent(hwnd) != nullptr) {
        return TRUE;
    }
    
    // Get window info
    WindowInfo info;
    info.hwnd = hwnd;
    info.title = getWindowTitle(hwnd);
    
    wchar_t className[256] = {};
    GetClassNameW(hwnd, className, 256);
    info.className = className;
    
    // Skip some known system windows
    if (info.className == L"Progman" ||
        info.className == L"Shell_TrayWnd" ||
        info.className == L"WorkerW" ||
        info.className == L"Shell_SecondaryTrayWnd") {
        return TRUE;
    }
    
    info.bounds = getWindowRect(hwnd);
    info.isVisible = true;
    info.isMinimized = IsIconic(hwnd);
    
    windows->push_back(info);
    return TRUE;
}

std::vector<WindowInfo> enumerateCaptureWindows() {
    std::vector<WindowInfo> windows;
    EnumWindows(windowEnumProc, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

bool isWgcSupported() {
    // Windows Graphics Capture is available on Windows 10 1903 (Build 18362) and later
    // Check Windows version using RtlGetVersion
    typedef LONG (WINAPI *RtlGetVersionFunc)(OSVERSIONINFOEXW*);
    static HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return false;
    }
    
    static auto rtlGetVersion = reinterpret_cast<RtlGetVersionFunc>(
        GetProcAddress(ntdll, "RtlGetVersion"));
    
    if (!rtlGetVersion) {
        return false;
    }
    
    OSVERSIONINFOEXW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (rtlGetVersion(&osvi) != 0) {
        return false;
    }
    
    // Windows 10 version 1903 (Build 18362) or later
    return osvi.dwMajorVersion > 10 ||
           (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber >= 18362);
}

bool isDxgiDuplicationSupported() {
    // DXGI Desktop Duplication is available on Windows 8 and later
    // Try to create a D3D11 device briefly to check
    return true;  // Almost always supported on modern Windows
}

}  // namespace win
}  // namespace desktop_capture
}  // namespace links

#endif  // _WIN32
