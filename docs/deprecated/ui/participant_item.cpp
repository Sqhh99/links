#include "participant_item.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

ParticipantItem::ParticipantItem(const ParticipantInfo& info, QWidget* parent)
    : QWidget(parent),
      identity_(info.identity),
      name_(info.name.isEmpty() ? info.identity : info.name),
      micEnabled_(info.isMicrophoneEnabled),
      camEnabled_(info.isCameraEnabled)
{
    setupUI();
    updateInfo(info);
}

void ParticipantItem::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 8, 12, 8);
    layout->setSpacing(8);
    
    // Space for avatar (handled in paintEvent)
    layout->addSpacing(36); 
    
    nameLabel_ = new QLabel(this);
    nameLabel_->setStyleSheet("color: #ffffff; font-size: 13px; font-weight: 500;");
    layout->addWidget(nameLabel_, 1);
    
    // Camera button
    cameraButton_ = new QPushButton(this);
    cameraButton_->setFixedSize(28, 28);
    cameraButton_->setIconSize(QSize(16, 16));
    cameraButton_->setCursor(Qt::PointingHandCursor);
    cameraButton_->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 14px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.1);
        }
        QPushButton:pressed {
            background-color: rgba(255, 255, 255, 0.15);
        }
    )");
    layout->addWidget(cameraButton_);
    
    // Microphone button
    micButton_ = new QPushButton(this);
    micButton_->setFixedSize(28, 28);
    micButton_->setIconSize(QSize(16, 16));
    micButton_->setCursor(Qt::PointingHandCursor);
    micButton_->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 14px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.1);
        }
        QPushButton:pressed {
            background-color: rgba(255, 255, 255, 0.15);
        }
    )");
    layout->addWidget(micButton_);
    
    // Kick button (only visible for hosts)
    kickButton_ = new QPushButton(this);
    kickButton_->setFixedSize(28, 28);
    kickButton_->setIconSize(QSize(16, 16));
    kickButton_->setCursor(Qt::PointingHandCursor);
    kickButton_->setIcon(QIcon(":/icon/user-x.png"));
    kickButton_->setToolTip(tr("Kick participant"));
    kickButton_->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 14px;
        }
        QPushButton:hover {
            background-color: rgba(255, 82, 82, 0.2);
        }
        QPushButton:pressed {
            background-color: rgba(255, 82, 82, 0.3);
        }
    )");
    kickButton_->hide(); // Hidden by default
    layout->addWidget(kickButton_);
    
    setFixedHeight(52);
    setAttribute(Qt::WA_Hover);
    
    // Connect button signals
    connect(micButton_, &QPushButton::clicked, this, [this]() {
        emit micToggleClicked(identity_);
    });
    
    connect(cameraButton_, &QPushButton::clicked, this, [this]() {
        emit cameraToggleClicked(identity_);
    });
    
    connect(kickButton_, &QPushButton::clicked, this, [this]() {
        emit kickClicked(identity_);
    });
}

void ParticipantItem::updateInfo(const ParticipantInfo& info)
{
    name_ = info.name.isEmpty() ? info.identity : info.name;
    micEnabled_ = info.isMicrophoneEnabled;
    camEnabled_ = info.isCameraEnabled;
    
    QString displayName = name_;
    if (isLocalParticipant_) {
        displayName += " (You)";
    }
    nameLabel_->setText(displayName);
    
    updateButtonStates();
    update(); // Repaint for avatar
}

void ParticipantItem::setHostMode(bool isLocalHost)
{
    isLocalHost_ = isLocalHost;
    // Show kick button for hosts, but not for local participant (can't kick yourself)
    kickButton_->setVisible(isLocalHost_ && !isLocalParticipant_);
}

void ParticipantItem::setIsLocalParticipant(bool isLocal)
{
    isLocalParticipant_ = isLocal;
    // Update kick button visibility
    kickButton_->setVisible(isLocalHost_ && !isLocalParticipant_);
    // Disable buttons for local participant (use main controls instead)
    micButton_->setEnabled(!isLocal);
    cameraButton_->setEnabled(!isLocal);
    
    // Update name if already set
    QString displayName = name_;
    if (isLocalParticipant_) {
        displayName += " (You)";
    }
    nameLabel_->setText(displayName);
}

void ParticipantItem::updateButtonStates()
{
    // Update mic button icon
    if (micEnabled_) {
        micButton_->setIcon(QIcon(":/icon/Turn_on_the_microphone.png"));
        micButton_->setToolTip(isLocalHost_ ? tr("Mute participant") : tr("Mute audio"));
    } else {
        micButton_->setIcon(QIcon(":/icon/mute_the_microphone.png"));
        micButton_->setToolTip(isLocalHost_ ? tr("Request unmute") : tr("Unmute audio"));
    }
    
    // Update camera button icon
    if (camEnabled_) {
        cameraButton_->setIcon(QIcon(":/icon/video.png"));
        cameraButton_->setToolTip(isLocalHost_ ? tr("Turn off camera") : tr("Hide video"));
    } else {
        cameraButton_->setIcon(QIcon(":/icon/close_video.png"));
        cameraButton_->setToolTip(isLocalHost_ ? tr("Request camera on") : tr("Show video"));
    }
}

void ParticipantItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw background on hover
    if (underMouse()) {
        painter.fillRect(rect(), QColor(255, 255, 255, 12));
        // Add a small selection bar on left
        painter.fillRect(QRect(0, 0, 3, height()), QColor(88, 101, 242));
    }
    
    // Draw Avatar Circle
    QRect avatarRect(10, (height() - 32) / 2, 32, 32);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(88, 101, 242)); 
    
    painter.drawEllipse(avatarRect);
    
    // Draw Initials
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(14);
    font.setBold(true);
    painter.setFont(font);
    
    QString initial = name_.left(1).toUpper();
    painter.drawText(avatarRect, Qt::AlignCenter, initial);
}