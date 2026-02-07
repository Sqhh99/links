#ifndef DESKTOP_CAPTURE_MAC_PLATFORM_WINDOW_OPS_MAC_H_
#define DESKTOP_CAPTURE_MAC_PLATFORM_WINDOW_OPS_MAC_H_

#ifdef __APPLE__

#include <cstdint>
#include <optional>
#include <vector>

#include "../../image_types.h"
#include "../../window_types.h"

namespace links {
namespace core {
namespace mac {

bool isWindowShareSupported();
bool isScreenShareSupported();
bool hasScreenRecordingPermission();

std::vector<std::uint32_t> enumerateDisplays();
std::vector<WindowInfo> enumerateWindows();

bool bringWindowToForeground(WindowId id);
bool excludeFromCapture(WindowId id);
bool isWindowValid(WindowId id);
bool isWindowMinimized(WindowId id);

std::optional<RawImage> captureWindowWithCoreGraphics(WindowId id);
std::optional<RawImage> captureDisplayWithCoreGraphics(std::uint32_t displayId);

}  // namespace mac
}  // namespace core
}  // namespace links

#endif  // __APPLE__
#endif  // DESKTOP_CAPTURE_MAC_PLATFORM_WINDOW_OPS_MAC_H_
