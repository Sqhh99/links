#include "login_window.h"
#include "conference_window.h"
#include "../utils/logger.h"
#include "../utils/settings.h"
#include "settings_dialog.h"
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QDateTime>
#include <QFrame>
#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include <QTimer>

LoginWindow::LoginWindow(QWidget* parent)
    : QMainWindow(parent),
      networkClient_(new NetworkClient(this)),
      isLoading_(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
#ifdef Q_OS_WIN
    windowEffect_.setAcrylicEffect(HWND(winId()), QColor(15, 15, 24, 180));
#endif
    setupUI();
    setupConnections();
    applyStyles();
    
    // Clock
    clockTimer_ = new QTimer(this);
    clockTimer_->setInterval(1000);
    connect(clockTimer_, &QTimer::timeout, this, [this]() {
        auto now = QDateTime::currentDateTime();
        timeLabel_->setText(now.toString("HH:mm"));
        dateLabel_->setText(now.toString("yyyy年MM月dd日 dddd"));
    });
    clockTimer_->start();
    auto now = QDateTime::currentDateTime();
    timeLabel_->setText(now.toString("HH:mm"));
    dateLabel_->setText(now.toString("yyyy年MM月dd日 dddd"));
    
    // Load saved settings
    userNameInput_->setText(Settings::instance().getLastUserName());
    roomNameInput_->setText(Settings::instance().getLastRoomName());
    
    // Set API URL from settings (use signaling server URL as the API endpoint)
    networkClient_->setApiUrl(Settings::instance().getSignalingServerUrl());
    
    Logger::instance().info("LoginWindow created");
}

LoginWindow::~LoginWindow()
{
}

