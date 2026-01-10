#include "settings_dialog.h"
#include "../utils/settings.h"
#include "../utils/logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QIcon>
#include <QScrollArea>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QAudioDevice>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground, true);
    resize(640, 480);
    setupUI();
    applyStyles();
    populateDevices();
    loadSettings();
}

void SettingsDialog::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* frame = new QWidget(this);
    frame->setObjectName("settingsFrame");
    auto* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(16, 16, 16, 16);
    frameLayout->setSpacing(12);
    rootLayout->addWidget(frame);

    titleBar_ = new QWidget(frame);
    titleBar_->setObjectName("settingsTitleBar");
    titleBar_->setFixedHeight(44);
    auto* titleLayout = new QHBoxLayout(titleBar_);
    titleLayout->setContentsMargins(12, 0, 12, 0);
    titleLayout->setSpacing(8);
    titleLabel_ = new QLabel("设置", titleBar_);
    titleLabel_->setObjectName("settingsTitle");
    titleLayout->addWidget(titleLabel_);
    titleLayout->addStretch();
    closeButton_ = new QPushButton(titleBar_);
    closeButton_->setObjectName("settingsClose");
    closeButton_->setFixedSize(32, 24);
    closeButton_->setIcon(QIcon(":/icon/close.png"));
    closeButton_->setIconSize(QSize(14, 14));
    titleLayout->addWidget(closeButton_);
    frameLayout->addWidget(titleBar_);

    auto* bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(12);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->addLayout(bodyLayout);

    navContainer_ = new QWidget(frame);
    navContainer_->setObjectName("settingsNav");
    auto* navLayout = new QVBoxLayout(navContainer_);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(8);
    auto makeNavBtn = [](const QString& text) {
        auto* btn = new QPushButton(text);
        btn->setObjectName("navButton");
        btn->setCheckable(true);
        btn->setMinimumHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };
    audioBtn_ = makeNavBtn("音频");
    videoBtn_ = makeNavBtn("视频");
    networkBtn_ = makeNavBtn("网络");
    audioBtn_->setChecked(true);
    navLayout->addWidget(audioBtn_);
    navLayout->addWidget(videoBtn_);
    navLayout->addWidget(networkBtn_);
    navLayout->addStretch();
    bodyLayout->addWidget(navContainer_, 1);

    stack_ = new QStackedWidget(frame);

    // Audio page
    auto* audioPage = new QWidget(stack_);
    auto* audioScroll = new QScrollArea(audioPage);
    audioScroll->setWidgetResizable(true);
    auto* audioContainer = new QWidget();
    auto* audioForm = new QFormLayout(audioContainer);
    audioForm->setLabelAlignment(Qt::AlignLeft);
    audioForm->setHorizontalSpacing(12);
    audioForm->setVerticalSpacing(10);
    micCombo_ = new QComboBox(audioContainer);
    speakerCombo_ = new QComboBox(audioContainer);
    echoCancel_ = new QCheckBox("回声消除", audioContainer);
    noiseSuppression_ = new QCheckBox("噪声抑制", audioContainer);
    echoCancel_->setChecked(true);
    noiseSuppression_->setChecked(true);
    audioForm->addRow("麦克风", micCombo_);
    audioForm->addRow("扬声器", speakerCombo_);
    audioForm->addRow("", echoCancel_);
    audioForm->addRow("", noiseSuppression_);
    audioContainer->setLayout(audioForm);
    audioScroll->setWidget(audioContainer);
    auto* audioLayout = new QVBoxLayout(audioPage);
    audioLayout->setContentsMargins(0, 0, 0, 0);
    audioLayout->addWidget(audioScroll);
    stack_->addWidget(audioPage);

    // Video page
    auto* videoPage = new QWidget(stack_);
    auto* videoScroll = new QScrollArea(videoPage);
    videoScroll->setWidgetResizable(true);
    auto* videoContainer = new QWidget();
    auto* videoForm = new QFormLayout(videoContainer);
    videoForm->setLabelAlignment(Qt::AlignLeft);
    videoForm->setHorizontalSpacing(12);
    videoForm->setVerticalSpacing(10);
    cameraCombo_ = new QComboBox(videoContainer);
    resolutionCombo_ = new QComboBox(videoContainer);
    resolutionCombo_->addItems({"1280x720", "1920x1080", "640x480"});
    hardwareAccel_ = new QCheckBox("启用硬件加速", videoContainer);
    hardwareAccel_->setChecked(true);
    videoForm->addRow("摄像头", cameraCombo_);
    videoForm->addRow("分辨率", resolutionCombo_);
    videoForm->addRow("", hardwareAccel_);
    videoContainer->setLayout(videoForm);
    videoScroll->setWidget(videoContainer);
    auto* videoLayout = new QVBoxLayout(videoPage);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    videoLayout->addWidget(videoScroll);
    stack_->addWidget(videoPage);

    // Network page
    auto* netPage = new QWidget(stack_);
    auto* netScroll = new QScrollArea(netPage);
    netScroll->setWidgetResizable(true);
    auto* netContainer = new QWidget();
    auto* netForm = new QFormLayout(netContainer);
    netForm->setLabelAlignment(Qt::AlignLeft);
    netForm->setHorizontalSpacing(12);
    netForm->setVerticalSpacing(10);
    apiUrlInput_ = new QLineEdit(netContainer);
    apiUrlInput_->setPlaceholderText("信令服务器地址，例如 wss://example.com");
    netForm->addRow("信令地址", apiUrlInput_);
    netContainer->setLayout(netForm);
    netScroll->setWidget(netContainer);
    auto* netLayout = new QVBoxLayout(netPage);
    netLayout->setContentsMargins(0, 0, 0, 0);
    netLayout->addWidget(netScroll);
    stack_->addWidget(netPage);

    bodyLayout->addWidget(stack_, 4);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    cancelButton_ = new QPushButton("取消", frame);
    cancelButton_->setObjectName("ghostButton");
    saveButton_ = new QPushButton("保存", frame);
    saveButton_->setObjectName("primaryButton");
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(saveButton_);
    frameLayout->addLayout(buttonLayout);

    connect(saveButton_, &QPushButton::clicked, this, &SettingsDialog::onAccept);
    connect(cancelButton_, &QPushButton::clicked, this, &SettingsDialog::onReject);
    connect(closeButton_, &QPushButton::clicked, this, &SettingsDialog::onReject);
    connect(audioBtn_, &QPushButton::clicked, this, [this]() {
        audioBtn_->setChecked(true);
        videoBtn_->setChecked(false);
        networkBtn_->setChecked(false);
        stack_->setCurrentIndex(0);
    });
    connect(videoBtn_, &QPushButton::clicked, this, [this]() {
        audioBtn_->setChecked(false);
        videoBtn_->setChecked(true);
        networkBtn_->setChecked(false);
        stack_->setCurrentIndex(1);
    });
    connect(networkBtn_, &QPushButton::clicked, this, [this]() {
        audioBtn_->setChecked(false);
        videoBtn_->setChecked(false);
        networkBtn_->setChecked(true);
        stack_->setCurrentIndex(2);
    });
}

