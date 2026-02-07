#ifndef DESKTOP_CAPTURE_LINUX_X11_PLATFORM_WINDOW_OPS_LINUX_X11_H_
#define DESKTOP_CAPTURE_LINUX_X11_PLATFORM_WINDOW_OPS_LINUX_X11_H_

#ifdef __linux__

#include <optional>
#include <vector>

#include "../../../image_types.h"
#include "../../../window_types.h"

namespace links {
namespace core {
namespace linux_x11 {

bool isX11Session();
bool isWindowShareSupported();
bool isScreenShareSupported();

std::vector<WindowInfo> enumerateWindows();
bool bringWindowToForeground(WindowId id);
bool excludeFromCapture(WindowId id);
bool isWindowValid(WindowId id);
bool isWindowMinimized(WindowId id);

std::optional<RawImage> captureWindowWithX11(WindowId id);
std::optional<RawImage> captureRootScreenWithX11();

}  // namespace linux_x11
}  // namespace core
}  // namespace links

#endif  // __linux__
#endif  // DESKTOP_CAPTURE_LINUX_X11_PLATFORM_WINDOW_OPS_LINUX_X11_H_
