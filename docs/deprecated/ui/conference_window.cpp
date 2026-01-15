#include "conference_window.h"
#include "gl_video_widget.h"
#include "../utils/logger.h"
#include "../utils/settings.h"
#include <QCloseEvent>
#include <QMessageBox>
#include <QSplitter>
#include <QTimer>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>
#include <QJsonObject>
#include <QJsonDocument>
#include "screen_picker_dialog.h"
#include "login_window.h"
#include "settings_dialog.h"

ConferenceWindow::ConferenceWindow(const QString& url,
                                 const QString& token,
                                 const QString& roomName,
                                 const QString& userName,
                                 bool isHost,
                                 QWidget* parent)
    : QMainWindow(parent),
      conferenceManager_(new ConferenceManager(this)),
      url_(url),
      token_(token),
      roomName_(roomName),
      userName_(userName),
      isChatVisible_(false),
      isParticipantsVisible_(false),
      mainParticipantId_(),
      pinnedMain_(false),
      isLocalUserHost_(isHost)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
#ifdef Q_OS_WIN
    windowEffect_.setAcrylicEffect(HWND(winId()), QColor(12, 12, 20, 200));
#endif
    setupUI();
    setupConnections();
    applyStyles();
    updateControlButtons();
    
    Logger::instance().info(QString("ConferenceWindow created for room: %1, isHost: %2").arg(roomName).arg(isHost));
    
    // Connect to conference
    conferenceManager_->connect(url, token);
}

ConferenceWindow::~ConferenceWindow()
{
    Logger::instance().info("ConferenceWindow destroyed");
}

