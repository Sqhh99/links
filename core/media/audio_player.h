#ifndef CORE_MEDIA_AUDIO_PLAYER_H
#define CORE_MEDIA_AUDIO_PLAYER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "audio_format.h"

namespace links {
namespace core {

/**
 * One remote participant's audio output.
 *
 * Owns device selection, format negotiation and the actual write. The caller
 * (MediaPipeline) owns resampling and the AEC reference tap, so the DSP stays
 * in core and only the device plumbing is platform-specific.
 */
class AudioPlayer {
public:
    virtual ~AudioPlayer() = default;

    /**
     * Pick a usable output format for the requested rate/channels: try
     * Int16 at exactly that rate, else fall back to the device's preferred
     * format. Returns an invalid format if no device is available.
     */
    virtual AudioFormat negotiate(int sampleRate, int channels) = 0;

    /// Open (or reopen) the device with `format`. Returns false if unavailable.
    virtual bool open(const AudioFormat& format) = 0;

    virtual bool isOpen() const = 0;
    virtual void write(const std::uint8_t* data, std::size_t bytes) = 0;
    virtual void close() = 0;

    /**
     * Identifies the device currently backing this player. MediaPipeline
     * compares it to detect a default-device change and reopen, which is what
     * the QAudioDevice inequality check did.
     */
    virtual std::string currentDeviceId() const = 0;
};

class AudioPlayerFactory {
public:
    virtual ~AudioPlayerFactory() = default;
    virtual std::unique_ptr<AudioPlayer> createPlayer() = 0;

    /// Identifies the current default output device, without opening it.
    virtual std::string defaultDeviceId() const = 0;
};

}  // namespace core
}  // namespace links

#endif  // CORE_MEDIA_AUDIO_PLAYER_H
