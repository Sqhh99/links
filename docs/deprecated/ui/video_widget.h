#ifndef VIDEO_WIDGET_H
#define VIDEO_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QString>
#include <QPainter>
#include <QImage>
#include <memory>
#include "livekit/track.h"

class VideoWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit VideoWidget(QWidget* parent = nullptr);
    ~VideoWidget() override;
    
    void setTrack(std::shared_ptr<livekit::Track> track);
    void clearTrack();
    void setVideoFrame(const QImage& frame);
    
    void setParticipantName(const QString& name);
    void setMuted(bool muted);
    void setMicEnabled(bool enabled);
    void setCameraEnabled(bool enabled);
    void setMirrored(bool mirrored);
    void setShowStatus(bool show);
    
    bool hasTrack() const { return track_ != nullptr; }
    QString getParticipantName() const { return participantName_; }
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private:
    void updateDisplay();
    
    std::shared_ptr<livekit::Track> track_;
    QString participantName_;
    bool isMuted_;
    bool isMirrored_;
    bool hasFrame_{false};
    QImage currentFrame_;
    bool micEnabled_{false};
    bool cameraEnabled_{true};
    bool showStatus_{true};
    
    // UI elements
    QLabel* nameLabel_;
    QLabel* mutedIcon_;
    QWidget* statusContainer_{nullptr};
    QLabel* micIcon_{nullptr};
    QLabel* camIcon_{nullptr};
};

#endif // VIDEO_WIDGET_H
