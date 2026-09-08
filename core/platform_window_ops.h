#ifndef CORE_PLATFORM_WINDOW_OPS_H
#define CORE_PLATFORM_WINDOW_OPS_H

#include <optional>
#include <vector>
#include "image_types.h"
#include "window_types.h"

namespace links {
namespace core {

std::vector<WindowInfo> enumerateWindows();

/// Monitors as the capture backends see them. ui/ matches a QScreen against
/// these to obtain a MonitorId, which keeps QScreen out of core.
std::vector<MonitorInfo> enumerateMonitors();
bool isWindowShareSupportedOnCurrentPlatform();
bool isScreenShareSupportedOnCurrentPlatform();
bool hasScreenCapturePermission();
bool bringWindowToForeground(WindowId id);
bool excludeFromCapture(WindowId id);
bool isWindowValid(WindowId id);
bool isWindowMinimized(WindowId id);
std::optional<RawImage> captureWindowWithWinRt(WindowId id);
std::optional<RawImage> captureWindowWithPrintApi(WindowId id);

}  // namespace core
}  // namespace links

#endif  // CORE_PLATFORM_WINDOW_OPS_H
