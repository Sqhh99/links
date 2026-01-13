/*
 * Copyright (c) 2026 Links Project
 * Screen Capturer - Qt Integration Layer
 * 
 * This class provides a Qt-friendly wrapper around the desktop_capture module,
 * integrating with LiveKit video sources and Qt signals/slots.
 */

#ifndef SCREEN_CAPTURER_H
#define SCREEN_CAPTURER_H

#include <QObject>
#include <QScreen>
#include <QTimer>
#include <QImage>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include "livekit/video_source.h"
#include "desktop_capture/desktop_capturer.h"

class ScreenCapturer : public QObject, public links::desktop_capture::DesktopCapturer::Callback
{
    Q_OBJECT
public:
    enum class Mode {
        Screen,
        Window
    };

    explicit ScreenCapturer(QObject* parent = nullptr);
    ~ScreenCapturer() override;

    // Start/stop capture
    bool start();
    void stop();
    bool isActive() const { return isActive_; }

    // Mode and target selection
    void setMode(Mode mode) { mode_ = mode; }
    void setScreen(QScreen* screen);
    void setWindow(WId windowId);

    // Get the LiveKit video source
    std::shared_ptr<livekit::VideoSource> getVideoSource() const { return videoSource_; }

    // DesktopCapturer::Callback interface
    void onCaptureResult(links::desktop_capture::DesktopCapturer::Result result,
                         std::unique_ptr<links::desktop_capture::DesktopFrame> frame) override;

signals:
    void frameCaptured(const QImage& image);
    void error(const QString& message);

private slots:
    void captureOnce();

private:
    bool initCapturer();
    bool validateWindowHandle() const;
    bool isWindowMinimized() const;
    QImage frameToQImage(const links::desktop_capture::DesktopFrame& frame);

    std::shared_ptr<livekit::VideoSource> videoSource_;
    std::unique_ptr<links::desktop_capture::DesktopCapturer> capturer_;
    QScreen* screen_{nullptr};
    WId windowId_{0};
    std::unique_ptr<QTimer> timer_;
    QImage lastValidFrame_;

    std::atomic<bool> isActive_{false};
    Mode mode_{Mode::Screen};
    int fps_{15};
    std::chrono::steady_clock::time_point lastFrameTime_;
    int stallRecoverMs_{5000};
    int consecutiveFailures_{0};
    std::mutex mutex_;
};

#endif // SCREEN_CAPTURER_H
