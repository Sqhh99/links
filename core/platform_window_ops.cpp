#include "platform_window_ops.h"

#ifdef _WIN32
#include "desktop_capture/win/platform_window_ops_win.h"
#elif defined(__APPLE__)
#include "desktop_capture/mac/platform_window_ops_mac.h"
#elif defined(__linux__)
#include "desktop_capture/linux/x11/platform_window_ops_linux_x11.h"
#endif

namespace links {
namespace core {

std::vector<WindowInfo> enumerateWindows()
{
#ifdef _WIN32
    return win::enumerateWindows();
#elif defined(__APPLE__)
    return mac::enumerateWindows();
#elif defined(__linux__)
    return linux_x11::enumerateWindows();
#else
    return {};
#endif
}

bool isWindowShareSupportedOnCurrentPlatform()
{
#ifdef _WIN32
    return true;
#elif defined(__APPLE__)
    return mac::isWindowShareSupported();
#elif defined(__linux__)
    return linux_x11::isWindowShareSupported();
#else
    return false;
#endif
}

bool isScreenShareSupportedOnCurrentPlatform()
{
#ifdef _WIN32
    return true;
#elif defined(__APPLE__)
    return mac::isScreenShareSupported();
#elif defined(__linux__)
    return linux_x11::isScreenShareSupported();
#else
    return false;
#endif
}

bool hasScreenCapturePermission()
{
#ifdef _WIN32
    return true;
#elif defined(__APPLE__)
    return mac::hasScreenRecordingPermission();
#elif defined(__linux__)
    return true;
#else
    return false;
#endif
}

bool bringWindowToForeground(WindowId id)
{
#ifdef _WIN32
    return win::bringWindowToForeground(id);
#elif defined(__APPLE__)
    return mac::bringWindowToForeground(id);
#elif defined(__linux__)
    return linux_x11::bringWindowToForeground(id);
#else
    (void)id;
    return false;
#endif
}

bool excludeFromCapture(WindowId id)
{
#ifdef _WIN32
    return win::excludeFromCapture(id);
#elif defined(__APPLE__)
    return mac::excludeFromCapture(id);
#elif defined(__linux__)
    return linux_x11::excludeFromCapture(id);
#else
    (void)id;
    return false;
#endif
}

bool isWindowValid(WindowId id)
{
#ifdef _WIN32
    return win::isWindowValid(id);
#elif defined(__APPLE__)
    return mac::isWindowValid(id);
#elif defined(__linux__)
    return linux_x11::isWindowValid(id);
#else
    return id != 0;
#endif
}

bool isWindowMinimized(WindowId id)
{
#ifdef _WIN32
    return win::isWindowMinimized(id);
#elif defined(__APPLE__)
    return mac::isWindowMinimized(id);
#elif defined(__linux__)
    return linux_x11::isWindowMinimized(id);
#else
    (void)id;
    return false;
#endif
}

std::optional<RawImage> captureWindowWithWinRt(WindowId id)
{
#ifdef _WIN32
    return win::captureWindowWithWinRt(id);
#else
    (void)id;
    return std::nullopt;
#endif
}

std::optional<RawImage> captureWindowWithPrintApi(WindowId id)
{
#ifdef _WIN32
    return win::captureWindowWithPrintApi(id);
#elif defined(__APPLE__)
    return mac::captureWindowWithCoreGraphics(id);
#elif defined(__linux__)
    return linux_x11::captureWindowWithX11(id);
#else
    (void)id;
    return std::nullopt;
#endif
}

}  // namespace core
}  // namespace links