void SettingsDialog::applyStyles()
{
    QString style = R"(
        #settingsFrame {
            background-color: #0f1116;
            border-radius: 14px;
            border: 1px solid #1f2230;
            color: #e9ebf1;
        }
        #settingsTitleBar {
            background: transparent;
        }
        #settingsTitle {
            font-size: 16px;
            font-weight: 700;
            color: #e9ebf1;
        }
        #settingsClose {
            border: none;
            background: rgba(255,255,255,0.06);
            border-radius: 6px;
        }
        #settingsClose:hover {
            background: rgba(255,82,82,0.18);
        }
        QLabel {
            color: #cfd2e0;
        }
        QComboBox, QLineEdit {
            background-color: #0e0e14;
            color: #ffffff;
            border: 1px solid #2a2a35;
            border-radius: 10px;
            padding: 8px 12px;
        }
        QComboBox:hover, QLineEdit:hover {
            border-color: #5865f2;
        }
        QCheckBox {
            color: #cfd2e0;
        }
        #settingsNav {
            background: rgba(255,255,255,0.02);
            border: 1px solid #1f2230;
            border-radius: 12px;
            padding: 10px;
        }
        #navButton {
            background: transparent;
            color: #9ea3b6;
            border: 1px solid #1f2230;
            border-radius: 10px;
            padding: 8px 12px;
            text-align: left;
        }
        #navButton:checked {
            background: #5865f2;
            color: white;
            border-color: #5865f2;
        }
        QPushButton#primaryButton {
            background-color: #5865f2;
            color: #ffffff;
            border: none;
            border-radius: 10px;
            padding: 10px 18px;
            font-weight: 700;
        }
        QPushButton#primaryButton:hover {
            background-color: #4752c4;
        }
        QPushButton#ghostButton {
            background-color: transparent;
            color: #c4c7d3;
            border: 1px solid #2a3041;
            border-radius: 10px;
            padding: 10px 16px;
        }
        QPushButton#ghostButton:hover {
            border-color: #3d4560;
        }
    )";
    setStyleSheet(style);
}

QString SettingsDialog::apiUrl() const
{
    return apiUrlInput_ ? apiUrlInput_->text().trimmed() : QString();
}

void SettingsDialog::setApiUrl(const QString& url)
{
    if (apiUrlInput_) {
        apiUrlInput_->setText(url);
    }
}

void SettingsDialog::onAccept()
{
    saveSettings();
    accept();
}

void SettingsDialog::onReject()
{
    reject();
}

void SettingsDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && titleBar_ && titleBar_->geometry().contains(event->position().toPoint())) {
        dragging_ = true;
        dragStartPos_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void SettingsDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_ && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragStartPos_);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void SettingsDialog::mouseReleaseEvent(QMouseEvent* event)
{
    dragging_ = false;
    QDialog::mouseReleaseEvent(event);
}