void ConferenceWindow::setupUI()
{
    setWindowTitle("LiveKit Conference - " + roomName_);
    resize(1280, 800);
    
    centralWidget_ = new QWidget(this);
    centralWidget_->setObjectName("windowFrame");
    setCentralWidget(centralWidget_);
    
    auto* rootLayout = new QVBoxLayout(centralWidget_);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    
    // Top bar
    topBar_ = new QWidget(this);
    topBar_->setObjectName("topBar");
    topBar_->setFixedHeight(52);
    
    auto* topBarLayout = new QHBoxLayout(topBar_);
    topBarLayout->setContentsMargins(16, 0, 16, 0);
    topBarLayout->setSpacing(10);
    
    // Logo/Room Info
    auto* logoLabel = new QLabel(this);
    QPixmap logoPixmap(":/icon/video.png");
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        logoLabel->setText("LK");
    }
    logoLabel->setObjectName("logoLabel");
    topBarLayout->addWidget(logoLabel);
    
    auto* vLine = new QFrame(this);
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFrameShadow(QFrame::Sunken);
    vLine->setObjectName("topBarSeparator");
    vLine->setFixedHeight(20);
    topBarLayout->addWidget(vLine);
    
    roomNameLabel_ = new QLabel(roomName_, this);
    roomNameLabel_->setObjectName("roomNameLabel");
    topBarLayout->addWidget(roomNameLabel_);
    
    topBarLayout->addStretch();
    
    // Status Section
    participantCountLabel_ = new QLabel("1 participant", this);
    participantCountLabel_->setObjectName("statusText");
    topBarLayout->addWidget(participantCountLabel_);
    
    connectionStatusLabel_ = new QLabel("Connecting...", this);
    connectionStatusLabel_->setObjectName("connectionStatusLabel");
    topBarLayout->addWidget(connectionStatusLabel_);

    sidebarToggleButton_ = new QPushButton(this);
    sidebarToggleButton_->setObjectName("titleButton");
    sidebarToggleButton_->setCheckable(true);
    sidebarToggleButton_->setChecked(true);
    sidebarToggleButton_->setIcon(QIcon(":/icon/left_sidebar.png"));
    sidebarToggleButton_->setFixedSize(40, 32);
    sidebarToggleButton_->setIconSize(QSize(18, 18));
    sidebarToggleButton_->setToolTip("显示/隐藏侧栏");
    topBarLayout->addWidget(sidebarToggleButton_);

    alwaysOnTopButton_ = new QPushButton(this);
    alwaysOnTopButton_->setObjectName("titleButton");
    alwaysOnTopButton_->setCheckable(true);
    alwaysOnTopButton_->setIcon(QIcon(":/icon/zhiding.png"));
    alwaysOnTopButton_->setFixedSize(32, 24);
    alwaysOnTopButton_->setIconSize(QSize(14, 14));
    alwaysOnTopButton_->setToolTip("Always on top");
    topBarLayout->addWidget(alwaysOnTopButton_);

    settingsButton_ = new QPushButton(this);
    settingsButton_->setObjectName("titleButton");
    settingsButton_->setIcon(QIcon(":/icon/set_up.png"));
    settingsButton_->setFixedSize(32, 24);
    settingsButton_->setIconSize(QSize(16, 16));
    settingsButton_->setToolTip("Settings");
    topBarLayout->addWidget(settingsButton_);

    minimizeButton_ = new QPushButton(this);
    minimizeButton_->setObjectName("titleButton");
    minimizeButton_->setIcon(QIcon(":/icon/minimize.png"));
    minimizeButton_->setFixedSize(32, 24);
    minimizeButton_->setIconSize(QSize(14, 14));
    topBarLayout->addWidget(minimizeButton_);

    fullscreenButton_ = new QPushButton(this);
    fullscreenButton_->setObjectName("titleButton");
    fullscreenButton_->setIcon(QIcon(":/icon/maximize.png"));
    fullscreenButton_->setFixedSize(32, 24);
    fullscreenButton_->setIconSize(QSize(14, 14));
    fullscreenButton_->setToolTip("Toggle Fullscreen");
    topBarLayout->addWidget(fullscreenButton_);

    closeButton_ = new QPushButton(this);
    closeButton_->setObjectName("titleButtonClose");
    closeButton_->setIcon(QIcon(":/icon/close.png"));
    closeButton_->setFixedSize(32, 24);
    closeButton_->setIconSize(QSize(14, 14));
    topBarLayout->addWidget(closeButton_);

    topBar_->installEventFilter(this);
    
    rootLayout->addWidget(topBar_);
    
    // Main Content Area
    auto* contentWidget = new QWidget(this);
    contentWidget->setObjectName("contentWidget");
    auto* contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    rootLayout->addWidget(contentWidget, 1);

    // Left sidebar - video thumbnails
    videoSidebar_ = new QWidget(contentWidget);
    videoSidebar_->setObjectName("videoSidebar");
    videoSidebar_->setFixedWidth(240);
    auto* sidebarLayout = new QVBoxLayout(videoSidebar_);
    sidebarLayout->setContentsMargins(16, 16, 16, 16);
    sidebarLayout->setSpacing(12);
    
    localVideoWidget_ = new GLVideoWidget(contentWidget);
    localVideoWidget_->setParticipantName(userName_ + " (You)");
    localVideoWidget_->setMirrored(true);
    localVideoWidget_->setMicEnabled(conferenceManager_->isMicrophoneEnabled());
    localVideoWidget_->setCameraEnabled(conferenceManager_->isCameraEnabled());
    localVideoWidget_->setFixedHeight(135);
    localVideoWidget_->installEventFilter(this);
    sidebarLayout->addWidget(localVideoWidget_);
    micState_["local"] = conferenceManager_->isMicrophoneEnabled();
    camState_["local"] = conferenceManager_->isCameraEnabled();

    // Local screen share placeholder (hidden until active)
    auto* screenLabel = new QLabel("SCREEN SHARE", contentWidget);
    screenLabel->setObjectName("sectionHeader");
    screenLabel->setVisible(false);
    sidebarLayout->addWidget(screenLabel);

    localScreenWidget_ = new GLVideoWidget(contentWidget);
    localScreenWidget_->setParticipantName("Screen");
    localScreenWidget_->setFixedHeight(135);
    localScreenWidget_->installEventFilter(this);
    localScreenWidget_->hide();
    sidebarLayout->addWidget(localScreenWidget_);
    
    auto* headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);
    auto* remoteLabel = new QLabel("EVERYONE", contentWidget);
    remoteLabel->setObjectName("sectionHeader");
    headerRow->addWidget(remoteLabel);
    headerRow->addStretch();
    sidebarLayout->addLayout(headerRow);
    
    remoteVideosScroll_ = new QScrollArea(contentWidget);
    remoteVideosScroll_->setWidgetResizable(true);
    remoteVideosScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    remoteVideosScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    remoteVideosScroll_->setObjectName("transparentScroll");
    
    remoteVideosContainer_ = new QWidget();
    remoteVideosContainer_->setObjectName("transparentWidget");
    remoteVideosLayout_ = new QVBoxLayout(remoteVideosContainer_);
    remoteVideosLayout_->setContentsMargins(0, 0, 0, 0);
    remoteVideosLayout_->setSpacing(12);
    remoteVideosLayout_->addStretch();
    
    remoteVideosScroll_->setWidget(remoteVideosContainer_);
    sidebarLayout->addWidget(remoteVideosScroll_, 1);
    
    contentLayout->addWidget(videoSidebar_);
    
    // Center panel - main video
    centerPanel_ = new QWidget(contentWidget);
    centerPanel_->setObjectName("centerPanel");
    auto* centerLayout = new QVBoxLayout(centerPanel_);
    centerLayout->setContentsMargins(16, 16, 16, 16); // Padding around main video
    centerLayout->setSpacing(0);
    
    // Main video container with simpler background
    mainVideoWidget_ = new GLVideoWidget(centerPanel_);
    mainVideoWidget_->setParticipantName("Waiting for participants...");
    mainVideoWidget_->setShowStatus(false); // main area does not show mic/cam icons
    centerLayout->addWidget(mainVideoWidget_, 1);
    
    contentLayout->addWidget(centerPanel_, 1);

    // OVERLAYS (Floating)
    
    // Control bar
    controlBar_ = new QWidget(this);
    controlBar_->setObjectName("controlBar");
    controlBar_->setFixedHeight(72);
    
    // Shadow for control bar
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 4);
    controlBar_->setGraphicsEffect(shadow);
    
    auto* controlLayout = new QHBoxLayout(controlBar_);
    controlLayout->setContentsMargins(16, 0, 16, 0);
    controlLayout->setSpacing(16);
    controlLayout->setAlignment(Qt::AlignCenter);
    
    micButton_ = new QPushButton(controlBar_);
    QIcon micIcon(":/icon/Turn_on_the_microphone.png");
    if (!micIcon.isNull()) {
        micButton_->setIcon(micIcon);
    }
    micButton_->setObjectName("controlButton");
    micButton_->setFixedSize(48, 48);
    micButton_->setIconSize(QSize(24, 24));
    micButton_->setCheckable(true);
    micButton_->setToolTip("Toggle Microphone");
    controlLayout->addWidget(micButton_);
    
    cameraButton_ = new QPushButton(controlBar_);
    cameraButton_->setIcon(QIcon(":/icon/video.png"));
    cameraButton_->setObjectName("controlButton");
    cameraButton_->setFixedSize(48, 48);
    cameraButton_->setIconSize(QSize(24, 24));
    cameraButton_->setCheckable(true);
    cameraButton_->setToolTip("Toggle Camera");
    controlLayout->addWidget(cameraButton_);
    
    screenShareButton_ = new QPushButton(controlBar_);
    screenShareButton_->setIcon(QIcon(":/icon/screen_sharing.png"));
    screenShareButton_->setObjectName("controlButton");
    screenShareButton_->setFixedSize(48, 48);
    screenShareButton_->setIconSize(QSize(24, 24));
    screenShareButton_->setCheckable(true);
    screenShareButton_->setToolTip("Share Screen");
    controlLayout->addWidget(screenShareButton_);
    
    // Separator
    auto* sep = new QFrame(controlBar_);
    sep->setFrameShape(QFrame::VLine);
    sep->setObjectName("controlSeparator");
    sep->setFixedSize(1, 24);
    controlLayout->addWidget(sep);
    
    chatButton_ = new QPushButton(controlBar_);
    chatButton_->setIcon(QIcon(":/icon/message.png"));
    chatButton_->setObjectName("controlButton");
    chatButton_->setFixedSize(48, 48);
    chatButton_->setIconSize(QSize(24, 24));
    chatButton_->setCheckable(true);
    chatButton_->setToolTip("Chat");
    controlLayout->addWidget(chatButton_);
    
    participantsButton_ = new QPushButton(controlBar_);
    participantsButton_->setIcon(QIcon(":/icon/user.png"));
    participantsButton_->setObjectName("controlButton");
    participantsButton_->setFixedSize(48, 48);
    participantsButton_->setIconSize(QSize(24, 24));
    participantsButton_->setCheckable(true);
    participantsButton_->setChecked(false);
    participantsButton_->setToolTip("Participants");
    controlLayout->addWidget(participantsButton_);
    
    auto* sep2 = new QFrame(controlBar_);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setObjectName("controlSeparator");
    sep2->setFixedSize(1, 24);
    controlLayout->addWidget(sep2);
    
    leaveButton_ = new QPushButton(controlBar_);
    leaveButton_->setIcon(QIcon(":/icon/hang_up.png"));
    leaveButton_->setObjectName("leaveButton");
    leaveButton_->setFixedSize(60, 48); // Slightly wider pill shape
    leaveButton_->setIconSize(QSize(24, 24));
    leaveButton_->setToolTip("Leave Meeting");
    controlLayout->addWidget(leaveButton_);
    
    // Right sidebar - participants/chat (Overlay)
    rightSidebar_ = new QWidget(this);
    rightSidebar_->setObjectName("rightSidebar");
    rightSidebar_->setFixedWidth(320);
    
    // Shadow for sidebar
    auto* sidebarShadow = new QGraphicsDropShadowEffect(this);
    sidebarShadow->setBlurRadius(30);
    sidebarShadow->setColor(QColor(0, 0, 0, 80));
    sidebarShadow->setOffset(-4, 0);
    rightSidebar_->setGraphicsEffect(sidebarShadow);
    
    auto* rightLayout = new QVBoxLayout(rightSidebar_);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    
    // Sidebar Header
    auto* sidebarHeader = new QWidget(rightSidebar_);
    sidebarHeader->setFixedHeight(50);
    sidebarHeader->setObjectName("sidebarHeader");
    auto* sidebarHeaderLayout = new QHBoxLayout(sidebarHeader);
    sidebarHeaderLayout->setContentsMargins(16, 0, 16, 0);
    
    sidebarTitleLabel_ = new QLabel("Participants", sidebarHeader);
    sidebarTitleLabel_->setObjectName("sidebarTitle");
    sidebarHeaderLayout->addWidget(sidebarTitleLabel_);
    
    rightLayout->addWidget(sidebarHeader);
    
    // Participants panel
    participantsPanel_ = new QWidget(rightSidebar_);
    auto* participantsPanelLayout = new QVBoxLayout(participantsPanel_);
    participantsPanelLayout->setContentsMargins(0, 0, 0, 0);
    
    participantsScroll_ = new QScrollArea(participantsPanel_);
    participantsScroll_->setWidgetResizable(true);
    participantsScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    participantsScroll_->setObjectName("transparentScroll");
    
    participantsContainer_ = new QWidget();
    participantsContainer_->setObjectName("transparentWidget");
    participantsLayout_ = new QVBoxLayout(participantsContainer_);
    participantsLayout_->setContentsMargins(8, 8, 8, 8);
    participantsLayout_->setSpacing(4);
    
    // Add local participant item at the top
    ParticipantInfo localInfo;
    localInfo.identity = "local";
    localInfo.name = userName_;
    localInfo.isMicrophoneEnabled = conferenceManager_->isMicrophoneEnabled();
    localInfo.isCameraEnabled = conferenceManager_->isCameraEnabled();
    localInfo.isScreenSharing = false;
    localInfo.isHost = isLocalUserHost_;
    
    localParticipantItem_ = new ParticipantItem(localInfo, this);
    localParticipantItem_->setIsLocalParticipant(true);
    localParticipantItem_->setHostMode(isLocalUserHost_);
    participantsLayout_->addWidget(localParticipantItem_);
    
    participantsLayout_->addStretch();
    
    participantsScroll_->setWidget(participantsContainer_);
    participantsPanelLayout->addWidget(participantsScroll_, 1);
    
    rightLayout->addWidget(participantsPanel_);
    
    // Chat panel
    chatPanel_ = new ChatPanel(rightSidebar_);
    chatPanel_->hide();
    rightLayout->addWidget(chatPanel_);

    // Sidebar toggle button
    // Initial Z-order
    controlBar_->raise();
    rightSidebar_->raise();
}

