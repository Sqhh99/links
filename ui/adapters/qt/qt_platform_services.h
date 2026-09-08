#ifndef QT_PLATFORM_SERVICES_H
#define QT_PLATFORM_SERVICES_H

#include <QObject>

#include <memory>

#include "core/conference/device_config.h"
#include "core/platform_services.h"
#include "qt_audio_input.h"
#include "qt_audio_player.h"
#include "qt_media_devices.h"
#include "qt_task_runner.h"
#include "qt_timer_factory.h"
#include "qt_video_input.h"

namespace links {
namespace qt_adapter {

/**
 * Owns every Qt-backed implementation of a core port and hands core a
 * PlatformServices view of them.
 *
 * One instance per conference window, created before the ConferenceManager and
 * destroyed after it -- core holds non-owning pointers into this object.
 */
class QtPlatformServices : public QObject {
    Q_OBJECT
public:
    explicit QtPlatformServices(QObject* parent = nullptr);

    const core::PlatformServices& services() const { return services_; }

    /// Reads the current Settings into the plain structs core consumes.
    static core::DeviceSelection readDeviceSelection();
    static core::AudioProcessingConfig readAudioConfig();

private:
    QtTaskRunner taskRunner_;
    QtBackgroundExecutor background_;
    QtTimerFactory timers_;
    QtMediaDeviceRegistry devices_;
    QtVideoInput camera_;
    QtAudioInput microphone_;
    QtAudioPlayerFactory audioPlayers_;

    core::PlatformServices services_;
};

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_PLATFORM_SERVICES_H
