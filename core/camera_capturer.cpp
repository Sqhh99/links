#include "camera_capturer.h"

#include "base/log.h"
#include "base/strings.h"
#include "base/time.h"
#include "livekit/video_frame.h"

namespace core = links::core;

CameraCapturer::CameraCapturer(core::VideoInput& input)
    : input_(input)
{
    try {
        videoSource_ = std::make_shared<livekit::VideoSource>(640, 480);
        core::logInfo("VideoSource created for camera");
    } catch (const std::exception& e) {
        core::logError(core::str::cat("Failed to create VideoSource: ", e.what()));
        error.notify(core::str::cat("Failed to create video source: ", e.what()));
        return;
    }

    startTime_ = std::chrono::steady_clock::now();

    input_.setFrameCallback([this](const core::VideoFrame& frame) { onFrame(frame); });
    input_.setErrorCallback([this](const std::string& message) { error.notify(message); });
}

CameraCapturer::~CameraCapturer()
{
    stop();
}

bool CameraCapturer::start()
{
    if (isActive_) {
        return true;
    }
    if (!videoSource_) {
        return false;
    }

    if (!input_.start()) {
        return false;
    }

    isActive_ = true;
    core::logInfo("Camera started");
    return true;
}

void CameraCapturer::stop()
{
    if (!isActive_) {
        return;
    }
    input_.stop();
    isActive_ = false;
    core::logInfo(core::str::cat("Camera stopped (captured ", frameCount_, " frames)"));
}

bool CameraCapturer::setCameraById(const std::string& deviceId)
{
    if (isActive_) {
        core::logWarning("Cannot change camera while active");
        return false;
    }
    return input_.setDeviceId(deviceId);
}

void CameraCapturer::onFrame(const core::VideoFrame& frame)
{
    if (!isActive_ || !videoSource_ || !frame.isValid()) {
        return;
    }

    // Frame rate limiting - skip frames to maintain target FPS
    const std::int64_t currentTime = core::elapsedMs(startTime_);
    if (currentTime - lastFrameTime_ < minFrameIntervalMs_) {
        return;
    }
    lastFrameTime_ = currentTime;

    try {
        livekit::VideoFrame lkFrame(frame.width(), frame.height(),
                                    livekit::VideoBufferType::RGBA,
                                    frame.toPackedRgba());
        videoSource_->captureFrame(lkFrame, core::nowMsSinceEpoch() * 1000);

        frameCaptured.notify(frame);

        frameCount_++;

        // Log every 30 frames (about 1 second at 30fps)
        if (frameCount_ % 30 == 0) {
            core::logDebug(core::str::cat("Captured ", frameCount_, " frames (",
                                          frame.width(), "x", frame.height(), ")"));
        }
    } catch (const std::exception& e) {
        core::logError(core::str::cat("Failed to capture frame: ", e.what()));
    }
}
