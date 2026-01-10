#include "video_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QIcon>

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent),
      track_(nullptr),
      isMuted_(false),
      isMirrored_(false)
{
    setMinimumSize(160, 90);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Create overlay elements
    auto* overlayLayout = new QVBoxLayout(this);
    overlayLayout->setContentsMargins(8, 8, 8, 8);
    
    // Top row with status
    auto* topLayout = new QHBoxLayout();
    topLayout->addStretch();
    
    mutedIcon_ = new QLabel(this);
    mutedIcon_->hide(); // no top-right icon needed now
    topLayout->addWidget(mutedIcon_);
    
    overlayLayout->addLayout(topLayout);
    overlayLayout->addStretch();
    
    // Bottom row: name on left, status icons on right
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(8, 0, 8, 6);
    bottomLayout->setSpacing(8);
    bottomLayout->setAlignment(Qt::AlignBottom);

    nameLabel_ = new QLabel(this);
    nameLabel_->setStyleSheet(R"(
        QLabel {
            background: rgba(0,0,0,0.45);
            color: white;
            padding: 4px 8px;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 600;
        }
    )");
    nameLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    bottomLayout->addWidget(nameLabel_);
    bottomLayout->addStretch();

    statusContainer_ = new QWidget(this);
    statusContainer_->setFixedHeight(24);
    statusContainer_->setMinimumWidth(48);
    statusContainer_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    auto* statusLayout = new QHBoxLayout(statusContainer_);
    statusLayout->setContentsMargins(4, 0, 4, 0);
    statusLayout->setSpacing(6);
    micIcon_ = new QLabel(statusContainer_);
    micIcon_->setObjectName("micStateIcon");
    micIcon_->setFixedSize(20, 20);
    micIcon_->setScaledContents(true);
    camIcon_ = new QLabel(statusContainer_);
    camIcon_->setObjectName("camStateIcon");
    camIcon_->setFixedSize(20, 20);
    camIcon_->setScaledContents(true);
    statusLayout->addWidget(micIcon_);
    statusLayout->addWidget(camIcon_);
    bottomLayout->addWidget(statusContainer_, 0, Qt::AlignRight | Qt::AlignVCenter);
    statusContainer_->setVisible(showStatus_);
    
    overlayLayout->addLayout(bottomLayout);
}

VideoWidget::~VideoWidget()
{
}

void VideoWidget::setTrack(std::shared_ptr<livekit::Track> track)
{
    track_ = track;
    updateDisplay();
}

void VideoWidget::clearTrack()
{
    track_ = nullptr;
    hasFrame_ = false;
    currentFrame_ = QImage();
    updateDisplay();
}

void VideoWidget::setVideoFrame(const QImage& frame)
{
    currentFrame_ = frame;
    hasFrame_ = !frame.isNull();
    update();
}

void VideoWidget::setParticipantName(const QString& name)
{
    participantName_ = name;
    nameLabel_->setText(name);
    updateDisplay();
}

void VideoWidget::setMuted(bool muted)
{
    isMuted_ = muted;
    mutedIcon_->setVisible(muted);
    micEnabled_ = !muted;
    updateDisplay();
}

void VideoWidget::setMicEnabled(bool enabled)
{
    micEnabled_ = enabled;
    mutedIcon_->setVisible(!enabled);
    updateDisplay();
}

void VideoWidget::setCameraEnabled(bool enabled)
{
    cameraEnabled_ = enabled;
    updateDisplay();
}

void VideoWidget::setMirrored(bool mirrored)
{
    isMirrored_ = mirrored;
    update();
}

void VideoWidget::setShowStatus(bool show)
{
    showStatus_ = show;
    if (statusContainer_) {
        statusContainer_->setVisible(showStatus_);
    }
    update();
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Create rounded clipping path
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.setClipPath(path);
    
    // Draw background
    painter.fillRect(rect(), QColor(20, 20, 26));
    
    if (hasFrame_ && !currentFrame_.isNull()) {
        QImage frameToDraw = isMirrored_ ? currentFrame_.mirrored(true, false) : currentFrame_;
        
        // Scale to fit while maintaining aspect ratio
        QSize scaledSize = frameToDraw.size().scaled(size(), Qt::KeepAspectRatio);
        QRect centeredRect(
            (width() - scaledSize.width()) / 2,
            (height() - scaledSize.height()) / 2,
            scaledSize.width(),
            scaledSize.height()
        );
        
        painter.drawImage(centeredRect, frameToDraw);
    } else {
        // Draw placeholder avatar/initials
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(42, 42, 53));
        painter.drawRect(rect());
        
        // Draw initials
        painter.setPen(QColor(100, 100, 110));
        QFont font = painter.font();
        // Dynamic font size based on widget size
        int fontSize = qMin(width(), height()) / 4;
        font.setPixelSize(qMax(20, fontSize));
        font.setBold(true);
        painter.setFont(font);
        
        QString text = participantName_.isEmpty() ? "?" : participantName_.left(1).toUpper();
        painter.drawText(rect(), Qt::AlignCenter, text);
    }
    
    // Draw border
    // painter.setClipping(false);
    // painter.setPen(QPen(QColor(60, 60, 70), 1));
    // painter.setBrush(Qt::NoBrush);
    // painter.drawRoundedRect(rect().adjusted(1,1,-1,-1), 8, 8);
}

void VideoWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateDisplay();
}

void VideoWidget::updateDisplay()
{
    if (micIcon_) {
        QString iconPath = micEnabled_ ? ":/icon/Turn_on_the_microphone.png" : ":/icon/mute_the_microphone.png";
        qDebug() << "VideoWidget::updateDisplay - participantName:" << participantName_ 
                 << "micEnabled:" << micEnabled_ << "iconPath:" << iconPath;
        micIcon_->setPixmap(QIcon(iconPath).pixmap(micIcon_->size()));
    }
    if (camIcon_) {
        camIcon_->setPixmap(QIcon(cameraEnabled_ ? ":/icon/video.png"
                                                 : ":/icon/close_video.png").pixmap(camIcon_->size()));
    }
    update();
}
