#ifndef CAMERA_CAPTURER_H
#define CAMERA_CAPTURER_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "base/signal.h"
#include "livekit/video_source.h"
#include "media/video_input.h"

/**
 * Drives a VideoInput and publishes its frames to LiveKit.
 *
 * The Qt Multimedia machinery that used to live here (QCamera,
 * QMediaCaptureSession, QVideoSink) is now behind links::core::VideoInput,
 * implemented in ui/adapters/qt. Frame pacing, the frame counter and the
 * LiveKit push stayed put.
 */
class CameraCapturer
{
public:
    explicit CameraCapturer(links::core::VideoInput& input);
    ~CameraCapturer();

    bool start();
    void stop();
    bool isActive() const { return isActive_; }

    std::shared_ptr<livekit::VideoSource> getVideoSource() const { return videoSource_; }

    /// Must be called before start(). Empty id selects the system default.
    bool setCameraById(const std::string& deviceId);

    void setTargetFps(int fps) { targetFps_ = fps; minFrameIntervalMs_ = 1000 / fps; }
    int getTargetFps() const { return targetFps_; }

    links::core::Signal<const links::core::VideoFrame&> frameCaptured;
    links::core::Signal<const std::string&> error;

private:
    void onFrame(const links::core::VideoFrame& frame);

    links::core::VideoInput& input_;
    std::shared_ptr<livekit::VideoSource> videoSource_;

    bool isActive_{false};
    int frameCount_{0};

    int targetFps_{30};
    int minFrameIntervalMs_{33};  // ~30fps
    std::chrono::steady_clock::time_point startTime_;
    std::int64_t lastFrameTime_{0};
};

#endif // CAMERA_CAPTURER_H