void LoginWindow::setupUI()
{
    setWindowTitle("LiveKit Conference - Join Meeting");
    resize(900, 620);
    
    centralWidget_ = new QWidget(this);
    centralWidget_->setObjectName("windowFrame");
    setCentralWidget(centralWidget_);
    
    auto* mainLayout = new QVBoxLayout(centralWidget_);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Title bar
    titleBar_ = new QWidget(this);
    titleBar_->setObjectName("titleBar");
    titleBar_->setFixedHeight(48);
    auto* titleLayout = new QHBoxLayout(titleBar_);
    titleLayout->setContentsMargins(14, 0, 14, 0);
    titleLayout->setSpacing(10);

    auto* titleIcon = new QLabel(this);
    QPixmap titlePx(":/icon/video.png");
    if (!titlePx.isNull()) {
        titleIcon->setPixmap(titlePx.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        titleIcon->setText("LK");
    }

    titleLabel_ = new QLabel("LiveKit Conference", this);
    titleLabel_->setObjectName("titleBarLabel");

    titleLayout->addWidget(titleIcon);
    titleLayout->addWidget(titleLabel_);
    titleLayout->addStretch();

    settingsButton_ = new QPushButton(this);
    settingsButton_->setObjectName("titleButton");
    settingsButton_->setIcon(QIcon(":/icon/set_up.png"));
    settingsButton_->setFixedSize(32, 24);
    settingsButton_->setIconSize(QSize(14, 14));
    settingsButton_->setToolTip("Settings");

    minimizeButton_ = new QPushButton(this);
    minimizeButton_->setObjectName("titleButton");
    minimizeButton_->setIcon(QIcon(":/icon/minimize.png"));
    minimizeButton_->setFixedSize(32, 24);
    minimizeButton_->setIconSize(QSize(14, 14));

    closeButton_ = new QPushButton(this);
    closeButton_->setObjectName("titleButtonClose");
    closeButton_->setIcon(QIcon(":/icon/close.png"));
    closeButton_->setFixedSize(32, 24);
    closeButton_->setIconSize(QSize(14, 14));

    titleLayout->addWidget(settingsButton_);
    titleLayout->addWidget(minimizeButton_);
    titleLayout->addWidget(closeButton_);

    titleBar_->installEventFilter(this);
    mainLayout->addWidget(titleBar_);

    // Body layout
    auto* bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(18, 12, 18, 18);
    bodyLayout->setSpacing(16);
    mainLayout->addLayout(bodyLayout);

    // Hero panel (left)
    heroPanel_ = new QWidget(this);
    heroPanel_->setObjectName("heroPanel");
    auto* heroLayout = new QVBoxLayout(heroPanel_);
    heroLayout->setContentsMargins(18, 18, 18, 18);
    heroLayout->setSpacing(16);

    auto* brandLayout = new QHBoxLayout();
    brandLayout->setSpacing(10);
    auto* brandBadge = new QLabel(heroPanel_);
    brandBadge->setObjectName("brandBadge");
    brandBadge->setText("LIVE");
    brandBadge->setAlignment(Qt::AlignCenter);
    brandBadge->setFixedSize(48, 32);
    auto* brandName = new QLabel("CloudMeet", heroPanel_);
    brandName->setObjectName("brandName");
    brandLayout->addWidget(brandBadge);
    brandLayout->addWidget(brandName);
    brandLayout->addStretch();
    heroLayout->addLayout(brandLayout);

    timeLabel_ = new QLabel(heroPanel_);
    timeLabel_->setObjectName("timeLabel");
    dateLabel_ = new QLabel(heroPanel_);
    dateLabel_->setObjectName("dateLabel");
    heroLayout->addWidget(timeLabel_);
    heroLayout->addWidget(dateLabel_);

    highlightLabel_ = new QLabel("下一场会议：产品评审\n10:00 - 11:00", heroPanel_);
    highlightLabel_->setObjectName("highlightCard");
    highlightLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    highlightLabel_->setWordWrap(true);
    heroLayout->addWidget(highlightLabel_);

    auto* bulletLayout = new QVBoxLayout();
    bulletLayout->setSpacing(8);
    QStringList bullets = {
        "低延迟高清音视频",
        "一键屏幕共享与录制",
        "端到端加密与入会鉴权"
    };
    for (const auto& b : bullets) {
        auto* row = new QHBoxLayout();
        auto* dot = new QLabel(heroPanel_);
        dot->setObjectName("dot");
        dot->setFixedSize(10, 10);
        auto* text = new QLabel(b, heroPanel_);
        text->setObjectName("bulletText");
        row->addWidget(dot);
        row->addWidget(text);
        row->addStretch();
        bulletLayout->addLayout(row);
    }
    heroLayout->addLayout(bulletLayout);
    heroLayout->addStretch();

    bodyLayout->addWidget(heroPanel_, 4);

    // Right card (forms)
    auto* card = new QFrame(this);
    card->setObjectName("loginCard");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 28, 28, 24);
    cardLayout->setSpacing(16);

    // Tabs
    auto* tabLayout = new QHBoxLayout();
    tabLayout->setSpacing(10);
    joinTabButton_ = new QPushButton("加入会议", this);
    startTabButton_ = new QPushButton("快速会议", this);
    scheduleTabButton_ = new QPushButton("预定会议", this);
    auto makeTab = [](QPushButton* btn) {
        btn->setObjectName("tabButton");
        btn->setCheckable(true);
        btn->setMinimumHeight(36);
    };
    makeTab(joinTabButton_);
    makeTab(startTabButton_);
    makeTab(scheduleTabButton_);
    joinTabButton_->setChecked(true);
    tabLayout->addWidget(joinTabButton_);
    tabLayout->addWidget(startTabButton_);
    tabLayout->addWidget(scheduleTabButton_);
    tabLayout->addStretch();
    cardLayout->addLayout(tabLayout);

    formStack_ = new QStackedWidget(card);

    // Join form
    auto* joinPage = new QWidget(card);
    auto* joinLayout = new QVBoxLayout(joinPage);
    joinLayout->setSpacing(12);
    auto* nameLabel = new QLabel("显示名称", joinPage);
    nameLabel->setObjectName("fieldLabel");
    userNameInput_ = new QLineEdit(joinPage);
    userNameInput_->setPlaceholderText("e.g. Alice Smith");
    userNameInput_->setMinimumHeight(44);
    auto* roomLabel = new QLabel("会议号 / 房间名", joinPage);
    roomLabel->setObjectName("fieldLabel");
    roomNameInput_ = new QLineEdit(joinPage);
    roomNameInput_->setPlaceholderText("如 daily-standup 或会议号");
    roomNameInput_->setMinimumHeight(44);

    auto* togglesLayout = new QHBoxLayout();
    togglesLayout->setSpacing(10);
    micToggleButton_ = new QPushButton("麦克风关", joinPage);
    micToggleButton_->setObjectName("pillToggle");
    micToggleButton_->setCheckable(true);
    micToggleButton_->setMinimumHeight(42);
    camToggleButton_ = new QPushButton("摄像头关", joinPage);
    camToggleButton_->setObjectName("pillToggle");
    camToggleButton_->setCheckable(true);
    camToggleButton_->setMinimumHeight(42);
    togglesLayout->addWidget(micToggleButton_);
    togglesLayout->addWidget(camToggleButton_);

    joinButton_ = new QPushButton("进入会议", joinPage);
    joinButton_->setObjectName("primaryButton");
    joinButton_->setMinimumHeight(46);
    joinButton_->setCursor(Qt::PointingHandCursor);

    joinLayout->addWidget(nameLabel);
    joinLayout->addWidget(userNameInput_);
    joinLayout->addWidget(roomLabel);
    joinLayout->addWidget(roomNameInput_);
    joinLayout->addLayout(togglesLayout);
    joinLayout->addWidget(joinButton_);
    joinLayout->addStretch();

    // Quick start page
    auto* startPage = new QWidget(card);
    auto* startLayout = new QVBoxLayout(startPage);
    startLayout->setSpacing(12);
    auto* startLabel = new QLabel("一键创建临时会议", startPage);
    startLabel->setObjectName("fieldLabel");
    auto* startDesc = new QLabel("将自动生成房间名，分享给参会者即可入会。", startPage);
    startDesc->setObjectName("hintLabel");
    startDesc->setWordWrap(true);
    quickJoinButton_ = new QPushButton("立即创建并进入", startPage);
    quickJoinButton_->setObjectName("primaryButton");
    quickJoinButton_->setMinimumHeight(46);
    startLayout->addWidget(startLabel);
    startLayout->addWidget(startDesc);
    startLayout->addSpacing(8);
    startLayout->addWidget(quickJoinButton_);
    startLayout->addStretch();

    // Schedule page
    auto* schedulePage = new QWidget(card);
    auto* scheduleLayout = new QVBoxLayout(schedulePage);
    scheduleLayout->setSpacing(12);
    auto* schedTitle = new QLabel("预定会议", schedulePage);
    schedTitle->setObjectName("fieldLabel");
    scheduledTimeInput_ = new QLineEdit(schedulePage);
    scheduledTimeInput_->setPlaceholderText("填写预定时间说明，例如：明日 10:00");
    scheduledTimeInput_->setMinimumHeight(44);
    auto* schedDesc = new QLabel("预定后将生成会议号并保存到本地提醒。", schedulePage);
    schedDesc->setObjectName("hintLabel");
    schedDesc->setWordWrap(true);
    createRoomButton_ = new QPushButton("预定并生成会议", schedulePage);
    createRoomButton_->setObjectName("primaryButton");
    createRoomButton_->setMinimumHeight(46);
    scheduleLayout->addWidget(schedTitle);
    scheduleLayout->addWidget(scheduledTimeInput_);
    scheduleLayout->addWidget(schedDesc);
    scheduleLayout->addWidget(createRoomButton_);
    scheduleLayout->addStretch();

    formStack_->addWidget(joinPage);
    formStack_->addWidget(startPage);
    formStack_->addWidget(schedulePage);
    cardLayout->addWidget(formStack_, 1);

    // Status + loading
    statusLabel_ = new QLabel("", card);
    statusLabel_->setObjectName("statusLabel");
    statusLabel_->setAlignment(Qt::AlignLeft);
    statusLabel_->setWordWrap(true);
    cardLayout->addWidget(statusLabel_);

    loadingWidget_ = new QWidget(card);
    loadingWidget_->setObjectName("loadingWidget");
    auto* loadingLayout = new QHBoxLayout(loadingWidget_);
    loadingLayout->setContentsMargins(0, 0, 0, 0);
    loadingLayout->setAlignment(Qt::AlignLeft);
    auto* loadingLabel = new QLabel("正在连接 LiveKit...", card);
    loadingLabel->setObjectName("loadingLabel");
    loadingLayout->addWidget(loadingLabel);
    loadingWidget_->hide();
    cardLayout->addWidget(loadingWidget_);

    bodyLayout->addWidget(card, 5);
}

