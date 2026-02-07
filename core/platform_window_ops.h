#ifndef CORE_PLATFORM_WINDOW_OPS_H
#define CORE_PLATFORM_WINDOW_OPS_H

#include <optional>
#include <vector>
#include "image_types.h"
#include "window_types.h"

namespace links {
namespace core {

std::vector<WindowInfo> enumerateWindows();
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
