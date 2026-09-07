#ifndef QT_MEDIA_DEVICES_H
#define QT_MEDIA_DEVICES_H

#include "core/media/media_devices.h"

namespace links {
namespace qt_adapter {

/// MediaDeviceRegistry backed by QMediaDevices.
class QtMediaDeviceRegistry : public core::MediaDeviceRegistry {
public:
    std::vector<core::MediaDeviceInfo> cameras() const override;
    std::vector<core::MediaDeviceInfo> microphones() const override;
    std::vector<core::MediaDeviceInfo> speakers() const override;
};

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_MEDIA_DEVICES_H
