/*
 * Copyright (c) 2026 Links Project
 * Screen Capturer implementation.
 */

#include "screen_capturer.h"

#include <algorithm>

#include "base/log.h"
#include "base/strings.h"
#include "base/time.h"
#include "livekit/video_frame.h"
#include "platform_window_ops.h"

using namespace links::desktop_capture;
namespace core = links::core;

ScreenCapturer::ScreenCapturer(core::TimerFactory& timers)
    : timers_(timers),
      videoSource_(std::make_shared<livekit::VideoSource>(1280, 720))
{
    lastFrameTime_ = std::chrono::steady_clock::now();
}

ScreenCapturer::~ScreenCapturer()
{
    stop();
}

void ScreenCapturer::setScreen(core::MonitorId monitorId)
{
    monitorId_ = monitorId;
    windowId_ = 0;
}

void ScreenCapturer::setWindow(core::WindowId windowId)
{
    windowId_ = windowId;
    monitorId_ = 0;
}

bool ScreenCapturer::initCapturer()
{
    CaptureOptions options;
    options.targetFps = fps_;
    options.stallTimeoutMs = stallRecoverMs_;

    if (mode_ == Mode::Window) {
        capturer_ = DesktopCapturer::createWindowCapturer(options);
        if (capturer_ && windowId_ != 0) {
            capturer_->selectSource(static_cast<DesktopCapturer::SourceId>(windowId_));
        }
    } else {
        capturer_ = DesktopCapturer::createScreenCapturer(options);
        if (capturer_) {
            auto sourceId = screenSourceId();
            if (sourceId == 0 && monitorId_ != 0) {
                core::logWarning("Selected screen not found, falling back to primary");
            }
            // Source 0 means primary screen when no specific monitor is resolved
            capturer_->selectSource(sourceId);
        }
    }

    if (!capturer_) {
        core::logError("Failed to create desktop capturer");
        return false;
    }

    capturer_->start(this);
    return true;
}

bool ScreenCapturer::start()
{
    if (isActive_) {
        return true;
    }

    if (mode_ == Mode::Window && !validateWindowHandle()) {
        error.notify("No valid window selected for capture");
        return false;
    }

    // A monitorId_ of 0 already means "primary" to the capture backends, so
    // there is nothing to resolve here -- the caller in ui/ picks the default
    // screen. That is what removed QGuiApplication from core.

    if (!initCapturer()) {
        return false;
    }

    consecutiveFailures_ = 0;
    lastFrameTime_ = std::chrono::steady_clock::now();

    timer_ = timers_.createTimer([this]() { captureOnce(); });
    timer_->start(std::chrono::milliseconds(1000 / fps_), /*repeat=*/true);

    isActive_ = true;
    const char* modeName = mode_ == Mode::Window ? "window" : "screen";
    core::logInfo(core::str::cat("Screen capture started (", modeName, ")"));
    return true;
}

void ScreenCapturer::stop()
{
    if (!isActive_) {
        return;
    }

    // Destroying the timer cancels it; no queued tick can arrive afterwards.
    timer_.reset();

    if (capturer_) {
        capturer_->stop();
        capturer_.reset();
    }

    consecutiveFailures_ = 0;
    isActive_ = false;
    core::logInfo("Screen capture stopped");
}

void ScreenCapturer::captureOnce()
{
    if (!isActive_ || !capturer_) {
        return;
    }

    // Handle minimized windows
    if (mode_ == Mode::Window && isWindowMinimized()) {
        if (lastValidFrame_.isValid()) {
            frameCaptured.notify(lastValidFrame_);
            try {
                livekit::VideoFrame frame(lastValidFrame_.width(), lastValidFrame_.height(),
                    livekit::VideoBufferType::RGBA, lastValidFrame_.toPackedRgba());
                videoSource_->captureFrame(frame, core::nowMsSinceEpoch() * 1000);
            } catch (const std::exception& e) {
                core::logError(core::str::cat("Failed to emit cached frame: ", e.what()));
            }
        }
        return;
    }

    // Check for stall
    auto now = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastFrameTime_).count();

    if (elapsedMs > stallRecoverMs_) {
        core::logWarning(core::str::cat("Capture stalled for ", elapsedMs, " ms, reinitializing"));
        capturer_->stop();
        initCapturer();
        lastFrameTime_ = std::chrono::steady_clock::now();
        consecutiveFailures_ = 0;
    }

    // Request a frame
    capturer_->captureFrame();
}

void ScreenCapturer::onCaptureResult(DesktopCapturer::Result result,
                                      std::unique_ptr<DesktopFrame> frame)
{
    if (result == DesktopCapturer::Result::SUCCESS && frame) {
        consecutiveFailures_ = 0;
        lastFrameTime_ = std::chrono::steady_clock::now();

        // Straight from the capture buffer into a shared frame. The previous
        // code went DesktopFrame -> QImage -> std::vector, i.e. one copy more.
        const core::VideoFrame image = core::VideoFrame::copyFrom(
            frame->data(), frame->width(), frame->height(),
            frame->stride(), core::PixelFormat::RGBA8888);
        if (!image.isValid()) {
            core::logWarning("Failed to convert captured frame");
            return;
        }

        lastValidFrame_ = image;
        frameCaptured.notify(image);

        try {
            livekit::VideoFrame lkFrame(image.width(), image.height(),
                livekit::VideoBufferType::RGBA, image.toPackedRgba());
            videoSource_->captureFrame(lkFrame, core::nowMsSinceEpoch() * 1000);
        } catch (const std::exception& e) {
            core::logError(core::str::cat("Failed to submit frame to video source: ", e.what()));
        }
    } else if (result == DesktopCapturer::Result::ERROR_PERMANENT) {
        core::logError("Permanent capture error");
        if (mode_ == Mode::Window && !validateWindowHandle()) {
            error.notify("窗口已关闭，停止共享");
            stop();
        }
    } else {
        // Temporary error
        consecutiveFailures_++;
        if (consecutiveFailures_ >= 10) {
            core::logWarning("Too many consecutive capture failures");
            // Try to reinitialize
            capturer_->stop();
            if (!initCapturer()) {
                error.notify("Failed to reinitialize capture");
                stop();
            }
            consecutiveFailures_ = 0;
        }
    }
}

bool ScreenCapturer::validateWindowHandle() const
{
    if (windowId_ == 0) {
        return false;
    }
    return links::core::isWindowValid(static_cast<links::core::WindowId>(windowId_));
}

bool ScreenCapturer::isWindowMinimized() const
{
    if (windowId_ == 0) {
        return false;
    }
    return links::core::isWindowMinimized(static_cast<links::core::WindowId>(windowId_));
}

DesktopCapturer::SourceId ScreenCapturer::screenSourceId() const
{
    // monitorId_ is already the backend-native handle (HMONITOR on Windows),
    // resolved by ui/ from the QScreen the user picked.
    return static_cast<DesktopCapturer::SourceId>(monitorId_);
}