void ConferenceWindow::setupConnections()
{
    // Conference manager signals
    connect(conferenceManager_, &ConferenceManager::connected,
            this, &ConferenceWindow::onConnected);
    connect(conferenceManager_, &ConferenceManager::disconnected,
            this, &ConferenceWindow::onDisconnected);
    connect(conferenceManager_, &ConferenceManager::connectionStateChanged,
            this, &ConferenceWindow::onConnectionStateChanged);
    connect(conferenceManager_, &ConferenceManager::participantJoined,
            this, &ConferenceWindow::onParticipantJoined);
    connect(conferenceManager_, &ConferenceManager::participantLeft,
            this, &ConferenceWindow::onParticipantLeft);
    connect(conferenceManager_, &ConferenceManager::trackSubscribed,
            this, &ConferenceWindow::onTrackSubscribed);
    connect(conferenceManager_, &ConferenceManager::trackUnsubscribed,
            this, &ConferenceWindow::onTrackUnsubscribed);
    connect(conferenceManager_, &ConferenceManager::chatMessageReceived,
            this, &ConferenceWindow::onChatMessageReceived);
    connect(conferenceManager_, &ConferenceManager::videoFrameReceived,
            this, &ConferenceWindow::onVideoFrameReceived);
    connect(conferenceManager_, &ConferenceManager::localVideoFrameReady,
            this, [this](const QImage& frame) {
                latestFrames_["local"] = frame;
                localVideoWidget_->setVideoFrame(frame);
                if (!pinnedMain_ && mainParticipantId_.isEmpty()) {
                    mainParticipantId_ = "local";
                    mainVideoWidget_->setVideoFrame(frame);
                    mainVideoWidget_->setParticipantName(userName_ + " (You)");
                }
                if (mainParticipantId_ == "local") {
                    mainVideoWidget_->setVideoFrame(frame);
                    mainVideoWidget_->setParticipantName(userName_ + " (You)");
                }
            });
    connect(conferenceManager_, &ConferenceManager::localScreenFrameReady,
            this, [this](const QImage& frame) {
                latestFrames_["local_screen"] = frame;
                if (screenLabel_) screenLabel_->setVisible(true);
                if (localScreenWidget_) {
                    localScreenWidget_->setVideoFrame(frame);
                    localScreenWidget_->show();
                }
                // auto-prioritize local screen share when not pinned
                if (!pinnedMain_) {
                    activeScreenShareId_ = "local_screen";
                    mainParticipantId_ = "local_screen";
                    if (mainVideoWidget_) {
                        mainVideoWidget_->setVideoFrame(frame);
                        mainVideoWidget_->setParticipantName("Screen Share");
                    }
                } else if (mainParticipantId_ == "local_screen" && mainVideoWidget_) {
                    mainVideoWidget_->setVideoFrame(frame);
                    mainVideoWidget_->setParticipantName("Screen Share");
                }
            });
    connect(conferenceManager_, &ConferenceManager::localScreenShareChanged,
            this, [this](bool enabled) {
                updateControlButtons();
                if (!enabled && localScreenWidget_) {
                    localScreenWidget_->hide();
                }
            });
    connect(fullscreenButton_, &QPushButton::clicked, this, &ConferenceWindow::toggleFullscreen);
    connect(minimizeButton_, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(closeButton_, &QPushButton::clicked, this, &QWidget::close);
    connect(alwaysOnTopButton_, &QPushButton::clicked, this, [this]() {
        alwaysOnTop_ = !alwaysOnTop_;
        setWindowFlag(Qt::WindowStaysOnTopHint, alwaysOnTop_);
        show();
        alwaysOnTopButton_->setIcon(alwaysOnTop_ ? QIcon(":/icon/yizhiding.png") : QIcon(":/icon/zhiding.png"));
    });
    connect(settingsButton_, &QPushButton::clicked, this, [this]() {
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            // Currently only UI/UX settings; wiring to backend can be added here
        }
    });
    
    // Control buttons
    connect(micButton_, &QPushButton::clicked,
            this, &ConferenceWindow::onMicrophoneToggled);
    connect(cameraButton_, &QPushButton::clicked,
            this, &ConferenceWindow::onCameraToggled);
    connect(screenShareButton_, &QPushButton::clicked,
            this, &ConferenceWindow::onScreenShareToggled);
    connect(chatButton_, &QPushButton::clicked,
            this, &ConferenceWindow::onChatToggled);
    connect(participantsButton_, &QPushButton::clicked,
            this, &ConferenceWindow::onParticipantsToggled);
    connect(leaveButton_, &QPushButton::clicked,
            this, &ConferenceWindow::onLeaveClicked);
    connect(sidebarToggleButton_, &QPushButton::clicked, this, [this]() {
        sidebarVisible_ = sidebarToggleButton_->isChecked();
        videoSidebar_->setVisible(sidebarVisible_);
        updateLayout();
    });
    
    // Chat panel
    connect(chatPanel_, &ChatPanel::sendMessageRequested,
            this, &ConferenceWindow::onSendChatMessage);

    connect(conferenceManager_, &ConferenceManager::trackMutedStateChanged,
            this, [this](const QString& trackSid, const QString& id, livekit::TrackKind kind, bool muted) {
                Q_UNUSED(trackSid);
                Logger::instance().info(QString("trackMutedStateChanged: id=%1, kind=%2, muted=%3")
                    .arg(id).arg(kind == livekit::proto::KIND_AUDIO ? "AUDIO" : "VIDEO").arg(muted));
                if (kind == livekit::proto::KIND_AUDIO) {
                    micState_[id] = !muted;
                    Logger::instance().info(QString("Setting mic state for %1 to %2").arg(id).arg(!muted));
                    refreshVideoWidgetState(id);
                } else if (kind == livekit::proto::KIND_VIDEO) {
                    camState_[id] = !muted;
                    refreshVideoWidgetState(id);
                }
            });

    // Remote audio activity - just track last seen time, don't change mic state
    // (mic state is controlled by trackMutedStateChanged only)
    connect(conferenceManager_, &ConferenceManager::audioActivity,
            this, [this](const QString& id, bool hasAudio) {
                if (hasAudio) {
                    lastAudioSeen_[id] = QDateTime::currentDateTime();
                }
            });

    // Inactivity timer for remote media states
    startInactivityTimer();
}

void ConferenceWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateLayout();
}

void ConferenceWindow::updateLayout()
{
    if (!centralWidget_) return;
    
    // Position Control Bar (Bottom Center, Floating)
    int controlWidth = 520; 
    int controlHeight = 72;
    int controlX = (width() - controlWidth) / 2;
    int controlY = height() - controlHeight - 32; // Margin from bottom
    
    controlBar_->setGeometry(controlX, controlY, controlWidth, controlHeight);
    
    // Position Right Sidebar (Right Edge, Floating, Full Height minus top bar)
    if (isChatVisible_ || isParticipantsVisible_) {
        int sidebarWidth = 320;
        int topMargin = 60; // Height of topBar
        int sidebarHeight = height() - topMargin;
        int sidebarX = width() - sidebarWidth;
        int sidebarY = topMargin;
        
        rightSidebar_->setGeometry(sidebarX, sidebarY, sidebarWidth, sidebarHeight);
        rightSidebar_->raise();
        rightSidebar_->show();
    } else {
        rightSidebar_->hide();
    }

    if (videoSidebar_) {
        videoSidebar_->setVisible(sidebarVisible_);
        videoSidebar_->setMaximumWidth(sidebarVisible_ ? 240 : 0);
        videoSidebar_->setMinimumWidth(sidebarVisible_ ? 200 : 0);
    }
}

void ConferenceWindow::applyStyles()
{
    QString styleSheet = QString(R"(
        QMainWindow {
            background: transparent;
        }

        #windowFrame {
            background-color: #0e0e11;
            border-radius: %1px;
        }
        
        /* Top Bar */
        #topBar {
            background-color: #1a1a23;
            border-bottom: 1px solid #2a2a35;
            border-top-left-radius: %1px;
            border-top-right-radius: %1px;
        }

        #titleButton, #titleButtonClose {
            border: none;
            background: transparent;
        }

        #titleButton:hover {
            background-color: rgba(255, 255, 255, 0.08);
        }

        #titleButtonClose:hover {
            background-color: rgba(255, 82, 82, 0.15);
        }
        
        #logoLabel {
            color: #ffffff;
            font-weight: 900;
            font-size: 16px;
            letter-spacing: 1px;
        }
        
        #roomNameLabel {
            color: #ffffff;
            font-weight: 600;
            font-size: 14px;
        }
        
        #statusText {
            color: #a0a0b0;
            font-size: 13px;
        }
        
        #connectionStatusLabel {
            font-size: 13px;
            font-weight: 500;
            padding-left: 8px;
        }
        
        #topBarSeparator {
            color: #2a2a35;
        }
        
        /* Sidebars */
        #videoSidebar {
            background-color: #13131a;
            border-right: 1px solid #2a2a35;
        }
        
        #sectionHeader {
            color: #6a6a7e;
            font-size: 11px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-top: 8px;
            margin-bottom: 4px;
        }
        
        #centerPanel {
            background-color: #000000;
        }
        
        #transparentScroll {
            background: transparent;
            border: none;
        }
        #transparentScroll QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 6px 0 6px 0;
        }
        #transparentScroll QScrollBar::handle:vertical {
            background: #555f7a;
            min-height: 28px;
            border-radius: 4px;
        }
        #transparentScroll QScrollBar::handle:vertical:hover {
            background: #6b74a0;
        }
        #transparentScroll QScrollBar::add-line:vertical,
        #transparentScroll QScrollBar::sub-line:vertical {
            height: 0;
        }
        
        #transparentWidget {
            background: transparent;
        }
        
        /* Floating Control Bar */
        #controlBar {
            background-color: rgba(26, 26, 35, 0.95);
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 36px;
        }
        
        #controlButton {
            background-color: transparent;
            color: #ffffff;
            border: none;
            border-radius: 24px;
            padding: 10px;
        }
        
        #controlButton:hover {
            background-color: rgba(255, 255, 255, 0.1);
        }
        
        #controlButton:checked {
            background-color: #5865f2; /* Brand color when active */
            color: #ffffff;
        }
        
        #controlSeparator {
            color: rgba(255, 255, 255, 0.1);
        }
        QPushButton#ghostButton {
            background-color: transparent;
            color: #c4c7d3;
            border: 1px solid #2a3041;
            border-radius: 10px;
            padding: 6px 12px;
        }
        QPushButton#ghostButton:hover {
            border-color: #3d4560;
        }
        
        #leaveButton {
            background-color: rgba(244, 67, 54, 0.15);
            color: #ff5252;
            border: none;
            border-radius: 24px;
            font-size: 20px;
        }
        
        #leaveButton:hover {
            background-color: #ff5252;
            color: white;
        }
        
        /* Right Sidebar Overlay */
        #rightSidebar {
            background-color: #1a1a23;
            border-left: 1px solid #2a2a35;
        }
        
        #sidebarHeader {
            border-bottom: 1px solid #2a2a35;
        }
        
        #sidebarTitle {
            color: #ffffff;
            font-size: 15px;
            font-weight: 600;
        }
    )");
    
    setStyleSheet(styleSheet.arg(isFullscreen_ ? 0 : cornerRadius_));
}

