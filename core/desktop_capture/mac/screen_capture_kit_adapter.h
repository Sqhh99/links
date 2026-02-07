#ifndef DESKTOP_CAPTURE_MAC_SCREEN_CAPTURE_KIT_ADAPTER_H_
#define DESKTOP_CAPTURE_MAC_SCREEN_CAPTURE_KIT_ADAPTER_H_

#ifdef __APPLE__

#include <cstdint>
#include <optional>

#include "../../image_types.h"

namespace links {
namespace core {
namespace mac {

bool isScreenCaptureKitAvailable();
bool hasScreenRecordingPermission();

std::optional<RawImage> captureDisplayWithScreenCaptureKit(std::uint32_t displayId);

}  // namespace mac
}  // namespace core
}  // namespace links

#endif  // __APPLE__
#endif  // DESKTOP_CAPTURE_MAC_SCREEN_CAPTURE_KIT_ADAPTER_H_
