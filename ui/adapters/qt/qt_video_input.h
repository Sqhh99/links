#ifndef QT_VIDEO_INPUT_H
#define QT_VIDEO_INPUT_H

#include <QByteArray>
#include <QCamera>
#include <QCameraDevice>
#include <QImage>
#include <QMediaCaptureSession>
#include <QObject>
#include <QVideoFrame>
#include <QVideoSink>

#include <memory>

#include "core/media/video_input.h"

namespace links {
namespace qt_adapter {

/**
 * VideoInput backed by Qt Multimedia.
 *
 * This is the QCamera half of the old core/camera_capturer.cpp, moved rather
 * than rewritten: device selection, the capture session and the
 * QVideoFrame -> RGBA conversion are the same steps in the same order.
 */
class QtVideoInput : public QObject, public core::VideoInput {
    Q_OBJECT
public:
    explicit QtVideoInput(QObject* parent = nullptr);
    ~QtVideoInput() override;

    bool start() override;
    void stop() override;
    bool isActive() const override;
    bool setDeviceId(const std::string& deviceId) override;
    void setFrameCallback(std::function<void(const core::VideoFrame&)> callback) override;
    void setErrorCallback(std::function<void(const std::string&)> callback) override;

private:
    void onVideoFrameChanged(const QVideoFrame& frame);
    void reportError(const std::string& message);

    std::unique_ptr<QCamera> camera_;
    std::unique_ptr<QMediaCaptureSession> captureSession_;
    std::unique_ptr<QVideoSink> videoSink_;
    QCameraDevice selectedDevice_;
    bool active_{false};

    std::function<void(const core::VideoFrame&)> frameCallback_;
    std::function<void(const std::string&)> errorCallback_;
};

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_VIDEO_INPUT_H