void ConferenceWindow::closeEvent(QCloseEvent* event)
{
    Logger::instance().info("Close event triggered");
    
    auto reply = QMessageBox::question(this, "Leave Meeting",
                                      "Are you sure you want to leave the meeting?",
                                      QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        Logger::instance().info("User confirmed leaving meeting");
        
        // Disconnect from conference
        if (conferenceManager_) {
            conferenceManager_->disconnect();
        }
        auto* login = new LoginWindow();
        login->show();
        event->accept();
        Logger::instance().info("Window closed, returning to login");
    } else {
        Logger::instance().info("User cancelled leaving meeting");
        event->ignore();
    }
}

// Conference event handlers

void ConferenceWindow::onConnected()
{
    Logger::instance().info("Connected to conference");
    connectionStatusLabel_->setText("Connected");
    connectionStatusLabel_->setStyleSheet("color: #4caf50;");
    updateParticipantCount();
    updateControlButtons();
}

void ConferenceWindow::onDisconnected()
{
    Logger::instance().info("Disconnected from conference");
    connectionStatusLabel_->setText("Disconnected");
    connectionStatusLabel_->setStyleSheet("color: #ff5252;");
    screenShareActive_.clear();
    mainParticipantId_.clear();
    pinnedMain_ = false;
    activeScreenShareId_.clear();
    mainVideoWidget_->clearTrack();
}

void ConferenceWindow::onConnectionStateChanged(livekit::ConnectionState state)
{
    QString stateText;
    QString color;
    
    switch (state) {
        case livekit::proto::CONN_CONNECTED:
            stateText = "Connected";
            color = "#4caf50";
            break;
        case livekit::proto::CONN_DISCONNECTED:
            stateText = "Disconnected";
            color = "#ff5252";
            break;
        case livekit::proto::CONN_RECONNECTING:
            stateText = "Reconnecting...";
            color = "#ff9800";
            break;
        default:
            stateText = "Unknown";
            color = "#a0a0b0";
    }
    
    connectionStatusLabel_->setText(stateText);
    connectionStatusLabel_->setStyleSheet(QString("color: %1;").arg(color));
}

void ConferenceWindow::onParticipantJoined(const ParticipantInfo& info)
{
    Logger::instance().info(QString("=== Participant joined: %1, mic=%2, cam=%3 ===")
        .arg(info.name).arg(info.isMicrophoneEnabled).arg(info.isCameraEnabled));
    
    // Add to participants list
    auto* item = new ParticipantItem(info, this);
    item->setHostMode(isLocalUserHost_);
    participantItems_[info.identity] = item;
    participantsLayout_->insertWidget(participantsLayout_->count() - 1, item);
    nameMap_[info.identity] = info.name.isEmpty() ? info.identity : info.name;
    
    // Connect item signals
    connect(item, &ParticipantItem::micToggleClicked, 
            this, &ConferenceWindow::onParticipantMicToggle);
    connect(item, &ParticipantItem::cameraToggleClicked, 
            this, &ConferenceWindow::onParticipantCameraToggle);
    connect(item, &ParticipantItem::kickClicked, 
            this, &ConferenceWindow::onParticipantKick);
    
    // Use the initial state from ParticipantInfo
    micState_[info.identity] = info.isMicrophoneEnabled;
    camState_[info.identity] = info.isCameraEnabled;
    
    Logger::instance().info(QString("Setting initial widget state: identity=%1, mic=%2, cam=%3")
        .arg(info.identity).arg(info.isMicrophoneEnabled).arg(info.isCameraEnabled));
    
    // Create video widget with initial state
    auto* videoWidget = new GLVideoWidget(this);
    videoWidget->setParticipantName(nameMap_[info.identity]);
    videoWidget->setMicEnabled(info.isMicrophoneEnabled);
    videoWidget->setCameraEnabled(info.isCameraEnabled);
    videoWidget->setFixedHeight(135);
    videoWidget->installEventFilter(this);
    remoteVideoWidgets_[info.identity] = videoWidget;
    remoteVideosLayout_->insertWidget(remoteVideosLayout_->count() - 1, videoWidget);
    
    updateParticipantCount();
}

void ConferenceWindow::onParticipantLeft(const QString& identity)
{
    Logger::instance().info("Participant left: " + identity);
    
    // Remove from participants list
    if (participantItems_.contains(identity)) {
        auto* item = participantItems_.take(identity);
        participantsLayout_->removeWidget(item);
        item->deleteLater();
    }
    
    // Remove video widget
    if (remoteVideoWidgets_.contains(identity)) {
        auto* widget = remoteVideoWidgets_.take(identity);
        remoteVideosLayout_->removeWidget(widget);
        widget->deleteLater();
    }

    if (mainParticipantId_ == identity) {
        mainParticipantId_.clear();
        mainVideoWidget_->clearTrack();
    }
    screenShareActive_.remove(identity);
    if (activeScreenShareId_ == identity) {
        activeScreenShareId_.clear();
    }
    micState_.remove(identity);
    camState_.remove(identity);
    nameMap_.remove(identity);
    lastAudioSeen_.remove(identity);
    lastVideoSeen_.remove(identity);
    
    updateParticipantCount();
}