void LoginWindow::setupConnections()
{
    connect(minimizeButton_, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(closeButton_, &QPushButton::clicked, this, &QWidget::close);
    connect(settingsButton_, &QPushButton::clicked, this, &LoginWindow::showSettingsDialog);
    connect(joinButton_, &QPushButton::clicked, this, &LoginWindow::onJoinClicked);
    connect(quickJoinButton_, &QPushButton::clicked, this, &LoginWindow::onQuickJoinClicked);
    connect(createRoomButton_, &QPushButton::clicked, this, &LoginWindow::onCreateRoomClicked);
    connect(joinTabButton_, &QPushButton::clicked, this, [this]() { switchTab(0); });
    connect(startTabButton_, &QPushButton::clicked, this, [this]() { switchTab(1); });
    connect(scheduleTabButton_, &QPushButton::clicked, this, [this]() { switchTab(2); });
    connect(micToggleButton_, &QPushButton::toggled, this, [this](bool on) {
        micToggleButton_->setText(on ? "麦克风开" : "麦克风关");
    });
    connect(camToggleButton_, &QPushButton::toggled, this, [this](bool on) {
        camToggleButton_->setText(on ? "摄像头开" : "摄像头关");
    });
    
    connect(networkClient_, &NetworkClient::tokenReceived, this, &LoginWindow::onTokenReceived);
    connect(networkClient_, &NetworkClient::error, this, &LoginWindow::onNetworkError);
    
    // Enter key to join
    connect(userNameInput_, &QLineEdit::returnPressed, this, &LoginWindow::onJoinClicked);
    connect(roomNameInput_, &QLineEdit::returnPressed, this, &LoginWindow::onJoinClicked);
}

void LoginWindow::applyStyles()
{
    QString styleSheet = QString(R"(
        QMainWindow {
            background: transparent;
        }

        #windowFrame {
            background-color: #0f1116;
            border-radius: %1px;
            border: 1px solid #1f2230;
        }

        #titleBar {
            background-color: transparent;
        }

        #titleBarLabel {
            color: #e5e5f0;
            font-size: 14px;
            font-weight: 600;
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
        
        #heroPanel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1a1f3b, stop:1 #111526);
            border-radius: 14px;
            border: 1px solid #1f2644;
        }

        #brandBadge {
            background: #5865f2;
            color: white;
            font-weight: 800;
            border-radius: 10px;
            letter-spacing: 1px;
        }

        #brandName {
            color: #f5f6ff;
            font-size: 18px;
            font-weight: 800;
        }

        #timeLabel {
            color: #f8fafc;
            font-size: 44px;
            font-weight: 300;
            letter-spacing: -1px;
        }

        #dateLabel {
            color: #c2c7d6;
            font-size: 14px;
        }

        #highlightCard {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.07);
            border-radius: 12px;
            padding: 12px 14px;
            color: #e6e9f5;
            font-weight: 600;
        }

        #dot {
            background: #7bd44a;
            border-radius: 6px;
        }

        #bulletText {
            color: #d8dded;
            font-size: 13px;
        }

        #loginCard {
            background-color: #11131d;
            border-radius: 14px;
            border: 1px solid #1f2230;
        }

        #tabButton {
            background: rgba(255,255,255,0.02);
            color: #9ea3b6;
            border: 1px solid #1f2230;
            border-radius: 10px;
            padding: 8px 14px;
        }
        #tabButton:checked {
            background: #5865f2;
            color: white;
            border-color: #5865f2;
        }

        #fieldLabel {
            color: #cfcfd8;
            font-size: 13px;
            font-weight: 600;
            margin-bottom: 2px;
        }

        #hintLabel {
            color: #8c90a3;
            font-size: 12px;
        }
        
        QLineEdit {
            background-color: #0e0e14;
            color: #ffffff;
            border: 1px solid #2a2a35;
            border-radius: 10px;
            padding: 0 16px;
            font-size: 15px;
            selection-background-color: #5865f2;
        }
        
        QLineEdit:focus {
            border: 1px solid #5865f2;
            background-color: #13131a;
        }
        
        #primaryButton {
            background-color: #5865f2;
            color: #ffffff;
            border: none;
            border-radius: 10px;
            font-size: 15px;
            font-weight: 700;
        }
        
        #primaryButton:hover {
            background-color: #4752c4;
        }
        
        #primaryButton:pressed {
            background-color: #3c45a5;
        }
        
        #primaryButton:disabled {
            background-color: #2a2a35;
            color: #6a6a7e;
        }

        #pillToggle {
            background: rgba(255,255,255,0.05);
            border: 1px solid #2a2a35;
            border-radius: 999px;
            color: #c0c3d3;
            padding: 8px 14px;
            font-weight: 600;
        }
        #pillToggle:checked {
            background: #2e9b6c;
            border-color: #2e9b6c;
            color: white;
        }
        
        #statusLabel {
            color: #ff5252;
            font-size: 13px;
            font-weight: 500;
        }
        
        #loadingLabel {
            color: #5865f2;
            font-size: 13px;
            font-weight: 500;
        }
    )").arg(cornerRadius_);
    
    setStyleSheet(styleSheet);
}

