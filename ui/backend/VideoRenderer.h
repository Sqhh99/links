#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QMutex>
#include <QPointer>

/**
 * @brief Video frame provider using QVideoSink for QML VideoOutput.
 * 
 * This class receives video frames from C++ and provides them to QML's
 * VideoOutput component through a QVideoSink.
 */
class VideoRenderer : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(QString participantId READ participantId WRITE setParticipantId NOTIFY participantIdChanged)
    Q_PROPERTY(QString participantName READ participantName WRITE setParticipantName NOTIFY participantNameChanged)
    Q_PROPERTY(bool micEnabled READ micEnabled WRITE setMicEnabled NOTIFY micEnabledChanged)
    Q_PROPERTY(bool camEnabled READ camEnabled WRITE setCamEnabled NOTIFY camEnabledChanged)
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY mirroredChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)
    
public:
    explicit VideoRenderer(QObject* parent = nullptr);
    ~VideoRenderer() override;
    
    // VideoSink for QML
    QVideoSink* videoSink() const { return videoSink_; }
    void setVideoSink(QVideoSink* sink);
    
    // Property getters
    QString participantId() const { return participantId_; }
    QString participantName() const { return participantName_; }
    bool micEnabled() const { return micEnabled_; }
    bool camEnabled() const { return camEnabled_; }
    bool mirrored() const { return mirrored_; }
    bool hasFrame() const { return hasFrame_; }
    
    // Property setters
    void setParticipantId(const QString& id);
    void setParticipantName(const QString& name);
    void setMicEnabled(bool enabled);
    void setCamEnabled(bool enabled);
    void setMirrored(bool mirrored);
    
    // Frame update (called from C++)
    Q_INVOKABLE void updateFrame(const QImage& frame);
    Q_INVOKABLE void clearFrame();
    
signals:
    void videoSinkChanged();
    void participantIdChanged();
    void participantNameChanged();
    void micEnabledChanged();
    void camEnabledChanged();
    void mirroredChanged();
    void hasFrameChanged();
    
private:
    QPointer<QVideoSink> videoSink_;
    QString participantId_;
    QString participantName_;
    bool micEnabled_{false};
    bool camEnabled_{true};
    bool mirrored_{false};
    bool hasFrame_{false};
    
    QMutex mutex_;
};

#endif // VIDEO_RENDERER_H