void ConferenceWindow::onTrackSubscribed(const TrackInfo& track)
{
    Logger::instance().info(QString("Track subscribed: %1 from %2")
                           .arg(track.trackSid, track.participantIdentity));
    trackKinds_[track.trackSid] = track.kind;
    
    if (track.kind == livekit::proto::KIND_VIDEO) {
        if (remoteVideoWidgets_.contains(track.participantIdentity)) {
            auto* widget = remoteVideoWidgets_[track.participantIdentity];
            widget->setTrack(track.track);
            QString displayName = nameMap_.value(track.participantIdentity, track.participantIdentity);
            widget->setParticipantName(displayName);
            // Don't force camState here; let trackMutedStateChanged handle it
        }
        // Prefer auto-select first video participant only when nothing is pinned and no main is set
        if (!pinnedMain_ && mainParticipantId_.isEmpty()) {
            mainParticipantId_ = track.participantIdentity;
        }
    }
    // Audio state is handled by trackMutedStateChanged, no need to set here
}

void ConferenceWindow::onTrackUnsubscribed(const QString& trackSid, const QString& participantIdentity)
{
    Logger::instance().info(QString("Track unsubscribed: %1 from %2")
                           .arg(trackSid, participantIdentity));
    livekit::proto::TrackKind kind = livekit::proto::KIND_VIDEO;
    if (trackKinds_.contains(trackSid)) {
        kind = trackKinds_.take(trackSid);
    }

    if (kind == livekit::proto::KIND_VIDEO) {
        camState_[participantIdentity] = false;
        if (remoteVideoWidgets_.contains(participantIdentity)) {
            auto* widget = remoteVideoWidgets_[participantIdentity];
            widget->clearTrack();
        }
        refreshVideoWidgetState(participantIdentity);
    } else if (kind == livekit::proto::KIND_AUDIO) {
        micState_[participantIdentity] = false;
        lastAudioSeen_.remove(participantIdentity);
        refreshVideoWidgetState(participantIdentity);
    } else {
        // If kind unknown, clear video widget but keep states untouched
        if (remoteVideoWidgets_.contains(participantIdentity)) {
            auto* widget = remoteVideoWidgets_[participantIdentity];
            widget->clearTrack();
        }
    }

    if (mainParticipantId_ == participantIdentity) {
        mainVideoWidget_->clearTrack();
    }
    screenShareActive_.remove(participantIdentity);
    if (activeScreenShareId_ == participantIdentity) {
        activeScreenShareId_.clear();
    }
}

void ConferenceWindow::onChatMessageReceived(const ChatMessage& message)
{
    Logger::instance().debug("Chat message received from " + message.sender);
    chatPanel_->addMessage(message);
    
    if (!isChatVisible_) {
        // Simple indication that a message arrived if chat not visible
        // chatButton_->setStyleSheet("..."); // TODO: Add unread badge
    }
}

void ConferenceWindow::onVideoFrameReceived(const QString& participantIdentity,
                                            const QString& trackSid,
                                            const QImage& frame,
                                            livekit::TrackSource source)
{
    Q_UNUSED(trackSid);

    bool isScreenShare = (source == livekit::proto::SOURCE_SCREENSHARE || source == livekit::proto::SOURCE_SCREENSHARE_AUDIO);
    if (isScreenShare) {
        screenShareActive_[participantIdentity] = true;
        activeScreenShareId_ = participantIdentity;
    } else {
        camState_[participantIdentity] = true;
        lastVideoSeen_[participantIdentity] = QDateTime::currentDateTime();
        refreshVideoWidgetState(participantIdentity);
        // If this participant already has an active screenshare, ignore their camera frames to stop flicker
        if (screenShareActive_.value(participantIdentity, false)) {
            return;
        }
    }

    // Update caches/thumbnails only when not suppressed
    latestFrames_[participantIdentity] = frame;

    if (remoteVideoWidgets_.contains(participantIdentity)) {
        auto* widget = remoteVideoWidgets_[participantIdentity];
        widget->setParticipantName(nameMap_.value(participantIdentity, participantIdentity));
        widget->setVideoFrame(frame);
        refreshVideoWidgetState(participantIdentity);
    }

    // If user pinned someone, only update that participant
    if (pinnedMain_) {
        if (mainParticipantId_ == participantIdentity && mainVideoWidget_) {
            mainVideoWidget_->setVideoFrame(frame);
            mainVideoWidget_->setParticipantName(participantIdentity);
        }
        return;
    }

    // Auto-selection rules (no pin):
    // 1) If any screen share is active, show that participant's screen share only.
    // 2) Otherwise, stick to the first-set main participant; don't auto switch on every new participant.
    bool anyScreenShareActive = !activeScreenShareId_.isEmpty();

    if (isScreenShare) {
        activeScreenShareId_ = participantIdentity;
        mainParticipantId_ = participantIdentity;
    } else if (anyScreenShareActive) {
        // Ignore camera frames from others while a screen share is active
        return;
    } else if (mainParticipantId_.isEmpty()) {
        mainParticipantId_ = participantIdentity;
    } else if (mainParticipantId_ != participantIdentity) {
        // Keep current main when other camera frames arrive
        return;
    }

    if (mainVideoWidget_) {
        mainVideoWidget_->setVideoFrame(frame);
        mainVideoWidget_->setParticipantName(participantIdentity);
    }
}

// Media control handlers

void ConferenceWindow::onMicrophoneToggled()
{
    conferenceManager_->toggleMicrophone();
    updateControlButtons();
    micButton_->setToolTip(micButton_->isChecked() ? "Mute Microphone" : "Unmute Microphone");
    micState_["local"] = conferenceManager_->isMicrophoneEnabled();
    if (localVideoWidget_) {
        localVideoWidget_->setMicEnabled(conferenceManager_->isMicrophoneEnabled());
    }
    // Update local participant item in the list
    if (localParticipantItem_) {
        ParticipantInfo localInfo;
        localInfo.identity = "local";
        localInfo.name = userName_;
        localInfo.isMicrophoneEnabled = conferenceManager_->isMicrophoneEnabled();
        localInfo.isCameraEnabled = conferenceManager_->isCameraEnabled();
        localInfo.isScreenSharing = conferenceManager_->isScreenSharing();
        localParticipantItem_->updateInfo(localInfo);
    }
}

