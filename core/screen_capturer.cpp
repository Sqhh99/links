/*
 * Copyright (c) 2026 Links Project
 * Screen Capturer - Qt Integration Layer Implementation
 */

#include "screen_capturer.h"
#include "../utils/logger.h"
#include "livekit/video_frame.h"
#include "platform_window_ops.h"
#include <QGuiApplication>
#include <QDateTime>
#include <algorithm>

#ifdef Q_OS_WIN
#include "desktop_capture/win/window_utils.h"
#endif

using namespace links::desktop_capture;

ScreenCapturer::ScreenCapturer(QObject* parent)
    : QObject(parent),
      videoSource_(std::make_shared<livekit::VideoSource>(1280, 720))
{
    lastFrameTime_ = std::chrono::steady_clock::now();
}

ScreenCapturer::~ScreenCapturer()
{
    stop();
}

void ScreenCapturer::setScreen(QScreen* screen)
{
    screen_ = screen;
    windowId_ = 0;
}

void ScreenCapturer::setWindow(WId windowId)
{
    windowId_ = windowId;
    screen_ = nullptr;
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
            if (sourceId == 0 && screen_) {
                Logger::instance().warning("Selected screen not found, falling back to primary");
            }
            // Source 0 means primary screen when no specific monitor is resolved
            capturer_->selectSource(sourceId);
        }
    }

    if (!capturer_) {
        Logger::instance().error("Failed to create desktop capturer");
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
        emit error("No valid window selected for capture");
        return false;
    }

    if (mode_ == Mode::Screen && !screen_) {
        screen_ = QGuiApplication::primaryScreen();
    }

    if (!initCapturer()) {
        return false;
    }

    consecutiveFailures_ = 0;
    lastFrameTime_ = std::chrono::steady_clock::now();

    timer_ = std::make_unique<QTimer>(this);
    timer_->setInterval(1000 / fps_);
    connect(timer_.get(), &QTimer::timeout, this, &ScreenCapturer::captureOnce);
    timer_->start();

    isActive_ = true;
    const char* modeName = mode_ == Mode::Window ? "window" : "screen";
    Logger::instance().info(QString("Screen capture started (%1)").arg(modeName));
    return true;
}

void ScreenCapturer::stop()
{
    if (!isActive_) {
        return;
    }

    if (timer_) {
        timer_->stop();
    }
    timer_.reset();

    if (capturer_) {
        capturer_->stop();
        capturer_.reset();
    }

    consecutiveFailures_ = 0;
    isActive_ = false;
    Logger::instance().info("Screen capture stopped");
}

void ScreenCapturer::captureOnce()
{
    if (!isActive_ || !capturer_) {
        return;
    }

    // Handle minimized windows
    if (mode_ == Mode::Window && isWindowMinimized()) {
        if (!lastValidFrame_.isNull()) {
            emit frameCaptured(lastValidFrame_);
            try {
                std::vector<uint8_t> frameData(lastValidFrame_.constBits(),
                    lastValidFrame_.constBits() + lastValidFrame_.sizeInBytes());
                livekit::VideoFrame frame(lastValidFrame_.width(), lastValidFrame_.height(),
                    livekit::VideoBufferType::RGBA, std::move(frameData));
                videoSource_->captureFrame(frame, QDateTime::currentMSecsSinceEpoch() * 1000);
            } catch (const std::exception& e) {
                Logger::instance().error(QString("Failed to emit cached frame: %1").arg(e.what()));
            }
        }
        return;
    }

    // Check for stall
    auto now = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastFrameTime_).count();

    if (elapsedMs > stallRecoverMs_) {
        Logger::instance().warning(QString("Capture stalled for %1 ms, reinitializing").arg(elapsedMs));
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

        QImage image = frameToQImage(*frame);
        if (image.isNull()) {
            Logger::instance().warning("Failed to convert frame to QImage");
            return;
        }

        lastValidFrame_ = image;
        emit frameCaptured(image);

        try {
            std::vector<uint8_t> frameData(image.constBits(),
                image.constBits() + image.sizeInBytes());
            livekit::VideoFrame lkFrame(image.width(), image.height(),
                livekit::VideoBufferType::RGBA, std::move(frameData));
            videoSource_->captureFrame(lkFrame, QDateTime::currentMSecsSinceEpoch() * 1000);
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Failed to submit frame to video source: %1").arg(e.what()));
        }
    } else if (result == DesktopCapturer::Result::ERROR_PERMANENT) {
        Logger::instance().error("Permanent capture error");
        if (mode_ == Mode::Window && !validateWindowHandle()) {
            emit error("窗口已关闭，停止共享");
            stop();
        }
    } else {
        // Temporary error
        consecutiveFailures_++;
        if (consecutiveFailures_ >= 10) {
            Logger::instance().warning("Too many consecutive capture failures");
            // Try to reinitialize
            capturer_->stop();
            if (!initCapturer()) {
                emit error("Failed to reinitialize capture");
                stop();
            }
            consecutiveFailures_ = 0;
        }
    }
}

QImage ScreenCapturer::frameToQImage(const DesktopFrame& frame)
{
    if (frame.width() <= 0 || frame.height() <= 0 || !frame.data()) {
        return {};
    }

    // The frame data is already in RGBA format
    QImage image(frame.data(), frame.width(), frame.height(),
                 frame.stride(), QImage::Format_RGBA8888);

    // Make a deep copy since the frame data will be deallocated
    return image.copy();
}

bool ScreenCapturer::validateWindowHandle() const
{
    return windowId_ != 0
        && links::core::isWindowValid(static_cast<links::core::WindowId>(windowId_));
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
#ifdef Q_OS_WIN
    if (!screen_) {
        return 0;
    }

    auto normalizeName = [](const QString& name) {
        QString normalized = name.trimmed().toUpper();
        if (normalized.startsWith("\\\\.\\") || normalized.startsWith("//./")) {
            normalized = normalized.mid(4);
        }
        return normalized;
    };

    const QString screenName = normalizeName(screen_->name());
    const QRect screenGeometry = screen_->geometry();
    const auto monitors = win::enumerateMonitors();

    for (const auto& monitor : monitors) {
        const QString monitorName =
            normalizeName(QString::fromWCharArray(monitor.deviceName.c_str()));
        if (!monitorName.isEmpty() && monitorName == screenName) {
            return reinterpret_cast<DesktopCapturer::SourceId>(monitor.handle);
        }
    }

    for (const auto& monitor : monitors) {
        QRect bounds(monitor.bounds.left(), monitor.bounds.top(),
                     monitor.bounds.width(), monitor.bounds.height());
        if (bounds == screenGeometry) {
            return reinterpret_cast<DesktopCapturer::SourceId>(monitor.handle);
        }
    }
#endif

    return 0;
}
