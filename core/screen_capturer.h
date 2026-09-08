/*
 * Copyright (c) 2026 Links Project
 * Screen Capturer - LiveKit integration over the platform-agnostic
 * desktop_capture backends.
 */

#ifndef SCREEN_CAPTURER_H
#define SCREEN_CAPTURER_H

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include "base/signal.h"
#include "base/timer.h"
#include "desktop_capture/desktop_capturer.h"
#include "livekit/video_source.h"
#include "media/video_frame.h"
#include "window_types.h"

class ScreenCapturer : public links::desktop_capture::DesktopCapturer::Callback
{
public:
    enum class Mode {
        Screen,
        Window
    };

    explicit ScreenCapturer(links::core::TimerFactory& timers);
    ~ScreenCapturer() override;

    // Start/stop capture
    bool start();
    void stop();
    bool isActive() const { return isActive_; }

    // Mode and target selection
    void setMode(Mode mode) { mode_ = mode; }

    /// Select a monitor by its core MonitorId. ui/ resolves a QScreen to one of
    /// these via links::core::enumerateMonitors(); 0 means the primary screen.
    void setScreen(links::core::MonitorId monitorId);
    void setWindow(links::core::WindowId windowId);

    // Get the LiveKit video source
    std::shared_ptr<livekit::VideoSource> getVideoSource() const { return videoSource_; }

    // DesktopCapturer::Callback interface
    void onCaptureResult(links::desktop_capture::DesktopCapturer::Result result,
                         std::unique_ptr<links::desktop_capture::DesktopFrame> frame) override;

    links::core::Signal<const links::core::VideoFrame&> frameCaptured;
    links::core::Signal<const std::string&> error;

private:
    void captureOnce();
    bool initCapturer();
    bool validateWindowHandle() const;
    bool isWindowMinimized() const;
    links::desktop_capture::DesktopCapturer::SourceId screenSourceId() const;

    links::core::TimerFactory& timers_;
    std::shared_ptr<livekit::VideoSource> videoSource_;
    std::unique_ptr<links::desktop_capture::DesktopCapturer> capturer_;
    links::core::MonitorId monitorId_{0};
    links::core::WindowId windowId_{0};
    std::unique_ptr<links::core::Timer> timer_;
    links::core::VideoFrame lastValidFrame_;

    std::atomic<bool> isActive_{false};
    Mode mode_{Mode::Screen};
    int fps_{15};
    std::chrono::steady_clock::time_point lastFrameTime_;
    int stallRecoverMs_{5000};
    int consecutiveFailures_{0};
    std::mutex mutex_;
};

#endif // SCREEN_CAPTURER_H
