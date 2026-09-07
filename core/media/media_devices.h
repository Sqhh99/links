#ifndef CORE_MEDIA_MEDIA_DEVICES_H
#define CORE_MEDIA_MEDIA_DEVICES_H

#include <string>
#include <vector>

namespace links {
namespace core {

struct MediaDeviceInfo {
    /// Opaque, platform-defined, and persisted in settings. On Windows this is
    /// a device path whose bytes are not necessarily valid UTF-8, so it is
    /// carried through verbatim and never re-encoded.
    std::string id;
    std::string label;  // for display, UTF-8
    bool isDefault{false};
};

class MediaDeviceRegistry {
public:
    virtual ~MediaDeviceRegistry() = default;
    virtual std::vector<MediaDeviceInfo> cameras() const = 0;
    virtual std::vector<MediaDeviceInfo> microphones() const = 0;
    virtual std::vector<MediaDeviceInfo> speakers() const = 0;
};

}  // namespace core
}  // namespace links

#endif  // CORE_MEDIA_MEDIA_DEVICES_H