void ConferenceWindow::onCameraToggled()
{
    conferenceManager_->toggleCamera();
    updateControlButtons();
    camState_["local"] = conferenceManager_->isCameraEnabled();
    if (localVideoWidget_) {
        localVideoWidget_->setCameraEnabled(conferenceManager_->isCameraEnabled());
    }
    // Update local participant item in the list
    if (localParticipantItem_) {
        ParticipantInfo localInfo;
        localInfo.identity = "local";
        localInfo.name = userName_;
        localInfo.isMicrophoneEnabled = conferenceManager_->isMicrophoneEnabled();
        localInfo.isCameraEnabled = conferenceManager_->isCameraEnabled();
        localInfo.isScreenSharing = conferenceManager_->isScreenSharing();
        localParticipantItem_->updateInfo(localInfo);
    }
}

void ConferenceWindow::onScreenShareToggled()
{
    // If already sharing, a second click just stops sharing without dialog.
    if (conferenceManager_->isScreenSharing()) {
        conferenceManager_->toggleScreenShare();
        screenShareButton_->setChecked(false);
        updateControlButtons();
        return;
    }

    ScreenPickerDialog picker(this);
    if (picker.exec() == QDialog::Accepted) {
        if (picker.selectionType() == ScreenPickerDialog::SelectionType::Screen) {
            conferenceManager_->setScreenShareMode(ScreenCapturer::Mode::Screen,
                                                   picker.selectedScreen(),
                                                   0);
        } else if (picker.selectionType() == ScreenPickerDialog::SelectionType::Window) {
            conferenceManager_->setScreenShareMode(ScreenCapturer::Mode::Window,
                                                   nullptr,
                                                   picker.selectedWindow());
        }
        conferenceManager_->toggleScreenShare();
        screenShareButton_->setChecked(conferenceManager_->isScreenSharing());
    } else {
        screenShareButton_->setChecked(conferenceManager_->isScreenSharing());
    }
    updateControlButtons();
}

// UI control handlers

void ConferenceWindow::onChatToggled()
{
    isChatVisible_ = chatButton_->isChecked();
    
    if (isChatVisible_) {
        sidebarTitleLabel_->setText("Chat");
        participantsPanel_->hide();
        chatPanel_->show();
        participantsButton_->setChecked(false);
        isParticipantsVisible_ = false;
    } else {
        chatPanel_->hide();
    }
    updateLayout();
}

void ConferenceWindow::onParticipantsToggled()
{
    isParticipantsVisible_ = participantsButton_->isChecked();
    
    if (isParticipantsVisible_) {
        sidebarTitleLabel_->setText("Participants");
        chatPanel_->hide();
        participantsPanel_->show();
        chatButton_->setChecked(false);
        isChatVisible_ = false;
    } else {
        participantsPanel_->hide();
    }
    updateLayout();
}

void ConferenceWindow::onLeaveClicked()
{
    close();
}

void ConferenceWindow::onSendChatMessage(const QString& message)
{
    conferenceManager_->sendChatMessage(message);
}

bool ConferenceWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == topBar_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                dragging_ = true;
                dragPos_ = me->globalPosition().toPoint() - frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && dragging_) {
            auto* me = static_cast<QMouseEvent*>(event);
            move(me->globalPosition().toPoint() - dragPos_);
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            dragging_ = false;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        // Handle clicking on thumbnails to pin
        for (auto it = remoteVideoWidgets_.cbegin(); it != remoteVideoWidgets_.cend(); ++it) {
            if (it.value() == watched) {
                mainParticipantId_ = it.key();
                pinnedMain_ = true;
                mainVideoWidget_->setParticipantName(mainParticipantId_);
                if (latestFrames_.contains(mainParticipantId_)) {
                    mainVideoWidget_->setVideoFrame(latestFrames_.value(mainParticipantId_));
                } else {
                    mainVideoWidget_->clearTrack();
                }
                return true;
            }
        }
    if (watched == localVideoWidget_) {
        mainParticipantId_ = "local";
        pinnedMain_ = true;
        mainVideoWidget_->setParticipantName(userName_ + " (You)");
        if (latestFrames_.contains("local")) {
            mainVideoWidget_->setVideoFrame(latestFrames_.value("local"));
        } else {
            mainVideoWidget_->clearTrack();
        }
        return true;
    } else if (watched == localScreenWidget_) {
        mainParticipantId_ = "local_screen";
        pinnedMain_ = true;
        activeScreenShareId_ = "local_screen";
        if (latestFrames_.contains("local_screen")) {
            mainVideoWidget_->setVideoFrame(latestFrames_.value("local_screen"));
            mainVideoWidget_->setParticipantName("Screen Share");
        } else {
            mainVideoWidget_->clearTrack();
        }
        return true;
    }
}
return QMainWindow::eventFilter(watched, event);
}

void ConferenceWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->position().y() <= topBar_->height()) {
        dragging_ = true;
        dragPos_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QMainWindow::mousePressEvent(event);
}

void ConferenceWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_) {
        move(event->globalPosition().toPoint() - dragPos_);
        event->accept();
        return;
    }
    QMainWindow::mouseMoveEvent(event);
}

void ConferenceWindow::mouseReleaseEvent(QMouseEvent* event)
{
    dragging_ = false;
    QMainWindow::mouseReleaseEvent(event);
}

void ConferenceWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->position().y() <= topBar_->height()) {
        toggleFullscreen();
        event->accept();
        return;
    }
    QMainWindow::mouseDoubleClickEvent(event);
}

void ConferenceWindow::updateParticipantCount()
{
    int count = conferenceManager_->getParticipantCount();
    QString text = (count == 1) ? "1 participant" : QString("%1 participants").arg(count);
    participantCountLabel_->setText(text);
}

void ConferenceWindow::updateControlButtons()
{
    micButton_->setChecked(conferenceManager_->isMicrophoneEnabled());
    cameraButton_->setChecked(conferenceManager_->isCameraEnabled());
    screenShareButton_->setChecked(conferenceManager_->isScreenSharing());

    micButton_->setIcon(conferenceManager_->isMicrophoneEnabled()
                        ? QIcon(":/icon/Turn_on_the_microphone.png")
                        : QIcon(":/icon/mute_the_microphone.png"));

    cameraButton_->setIcon(conferenceManager_->isCameraEnabled()
                           ? QIcon(":/icon/video.png")
                           : QIcon(":/icon/close_video.png"));

    screenShareButton_->setIcon(QIcon(":/icon/screen_sharing.png"));
    chatButton_->setIcon(QIcon(":/icon/message.png"));
    participantsButton_->setIcon(QIcon(":/icon/user.png"));
}

void ConferenceWindow::updateParticipantMediaState(const QString& id, bool micOn, bool camOn)
{
    micState_[id] = micOn;
    camState_[id] = camOn;
    refreshVideoWidgetState(id);
}

