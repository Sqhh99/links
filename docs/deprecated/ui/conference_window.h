#ifndef CONFERENCE_WINDOW_H
#define CONFERENCE_WINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QMap>
#include <QString>
#include <QImage>
#include <QMouseEvent>
#include <QTimer>
#include <QDateTime>
#include "video_widget.h"
#include "gl_video_widget.h"
#include "participant_item.h"
#include "chat_panel.h"
#include "../core/conference_manager.h"
#include "../core/network_client.h"
#include "WindowEffect.h"

class ConferenceWindow : public QMainWindow
{
    Q_OBJECT
    bool eventFilter(QObject* watched, QEvent* event) override;
    
public:
    explicit ConferenceWindow(const QString& url,
                             const QString& token,
                             const QString& roomName,
                             const QString& userName,
                             bool isHost = false,
                             QWidget* parent = nullptr);
    ~ConferenceWindow() override;
    
protected:
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    
private slots:
    // Conference events
    void onConnected();
    void onDisconnected();
    void onConnectionStateChanged(livekit::ConnectionState state);
    void onParticipantJoined(const ParticipantInfo& info);
    void onParticipantLeft(const QString& identity);
    void onTrackSubscribed(const TrackInfo& track);
    void onTrackUnsubscribed(const QString& trackSid, const QString& participantIdentity);
    void onChatMessageReceived(const ChatMessage& message);
    void onVideoFrameReceived(const QString& participantIdentity,
                              const QString& trackSid,
                              const QImage& frame,
                              livekit::TrackSource source);
    
    // Media controls
    void onMicrophoneToggled();
    void onCameraToggled();
    void onScreenShareToggled();
    
    // UI controls
    void onChatToggled();
    void onParticipantsToggled();
    void onLeaveClicked();
    void onSendChatMessage(const QString& message);
    void updateParticipantMediaState(const QString& id, bool micOn, bool camOn);
    void refreshVideoWidgetState(const QString& id);
    void updateLastSeen(const QString& id, bool isAudio);
    void startInactivityTimer();
    void handleInactivityCheck();
    
    // Participant item button handlers
    void onParticipantMicToggle(const QString& identity);
    void onParticipantCameraToggle(const QString& identity);
    void onParticipantKick(const QString& identity);

private:
    void setupUI();
    void setupConnections();
    void applyStyles();
    void updateParticipantCount();
    void updateControlButtons();
    void toggleFullscreen();
    void updateWindowFrameRadius();
    
    // Conference
    ConferenceManager* conferenceManager_;
    QString url_;
    QString token_;
    QString roomName_;
    QString userName_;
    
    // UI - Main layout
    QWidget* centralWidget_;
    QHBoxLayout* mainLayout_;
    
    // UI - Left sidebar (local + remote videos)
    QWidget* videoSidebar_;
    GLVideoWidget* localVideoWidget_;
    QScrollArea* remoteVideosScroll_;
    QWidget* remoteVideosContainer_;
    QVBoxLayout* remoteVideosLayout_;
    
    // UI - Center (main video + controls)
    QWidget* centerPanel_;
    GLVideoWidget* mainVideoWidget_;
    QWidget* controlBar_;
    QPushButton* micButton_;
    QPushButton* cameraButton_;
    QPushButton* screenShareButton_;
    QPushButton* chatButton_;
    QPushButton* participantsButton_;
    QPushButton* fullscreenButton_;
    QPushButton* settingsButton_;
    QPushButton* sidebarToggleButton_{nullptr};
    QPushButton* leaveButton_;
    
    // UI - Right sidebar (participants/chat)
    QWidget* rightSidebar_;
    QWidget* participantsPanel_;
    QScrollArea* participantsScroll_;
    QWidget* participantsContainer_;
    QVBoxLayout* participantsLayout_;
    ChatPanel* chatPanel_;
    
    // UI - Top bar
    QWidget* topBar_;
    QPushButton* minimizeButton_;
    QPushButton* alwaysOnTopButton_;
    QPushButton* closeButton_;
    QLabel* roomNameLabel_;
    QLabel* participantCountLabel_;
    QLabel* connectionStatusLabel_;
    QLabel* sidebarTitleLabel_;
    
    // State
    QMap<QString, GLVideoWidget*> remoteVideoWidgets_;
    QMap<QString, ParticipantItem*> participantItems_;
    ParticipantItem* localParticipantItem_{nullptr};
    bool isChatVisible_;
    bool isParticipantsVisible_;
    bool sidebarVisible_{true};
    QString mainParticipantId_;
    QMap<QString, bool> screenShareActive_;
    QMap<QString, bool> micState_;
    QMap<QString, bool> camState_;
    QMap<QString, QString> nameMap_;
    QMap<QString, livekit::proto::TrackKind> trackKinds_;
    QMap<QString, QDateTime> lastAudioSeen_;
    QMap<QString, QDateTime> lastVideoSeen_;
    QTimer* inactivityTimer_{nullptr};
    bool pinnedMain_{false};
    QMap<QString, QImage> latestFrames_;
    QString activeScreenShareId_;
    bool alwaysOnTop_{false};
    bool isFullscreen_{false};
    bool dragging_{false};
    QPoint dragPos_;
    int cornerRadius_{12};
    WindowEffect windowEffect_;
    GLVideoWidget* localScreenWidget_{nullptr};
    QLabel* screenLabel_{nullptr};
    bool isLocalUserHost_{false};
    QMap<QString, bool> mutedParticipants_;
    QMap<QString, bool> hiddenVideoParticipants_;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateLayout();
};

#endif // CONFERENCE_WINDOW_H
