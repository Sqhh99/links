#include "qt_video_input.h"

#include <QMediaDevices>

#include <utility>

namespace links {
namespace qt_adapter {

QtVideoInput::QtVideoInput(QObject* parent)
    : QObject(parent)
{
}

QtVideoInput::~QtVideoInput()
{
    stop();
}

void QtVideoInput::setFrameCallback(std::function<void(const core::VideoFrame&)> callback)
{
    frameCallback_ = std::move(callback);
}

void QtVideoInput::setErrorCallback(std::function<void(const std::string&)> callback)
{
    errorCallback_ = std::move(callback);
}

void QtVideoInput::reportError(const std::string& message)
{
    if (errorCallback_) {
        errorCallback_(message);
    }
}

bool QtVideoInput::setDeviceId(const std::string& deviceId)
{
    if (deviceId.empty()) {
        selectedDevice_ = QCameraDevice();
        camera_.reset();
        return true;
    }

    // Device ids are opaque bytes; compare them without any re-encoding.
    const QByteArray wanted = QByteArray::fromStdString(deviceId);
    const auto cameras = QMediaDevices::videoInputs();
    for (const auto& device : cameras) {
        if (device.id() == wanted) {
            selectedDevice_ = device;
            camera_.reset();  // recreated with the new device on next start()
            return true;
        }
    }
    return false;
}

bool QtVideoInput::start()
{
    if (active_) {
        return true;
    }

    if (!camera_) {
        const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
        if (cameras.isEmpty()) {
            reportError("No cameras available. Ensure Qt multimedia plugins are present "
                        "and the device is free.");
            return false;
        }

        QCameraDevice deviceToUse;
        if (!selectedDevice_.isNull()) {
            for (const auto& cam : cameras) {
                if (cam.id() == selectedDevice_.id()) {
                    deviceToUse = cam;
                    break;
                }
            }
        }
        if (deviceToUse.isNull()) {
            deviceToUse = cameras.first();
        }

        camera_ = std::make_unique<QCamera>(deviceToUse);
        videoSink_ = std::make_unique<QVideoSink>();
        captureSession_ = std::make_unique<QMediaCaptureSession>();
        captureSession_->setCamera(camera_.get());
        captureSession_->setVideoSink(videoSink_.get());

        connect(videoSink_.get(), &QVideoSink::videoFrameChanged,
                this, &QtVideoInput::onVideoFrameChanged);
    }

    camera_->start();
    active_ = true;
    return true;
}

void QtVideoInput::stop()
{
    if (!active_) {
        return;
    }
    if (camera_) {
        camera_->stop();
    }
    active_ = false;
}

bool QtVideoInput::isActive() const
{
    return active_;
}

void QtVideoInput::onVideoFrameChanged(const QVideoFrame& frame)
{
    if (!active_ || !frameCallback_) {
        return;
    }

    QVideoFrame localFrame = frame;
    if (!localFrame.map(QVideoFrame::ReadOnly)) {
        return;
    }
    QImage image = localFrame.toImage();
    localFrame.unmap();

    if (image.isNull()) {
        return;
    }
    if (image.format() != QImage::Format_RGBA8888) {
        image = image.convertToFormat(QImage::Format_RGBA8888);
    }

    const core::VideoFrame out = core::VideoFrame::copyFrom(
        image.constBits(), image.width(), image.height(),
        static_cast<int>(image.bytesPerLine()), core::PixelFormat::RGBA8888);

    frameCallback_(out);
}

}  // namespace qt_adapter
}  // namespace links
