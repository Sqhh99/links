#include "VideoRenderer.h"
#include <QMutexLocker>

VideoRenderer::VideoRenderer(QObject* parent)
    : QObject(parent)
{
}

VideoRenderer::~VideoRenderer() = default;

void VideoRenderer::setVideoSink(QVideoSink* sink)
{
    if (videoSink_ != sink) {
        videoSink_ = sink;
        emit videoSinkChanged();
    }
}

void VideoRenderer::setParticipantId(const QString& id)
{
    if (participantId_ != id) {
        participantId_ = id;
        emit participantIdChanged();
    }
}

void VideoRenderer::setParticipantName(const QString& name)
{
    if (participantName_ != name) {
        participantName_ = name;
        emit participantNameChanged();
    }
}

void VideoRenderer::setMicEnabled(bool enabled)
{
    if (micEnabled_ != enabled) {
        micEnabled_ = enabled;
        emit micEnabledChanged();
    }
}

void VideoRenderer::setCamEnabled(bool enabled)
{
    if (camEnabled_ != enabled) {
        camEnabled_ = enabled;
        emit camEnabledChanged();
    }
}

void VideoRenderer::setMirrored(bool mirrored)
{
    if (mirrored_ != mirrored) {
        mirrored_ = mirrored;
        emit mirroredChanged();
    }
}

void VideoRenderer::updateFrame(const QImage& frame)
{
    if (frame.isNull()) return;
    
    QMutexLocker locker(&mutex_);
    
    if (!videoSink_) return;
    
    // Convert to format suitable for QVideoFrame
    QImage convertedImage = frame;
    if (frame.format() != QImage::Format_RGBA8888 && 
        frame.format() != QImage::Format_RGB32) {
        convertedImage = frame.convertToFormat(QImage::Format_RGBA8888);
    }
    
    // Apply mirroring if needed
    if (mirrored_) {
        convertedImage = convertedImage.mirrored(true, false);
    }
    
    // Create video frame from image
    QVideoFrame videoFrame(convertedImage);
    
    if (videoFrame.isValid()) {
        videoSink_->setVideoFrame(videoFrame);
        
        if (!hasFrame_) {
            hasFrame_ = true;
            emit hasFrameChanged();
        }
    }
}

void VideoRenderer::clearFrame()
{
    QMutexLocker locker(&mutex_);
    
    if (videoSink_) {
        videoSink_->setVideoFrame(QVideoFrame());
    }
    
    if (hasFrame_) {
        hasFrame_ = false;
        emit hasFrameChanged();
    }
}