void LoginWindow::onJoinClicked()
{
    QString userName = userNameInput_->text().trimmed();
    QString roomName = roomNameInput_->text().trimmed();
    
    if (userName.isEmpty()) {
        showError("Please enter your name");
        userNameInput_->setFocus();
        return;
    }
    
    if (roomName.isEmpty()) {
        showError("Please enter a room name");
        roomNameInput_->setFocus();
        return;
    }
    
    // Save settings
    Settings::instance().setLastUserName(userName);
    Settings::instance().setLastRoomName(roomName);
    
    Logger::instance().info(QString("Requesting token for room '%1', user '%2'")
                           .arg(roomName, userName));
    
    showLoading(true);
    networkClient_->requestToken(roomName, userName);
}

void LoginWindow::onQuickJoinClicked()
{
    QString userName = userNameInput_->text().trimmed();
    
    if (userName.isEmpty()) {
        showError("Please enter your name");
        userNameInput_->setFocus();
        return;
    }
    
    // Generate random room name
    QString randomRoom = QString("room-%1").arg(QDateTime::currentMSecsSinceEpoch());
    roomNameInput_->setText(randomRoom);
    
    onJoinClicked();
}

void LoginWindow::onCreateRoomClicked()
{
    QString userName = userNameInput_->text().trimmed();
    
    if (userName.isEmpty()) {
        showError("Please enter your name");
        userNameInput_->setFocus();
        return;
    }
    
    QString note = scheduledTimeInput_ ? scheduledTimeInput_->text().trimmed() : QString();
    QString suffix = note.isEmpty() ? QString::number(QDateTime::currentMSecsSinceEpoch())
                                    : note.simplified().replace(" ", "-");
    QString privateRoom = QString("scheduled-%1").arg(suffix);
    roomNameInput_->setText(privateRoom);
    
    onJoinClicked();
}

