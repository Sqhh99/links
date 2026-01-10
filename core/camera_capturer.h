#ifndef CAMERA_CAPTURER_H
#define CAMERA_CAPTURER_H

#include <QObject>
#include <QCamera>
#include <QVideoSink>
#include <QMediaCaptureSession>
#include <QVideoFrame>
#include <QImage>
#include <QElapsedTimer>
#include <memory>
#include <atomic>
#include "livekit/video_source.h"

class CameraCapturer : public QObject
{
    Q_OBJECT
    
public:
    explicit CameraCapturer(QObject* parent = nullptr);
    ~CameraCapturer() override;
    
    // Start/stop capture
    bool start();
    void stop();
    bool isActive() const { return isActive_; }
    
    // Get the LiveKit video source
    std::shared_ptr<livekit::VideoSource> getVideoSource() const { return videoSource_; }
    
    // Get available cameras
    static QList<QCameraDevice> availableCameras();
    
    // Set camera device (must be called before start())
    void setCamera(const QCameraDevice& device);
    void setCameraById(const QByteArray& deviceId);
    
    // Frame rate control
    void setTargetFps(int fps) { targetFps_ = fps; minFrameIntervalMs_ = 1000 / fps; }
    int getTargetFps() const { return targetFps_; }
    
signals:
    void frameReady(const QVideoFrame& frame);
    void frameCaptured(const QImage& image);
    void error(const QString& message);
    
private slots:
    void onVideoFrameChanged(const QVideoFrame& frame);
    
private:
    void processFrame(const QVideoFrame& frame);
    
    std::unique_ptr<QCamera> camera_;
    std::unique_ptr<QMediaCaptureSession> captureSession_;
    std::unique_ptr<QVideoSink> videoSink_;
    std::shared_ptr<livekit::VideoSource> videoSource_;
    
    bool isActive_;
    int frameCount_;
    
    // Frame rate control
    int targetFps_{30};
    int minFrameIntervalMs_{33}; // ~30fps
    QElapsedTimer frameTimer_;
    qint64 lastFrameTime_{0};
    
    // Cached converted image to avoid repeated allocations
    QImage cachedRgbaImage_;
    
    // Selected camera device
    QCameraDevice selectedDevice_;
};

#endif // CAMERA_CAPTURER_H
