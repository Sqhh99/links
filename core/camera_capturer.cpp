#include "camera_capturer.h"
#include "../utils/logger.h"
#include "livekit/video_frame.h"
#include <QMediaDevices>
#include <QImage>
#include <QDateTime>

CameraCapturer::CameraCapturer(QObject* parent)
    : QObject(parent),
      isActive_(false),
      frameCount_(0)
{
    // Create LiveKit video source (640x480 default)
    try {
        videoSource_ = std::make_shared<livekit::VideoSource>(640, 480);
        Logger::instance().info("VideoSource created for camera");
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Failed to create VideoSource: %1").arg(e.what()));
        emit error(QString("Failed to create video source: %1").arg(e.what()));
        return;
    }
    
    // Initialize frame timer
    frameTimer_.start();
    
    // Defer actual camera initialization to start() so we can retry if no device is present at construction time
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

    // (Re)initialize camera on start, in case devices appeared after construction
    if (!camera_) {
        QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
        if (cameras.isEmpty()) {
            Logger::instance().warning("No cameras available (Qt multimedia backend not found or device in use)");
            emit error("No cameras available. Ensure Qt multimedia plugins are present and the device is free.");
            return false;
        }

        // Use selected device if set, otherwise use first available
        QCameraDevice deviceToUse;
        if (!selectedDevice_.isNull()) {
            // Check if selected device is still available
            bool found = false;
            for (const auto& cam : cameras) {
                if (cam.id() == selectedDevice_.id()) {
                    deviceToUse = cam;
                    found = true;
                    break;
                }
            }
            if (!found) {
                Logger::instance().warning(QString("Selected camera '%1' not found, using default")
                                          .arg(selectedDevice_.description()));
                deviceToUse = cameras.first();
            }
        } else {
            deviceToUse = cameras.first();
        }

        camera_ = std::make_unique<QCamera>(deviceToUse);
        Logger::instance().info(QString("Using camera: %1").arg(deviceToUse.description()));

        captureSession_ = std::make_unique<QMediaCaptureSession>();
        captureSession_->setCamera(camera_.get());

        videoSink_ = std::make_unique<QVideoSink>();
        captureSession_->setVideoSink(videoSink_.get());

        connect(videoSink_.get(), &QVideoSink::videoFrameChanged,
                this, &CameraCapturer::onVideoFrameChanged);
    }
    
    try {
        camera_->start();
        isActive_ = true;
        frameCount_ = 0;
        Logger::instance().info("Camera started");
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Failed to start camera: %1").arg(e.what()));
        emit error(QString("Failed to start camera: %1").arg(e.what()));
        return false;
    }
}

void CameraCapturer::stop()
{
    if (!isActive_) {
        return;
    }
    
    if (camera_) {
        camera_->stop();
    }
    
    isActive_ = false;
    Logger::instance().info(QString("Camera stopped (captured %1 frames)").arg(frameCount_));
}

QList<QCameraDevice> CameraCapturer::availableCameras()
{
    return QMediaDevices::videoInputs();
}

void CameraCapturer::onVideoFrameChanged(const QVideoFrame& frame)
{
    if (!isActive_ || !videoSource_) {
        return;
    }
    
    processFrame(frame);
    emit frameReady(frame);
}

void CameraCapturer::processFrame(const QVideoFrame& frame)
{
    // Frame rate limiting - skip frames to maintain target FPS
    qint64 currentTime = frameTimer_.elapsed();
    if (currentTime - lastFrameTime_ < minFrameIntervalMs_) {
        return; // Skip this frame to maintain target frame rate
    }
    lastFrameTime_ = currentTime;
    
    // Make the frame writable
    QVideoFrame localFrame = frame;
    if (!localFrame.map(QVideoFrame::ReadOnly)) {
        Logger::instance().warning("Failed to map video frame");
        return;
    }
    
    // Convert to RGBA format for LiveKit - reuse cached image when possible
    QImage image = localFrame.toImage();
    localFrame.unmap();
    
    if (image.isNull()) {
        Logger::instance().warning("Failed to convert frame to image");
        return;
    }
    
    // Convert to RGBA32 if needed - reuse cached image to avoid allocation
    if (image.format() != QImage::Format_RGBA8888) {
        // Reuse cached image if size matches
        if (cachedRgbaImage_.width() == image.width() && 
            cachedRgbaImage_.height() == image.height() &&
            cachedRgbaImage_.format() == QImage::Format_RGBA8888) {
            // Fast path: convert directly into cached buffer
            image = image.convertToFormat(QImage::Format_RGBA8888);
            cachedRgbaImage_ = image;
        } else {
            image = image.convertToFormat(QImage::Format_RGBA8888);
            cachedRgbaImage_ = image;
        }
    }
    
    // Send to LiveKit
    try {
        // Create LKVideoFrame from RGBA image data
        std::vector<uint8_t> frameData(image.constBits(), image.constBits() + image.sizeInBytes());
        livekit::LKVideoFrame frame(image.width(), image.height(), 
                                    livekit::VideoBufferType::RGBA, 
                                    std::move(frameData));
        
        // Capture frame with current timestamp in microseconds
        int64_t timestamp_us = QDateTime::currentMSecsSinceEpoch() * 1000;
        videoSource_->captureFrame(frame, timestamp_us);

        emit frameCaptured(image);
        
        frameCount_++;
        
        // Log every 30 frames (about 1 second at 30fps)
        if (frameCount_ % 30 == 0) {
            Logger::instance().debug(QString("Captured %1 frames (%2x%3)")
                                    .arg(frameCount_)
                                    .arg(image.width())
                                    .arg(image.height()));
        }
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Failed to capture frame: %1").arg(e.what()));
    }
}

void CameraCapturer::setCamera(const QCameraDevice& device)
{
    if (isActive_) {
        Logger::instance().warning("Cannot change camera while active");
        return;
    }
    selectedDevice_ = device;
    camera_.reset(); // Reset camera so it will be recreated with new device on next start()
    Logger::instance().info(QString("Camera device set to: %1").arg(device.description()));
}

void CameraCapturer::setCameraById(const QByteArray& deviceId)
{
    if (deviceId.isEmpty()) {
        selectedDevice_ = QCameraDevice();
        return;
    }
    
    const auto cameras = QMediaDevices::videoInputs();
    for (const auto& device : cameras) {
        if (device.id() == deviceId) {
            setCamera(device);
            return;
        }
    }
    Logger::instance().warning(QString("Camera with ID '%1' not found")
                              .arg(QString(deviceId)));
}
