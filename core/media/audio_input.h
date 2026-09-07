#ifndef CORE_MEDIA_AUDIO_INPUT_H
#define CORE_MEDIA_AUDIO_INPUT_H

#include <cstdint>
#include <functional>
#include <string>

#include "audio_format.h"

namespace links {
namespace core {

/**
 * A microphone, reduced to raw PCM delivery.
 *
 * The 10 ms framing, the WebRTC AudioProcessingModule and the LiveKit push all
 * stay in core::MicrophoneCapturer -- only device plumbing lives behind this.
 */
class AudioInput {
public:
    virtual ~AudioInput() = default;

    /// Requests 48 kHz mono int16, matching the current fixed format.
    virtual bool start(const AudioFormat& format) = 0;
    virtual void stop() = 0;
    virtual bool isActive() const = 0;

    /// Must be called before start(). Empty id means "system default".
    virtual void setDeviceId(const std::string& deviceId) = 0;

    /// Interleaved int16 PCM; `sampleCount` counts samples, not frames.
    virtual void setDataCallback(
        std::function<void(const std::int16_t*, std::size_t)> callback) = 0;
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};

}  // namespace core
}  // namespace links

#endif  // CORE_MEDIA_AUDIO_INPUT_H
