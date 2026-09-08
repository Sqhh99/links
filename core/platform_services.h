#ifndef CORE_PLATFORM_SERVICES_H
#define CORE_PLATFORM_SERVICES_H

#include "base/executor.h"
#include "base/timer.h"
#include "media/audio_input.h"
#include "media/audio_player.h"
#include "media/media_devices.h"
#include "media/video_input.h"

namespace links {
namespace core {

/**
 * Everything core needs from the host application, in one bundle.
 *
 * All pointers are non-owning and must outlive the ConferenceManager. The
 * `links` executable owns a single QtPlatformServices that supplies them.
 */
struct PlatformServices {
    TaskRunner* taskRunner{nullptr};
    BackgroundExecutor* background{nullptr};
    TimerFactory* timers{nullptr};
    MediaDeviceRegistry* devices{nullptr};
    VideoInput* camera{nullptr};
    AudioInput* microphone{nullptr};
    AudioPlayerFactory* audioPlayers{nullptr};

    bool valid() const
    {
        return taskRunner && background && timers && devices
            && camera && microphone && audioPlayers;
    }
};

}  // namespace core
}  // namespace links

#endif  // CORE_PLATFORM_SERVICES_H
