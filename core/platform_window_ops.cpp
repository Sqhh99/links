#include "platform_window_ops.h"

#ifdef _WIN32
#include "desktop_capture/win/platform_window_ops_win.h"
#endif

namespace links {
namespace core {

std::vector<WindowInfo> enumerateWindows()
{
#ifdef _WIN32
    return win::enumerateWindows();
#else
    return {};
#endif
}

bool bringWindowToForeground(WindowId id)
{
#ifdef _WIN32
    return win::bringWindowToForeground(id);
#else
    (void)id;
    return false;
#endif
}

bool excludeFromCapture(WindowId id)
{
#ifdef _WIN32
    return win::excludeFromCapture(id);
#else
    (void)id;
    return false;
#endif
}

bool isWindowValid(WindowId id)
{
#ifdef _WIN32
    return win::isWindowValid(id);
#else
    return id != 0;
#endif
}

bool isWindowMinimized(WindowId id)
{
#ifdef _WIN32
    return win::isWindowMinimized(id);
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
#else
    (void)id;
    return std::nullopt;
#endif
}

}  // namespace core
}  // namespace links