void SettingsDialog::populateDevices()
{
    // Populate microphones
    micCombo_->clear();
    const auto microphones = QMediaDevices::audioInputs();
    for (const auto& device : microphones) {
        // Convert QByteArray ID to QString for consistent storage
        micCombo_->addItem(device.description(), QString::fromUtf8(device.id()));
    }
    if (micCombo_->count() == 0) {
        micCombo_->addItem("无可用麦克风", "");
    }
    
    // Populate speakers
    speakerCombo_->clear();
    const auto speakers = QMediaDevices::audioOutputs();
    for (const auto& device : speakers) {
        // Convert QByteArray ID to QString for consistent storage
        speakerCombo_->addItem(device.description(), QString::fromUtf8(device.id()));
    }
    if (speakerCombo_->count() == 0) {
        speakerCombo_->addItem("无可用扬声器", "");
    }
    
    // Populate cameras
    cameraCombo_->clear();
    const auto cameras = QMediaDevices::videoInputs();
    for (const auto& device : cameras) {
        // Convert QByteArray ID to QString for consistent storage
        cameraCombo_->addItem(device.description(), QString::fromUtf8(device.id()));
    }
    if (cameraCombo_->count() == 0) {
        cameraCombo_->addItem("无可用摄像头", "");
    }
}

void SettingsDialog::loadSettings()
{
    auto& settings = Settings::instance();
    
    Logger::instance().info("Loading settings...");
    
    // Load signaling server URL
    QString serverUrl = settings.getSignalingServerUrl();
    Logger::instance().info(QString("Loaded signaling server URL: %1").arg(serverUrl));
    apiUrlInput_->setText(serverUrl);
    
    // Load selected camera
    QString cameraId = settings.getSelectedCameraId();
    Logger::instance().info(QString("Loaded camera ID: '%1'").arg(cameraId));
    if (!cameraId.isEmpty()) {
        int index = cameraCombo_->findData(cameraId);
        Logger::instance().info(QString("Camera combo box index for ID '%1': %2 (total items: %3)")
                               .arg(cameraId).arg(index).arg(cameraCombo_->count()));
        if (index >= 0) {
            cameraCombo_->setCurrentIndex(index);
            Logger::instance().info(QString("Set camera to index %1").arg(index));
        } else {
            Logger::instance().warning(QString("Camera ID '%1' not found in combo box").arg(cameraId));
        }
    }
    
    // Load selected microphone
    QString micId = settings.getSelectedMicrophoneId();
    Logger::instance().info(QString("Loaded microphone ID: '%1'").arg(micId));
    if (!micId.isEmpty()) {
        int index = micCombo_->findData(micId);
        Logger::instance().info(QString("Microphone combo box index for ID '%1': %2 (total items: %3)")
                               .arg(micId).arg(index).arg(micCombo_->count()));
        if (index >= 0) {
            micCombo_->setCurrentIndex(index);
            Logger::instance().info(QString("Set microphone to index %1").arg(index));
        } else {
            Logger::instance().warning(QString("Microphone ID '%1' not found in combo box").arg(micId));
        }
    }
    
    // Load selected speaker
    QString speakerId = settings.getSelectedSpeakerId();
    Logger::instance().info(QString("Loaded speaker ID: '%1'").arg(speakerId));
    if (!speakerId.isEmpty()) {
        int index = speakerCombo_->findData(speakerId);
        Logger::instance().info(QString("Speaker combo box index for ID '%1': %2 (total items: %3)")
                               .arg(speakerId).arg(index).arg(speakerCombo_->count()));
        if (index >= 0) {
            speakerCombo_->setCurrentIndex(index);
            Logger::instance().info(QString("Set speaker to index %1").arg(index));
        } else {
            Logger::instance().warning(QString("Speaker ID '%1' not found in combo box").arg(speakerId));
        }
    }
    
    Logger::instance().info("Settings loaded successfully");
}

void SettingsDialog::saveSettings()
{
    auto& settings = Settings::instance();
    
    Logger::instance().info("Saving settings...");
    
    // Save signaling server URL
    QString serverUrl = apiUrlInput_->text().trimmed();
    settings.setSignalingServerUrl(serverUrl);
    Logger::instance().info(QString("Saved signaling server URL: %1").arg(serverUrl));
    
    // Save selected camera
    if (cameraCombo_->count() > 0 && cameraCombo_->currentIndex() >= 0) {
        QString cameraId = cameraCombo_->currentData().toString();
        settings.setSelectedCameraId(cameraId);
        Logger::instance().info(QString("Saved camera ID: '%1'").arg(cameraId));
    }
    
    // Save selected microphone
    if (micCombo_->count() > 0 && micCombo_->currentIndex() >= 0) {
        QString micId = micCombo_->currentData().toString();
        settings.setSelectedMicrophoneId(micId);
        Logger::instance().info(QString("Saved microphone ID: '%1'").arg(micId));
    }
    
    // Save selected speaker
    if (speakerCombo_->count() > 0 && speakerCombo_->currentIndex() >= 0) {
        QString speakerId = speakerCombo_->currentData().toString();
        settings.setSelectedSpeakerId(speakerId);
        Logger::instance().info(QString("Saved speaker ID: '%1'").arg(speakerId));
    }
    
    // Force sync to disk
    settings.sync();
    
    Logger::instance().info("Settings saved successfully");
}