void ConferenceWindow::refreshVideoWidgetState(const QString& id)
{
    Logger::instance().info(QString("refreshVideoWidgetState: id=%1, micState=%2, camState=%3")
        .arg(id).arg(micState_.value(id, false)).arg(camState_.value(id, false)));
    
    if (id == "local" && localVideoWidget_) {
        localVideoWidget_->setMicEnabled(micState_.value(id, false));
        localVideoWidget_->setCameraEnabled(camState_.value(id, false));
    }
    if (remoteVideoWidgets_.contains(id)) {
        auto* widget = remoteVideoWidgets_.value(id);
        bool micEnabled = micState_.value(id, false);
        bool camEnabled = camState_.value(id, false);
        Logger::instance().info(QString("Setting widget state: mic=%1, cam=%2").arg(micEnabled).arg(camEnabled));
        widget->setMicEnabled(micEnabled);
        widget->setCameraEnabled(camEnabled);
    }
}

void ConferenceWindow::updateLastSeen(const QString& id, bool isAudio)
{
    auto now = QDateTime::currentDateTime();
    if (isAudio) {
        lastAudioSeen_[id] = now;
    } else {
        lastVideoSeen_[id] = now;
    }
}

void ConferenceWindow::startInactivityTimer()
{
    if (!inactivityTimer_) {
        inactivityTimer_ = new QTimer(this);
        inactivityTimer_->setInterval(2000);
        connect(inactivityTimer_, &QTimer::timeout, this, &ConferenceWindow::handleInactivityCheck);
    }
    inactivityTimer_->start();
}

void ConferenceWindow::handleInactivityCheck()
{
    auto now = QDateTime::currentDateTime();
    const int graceMs = 5000;
    
    // Audio inactivity: if no audio seen for grace period, mark mic as off.
    for (auto it = lastAudioSeen_.begin(); it != lastAudioSeen_.end(); ++it) {
        if (it.value().msecsTo(now) > graceMs) {
            if (micState_.value(it.key(), true)) {
                micState_[it.key()] = false;
                refreshVideoWidgetState(it.key());
            }
        }
    }

    // Video inactivity fallback unchanged
    for (auto it = lastVideoSeen_.begin(); it != lastVideoSeen_.end(); ++it) {
        if (it.value().msecsTo(now) > graceMs) {
            camState_[it.key()] = false;
            refreshVideoWidgetState(it.key());
        }
    }
}

void ConferenceWindow::toggleFullscreen()
{
    isFullscreen_ = !isFullscreen_;
    if (isFullscreen_) {
        showFullScreen();
        fullscreenButton_->setToolTip("Exit Fullscreen");
    } else {
        showNormal();
        fullscreenButton_->setToolTip("Enter Fullscreen");
    }
    applyStyles();
    updateLayout();
}

void ConferenceWindow::updateWindowFrameRadius()
{
    // Re-apply style to adjust rounded corners based on fullscreen state
    applyStyles();
}

void ConferenceWindow::onParticipantMicToggle(const QString& identity)
{
    Logger::instance().info(QString("Mic toggle clicked for participant: %1").arg(identity));
    
    if (isLocalUserHost_) {
        // Host: Send mute request via RPC
        QJsonObject payload;
        payload["action"] = "mute_audio";
        payload["target_identity"] = identity;
        payload["muted"] = micState_.value(identity, false); // Toggle current state
        
        QJsonDocument doc(payload);
        std::string jsonStr = doc.toJson(QJsonDocument::Compact).toStdString();
        std::vector<uint8_t> data(jsonStr.begin(), jsonStr.end());
        
        // Send via data channel
        if (conferenceManager_ && conferenceManager_->isConnected()) {
            // PublishData to "room.control" topic
            // Note: This requires server-side handling
            Logger::instance().info(QString("Sending mute request for %1").arg(identity));
        }
    } else {
        // Participant: Toggle local playback of this participant's audio
        bool currentlyMuted = mutedParticipants_.value(identity, false);
        mutedParticipants_[identity] = !currentlyMuted;
        
        Logger::instance().info(QString("Local mute toggled for %1: %2")
            .arg(identity).arg(!currentlyMuted ? "muted" : "unmuted"));
        
        // Note: Actual audio muting would require AudioSink control
        // which is handled in ConferenceManager
    }
}

void ConferenceWindow::onParticipantCameraToggle(const QString& identity)
{
    Logger::instance().info(QString("Camera toggle clicked for participant: %1").arg(identity));
    
    if (isLocalUserHost_) {
        // Host: Send camera disable request via RPC
        QJsonObject payload;
        payload["action"] = "disable_video";
        payload["target_identity"] = identity;
        payload["disabled"] = camState_.value(identity, false); // Toggle current state
        
        QJsonDocument doc(payload);
        std::string jsonStr = doc.toJson(QJsonDocument::Compact).toStdString();
        std::vector<uint8_t> data(jsonStr.begin(), jsonStr.end());
        
        if (conferenceManager_ && conferenceManager_->isConnected()) {
            Logger::instance().info(QString("Sending camera disable request for %1").arg(identity));
        }
    } else {
        // Participant: Toggle visibility of this participant's video
        bool currentlyHidden = hiddenVideoParticipants_.value(identity, false);
        hiddenVideoParticipants_[identity] = !currentlyHidden;
        
        // Hide or show the video widget for this participant
        if (remoteVideoWidgets_.contains(identity)) {
            remoteVideoWidgets_[identity]->setVisible(currentlyHidden);
        }
        
        Logger::instance().info(QString("Local video visibility toggled for %1: %2")
            .arg(identity).arg(currentlyHidden ? "visible" : "hidden"));
    }
}

void ConferenceWindow::onParticipantKick(const QString& identity)
{
    Logger::instance().info(QString("Kick clicked for participant: %1").arg(identity));
    
    if (!isLocalUserHost_) {
        Logger::instance().warning("Only hosts can kick participants");
        return;
    }
    
    // Confirm kick action
    auto reply = QMessageBox::question(this, tr("Kick Participant"),
        tr("Are you sure you want to kick %1 from the meeting?").arg(nameMap_.value(identity, identity)),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Create network client and call kick API using the correct signaling server URL
    auto* networkClient = new NetworkClient(this);
    QString apiUrl = Settings::instance().getSignalingServerUrl();
    networkClient->setApiUrl(apiUrl);
    
    Logger::instance().info(QString("Calling kick API at: %1 for participant: %2").arg(apiUrl, identity));
    
    // Call the server kick API
    networkClient->kickParticipant(roomName_, identity);
    
    // Clean up network client after a delay
    QTimer::singleShot(5000, networkClient, &QObject::deleteLater);
}
