#ifndef CORE_MEDIA_VIDEO_INPUT_H
#define CORE_MEDIA_VIDEO_INPUT_H

#include <functional>
#include <string>

#include "video_frame.h"

namespace links {
namespace core {

/**
 * A camera, reduced to what core needs from it.
 *
 * Implementations deliver RGBA8888 frames on the main thread. Everything
 * above this line -- frame pacing, the LiveKit push, the frame counter --
 * stays in core::CameraCapturer.
 */
class VideoInput {
public:
    virtual ~VideoInput() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isActive() const = 0;

    /// Must be called before start(). Empty id means "system default".
    virtual bool setDeviceId(const std::string& deviceId) = 0;

    virtual void setFrameCallback(std::function<void(const VideoFrame&)> callback) = 0;
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};

}  // namespace core
}  // namespace links

#endif  // CORE_MEDIA_VIDEO_INPUT_H
