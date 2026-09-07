#include "qt_media_devices.h"

#include <QAudioDevice>
#include <QCameraDevice>
#include <QMediaDevices>

namespace links {
namespace qt_adapter {
namespace {

template <typename DeviceList>
std::vector<core::MediaDeviceInfo> convert(const DeviceList& devices,
                                           const QByteArray& defaultId)
{
    std::vector<core::MediaDeviceInfo> out;
    out.reserve(static_cast<std::size_t>(devices.size()));
    for (const auto& device : devices) {
        core::MediaDeviceInfo info;
        info.id = device.id().toStdString();
        info.label = device.description().toStdString();
        info.isDefault = (device.id() == defaultId);
        out.push_back(std::move(info));
    }
    return out;
}

}  // namespace

std::vector<core::MediaDeviceInfo> QtMediaDeviceRegistry::cameras() const
{
    return convert(QMediaDevices::videoInputs(), QMediaDevices::defaultVideoInput().id());
}

std::vector<core::MediaDeviceInfo> QtMediaDeviceRegistry::microphones() const
{
    return convert(QMediaDevices::audioInputs(), QMediaDevices::defaultAudioInput().id());
}

std::vector<core::MediaDeviceInfo> QtMediaDeviceRegistry::speakers() const
{
    return convert(QMediaDevices::audioOutputs(), QMediaDevices::defaultAudioOutput().id());
}

}  // namespace qt_adapter
}  // namespace links