void LoginWindow::onTokenReceived(const TokenResponse& response)
{
    showLoading(false);
    
    if (!response.success) {
        showError("Failed to get token: " + response.error);
        Logger::instance().error("Token request failed: " + response.error);
        return;
    }
    
    Logger::instance().info("Token received, joining conference");
    joinConference(response);
}

void LoginWindow::onNetworkError(const QString& error)
{
    showLoading(false);
    showError("Network error: " + error);
}

void LoginWindow::showSettingsDialog()
{
    if (!settingsDialog_) {
        settingsDialog_ = new SettingsDialog(this);
    }
    settingsDialog_->setApiUrl(networkClient_->getApiUrl());
    if (settingsDialog_->exec() == QDialog::Accepted) {
        networkClient_->setApiUrl(settingsDialog_->apiUrl());
    }
}

void LoginWindow::switchTab(int index)
{
    formStack_->setCurrentIndex(index);
    joinTabButton_->setChecked(index == 0);
    startTabButton_->setChecked(index == 1);
    scheduleTabButton_->setChecked(index == 2);
}

void LoginWindow::showError(const QString& message)
{
    statusLabel_->setText(message);
    statusLabel_->setStyleSheet("color: #ff5252;");
}

void LoginWindow::showLoading(bool show)
{
    isLoading_ = show;
    loadingWidget_->setVisible(show);
    joinButton_->setEnabled(!show);
    quickJoinButton_->setEnabled(!show);
    createRoomButton_->setEnabled(!show);
    
    if (show) {
        statusLabel_->setText("");
    }
}

void LoginWindow::joinConference(const TokenResponse& response)
{
    // Create and show conference window
    auto* conferenceWindow = new ConferenceWindow(
        response.url,
        response.token,
        response.roomName,
        userNameInput_->text(),
        response.isHost  // Pass host status from server
    );
    
    conferenceWindow->show();
    
    // Close login window
    close();
}

bool LoginWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == titleBar_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                dragging_ = true;
                dragPos_ = me->globalPosition().toPoint() - frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (dragging_) {
                auto* me = static_cast<QMouseEvent*>(event);
                move(me->globalPosition().toPoint() - dragPos_);
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            dragging_ = false;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
