#ifndef CONFERENCE_BACKEND_H
#define CONFERENCE_BACKEND_H

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QImage>
#include "../core/conference_manager.h"
#include "../core/network_client.h"
#include "../core/screen_capturer.h"

class ConferenceBackend : public QObject
{
    Q_OBJECT
    
    // Room info
    Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY userNameChanged)
    Q_PROPERTY(bool isHost READ isHost CONSTANT)
    
    // Connection
    Q_PROPERTY(int participantCount READ participantCount NOTIFY participantCountChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString connectionColor READ connectionColor NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    
    // Media state
    Q_PROPERTY(bool micEnabled READ micEnabled NOTIFY micEnabledChanged)
    Q_PROPERTY(bool camEnabled READ camEnabled NOTIFY camEnabledChanged)
    Q_PROPERTY(bool screenSharing READ screenSharing NOTIFY screenSharingChanged)
    
    // UI state
    Q_PROPERTY(bool isChatVisible READ isChatVisible WRITE setIsChatVisible NOTIFY chatVisibleChanged)
    Q_PROPERTY(bool isParticipantsVisible READ isParticipantsVisible WRITE setIsParticipantsVisible NOTIFY participantsVisibleChanged)
    Q_PROPERTY(bool sidebarVisible READ sidebarVisible WRITE setSidebarVisible NOTIFY sidebarVisibleChanged)
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(bool isFullscreen READ isFullscreen WRITE setIsFullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop WRITE setAlwaysOnTop NOTIFY alwaysOnTopChanged)
    Q_PROPERTY(bool showScreenShareInMain READ showScreenShareInMain NOTIFY showScreenShareInMainChanged)
    
    // Data
    Q_PROPERTY(QVariantList participants READ participants NOTIFY participantsChanged)
    Q_PROPERTY(QVariantList chatMessages READ chatMessages NOTIFY chatMessagesChanged)
    Q_PROPERTY(QString mainParticipantId READ mainParticipantId NOTIFY mainParticipantChanged)
    
public:
    explicit ConferenceBackend(QObject* parent = nullptr);
    ~ConferenceBackend() override;
    
    // Initialize with connection params
    Q_INVOKABLE void initialize(const QString& url, const QString& token,
                                const QString& roomName, const QString& userName, bool isHost);
    
    // Property getters
    QString roomName() const { return roomName_; }
    QString userName() const { return userName_; }
    bool isHost() const { return isHost_; }
    int participantCount() const;
    QString connectionStatus() const { return connectionStatus_; }
    QString connectionColor() const { return connectionColor_; }
    bool isConnected() const;
    bool micEnabled() const;
    bool camEnabled() const;
    bool screenSharing() const;
    bool isChatVisible() const { return isChatVisible_; }
    bool isParticipantsVisible() const { return isParticipantsVisible_; }
    bool sidebarVisible() const { return sidebarVisible_; }
    QString viewMode() const { return viewMode_; }
    bool isFullscreen() const { return isFullscreen_; }
    bool alwaysOnTop() const { return alwaysOnTop_; }
    bool showScreenShareInMain() const { return showScreenShareInMain_; }
    QVariantList participants() const { return participants_; }
    QVariantList chatMessages() const { return chatMessages_; }
    QString mainParticipantId() const { return mainParticipantId_; }
    
    // Property setters
    void setIsChatVisible(bool visible);
    void setIsParticipantsVisible(bool visible);
    void setSidebarVisible(bool visible);
    void setViewMode(const QString& mode);
    void setIsFullscreen(bool fullscreen);
    void setAlwaysOnTop(bool onTop);
    
    // Media controls
    Q_INVOKABLE void toggleMicrophone();
    Q_INVOKABLE void toggleCamera();
    Q_INVOKABLE void toggleScreenShare();
    Q_INVOKABLE void startScreenShare(int screenIndex);
    Q_INVOKABLE void startWindowShare(qulonglong windowId);
    Q_INVOKABLE void stopScreenShare();
    
    // Device switching during conference
    Q_INVOKABLE void switchMicrophone(const QString& deviceId);
    Q_INVOKABLE void switchCamera(const QString& deviceId);
    
    // UI controls
    Q_INVOKABLE void toggleChat();
    Q_INVOKABLE void toggleParticipants();
    Q_INVOKABLE void toggleMainViewSource();  // Switch between camera/screen share in main view (local)
    Q_INVOKABLE void toggleRemoteMainViewSource(const QString& participantId);  // For remote participants
    Q_INVOKABLE bool getRemoteShowScreenInMain(const QString& participantId) const;
    Q_INVOKABLE bool getRemoteScreenSharing(const QString& participantId) const;
    Q_INVOKABLE void leave();
    Q_INVOKABLE void confirmLeave();
    
    // Chat
    Q_INVOKABLE void sendChatMessage(const QString& message);
    
    // Participant management
    Q_INVOKABLE void pinParticipant(const QString& identity);
    Q_INVOKABLE void unpinMain();
    Q_INVOKABLE void kickParticipant(const QString& identity);
    Q_INVOKABLE void muteParticipant(const QString& identity);
    Q_INVOKABLE void hideParticipantVideo(const QString& identity);
    
    // Get participant info
    Q_INVOKABLE QVariantMap getParticipantInfo(const QString& identity) const;
    Q_INVOKABLE bool isParticipantMicEnabled(const QString& identity) const;
    Q_INVOKABLE bool isParticipantCamEnabled(const QString& identity) const;
    
signals:
    void roomNameChanged();
    void userNameChanged();
    void participantCountChanged();
    void connectionStatusChanged();
    void micEnabledChanged();
    void camEnabledChanged();
    void screenSharingChanged();
    void chatVisibleChanged();
    void participantsVisibleChanged();
    void sidebarVisibleChanged();
    void viewModeChanged();
    void fullscreenChanged();
    void alwaysOnTopChanged();
    void showScreenShareInMainChanged();
    void participantsChanged();
    void chatMessagesChanged();
    void mainParticipantChanged();
    
    // Video frame signals for QML video renderers
    void localVideoFrameReady(const QImage& frame);
    void localScreenFrameReady(const QImage& frame);
    void remoteVideoFrameReady(const QString& participantId, const QImage& frame);
    void remoteScreenFrameReady(const QString& participantId, const QImage& frame);
    
    // Navigation
    void leaveRequested();
    void showSettings();
    
    // Participant events for UI updates
    void participantJoined(const QString& identity, const QString& name);
    void participantLeft(const QString& identity);
    void remoteViewStateChanged(const QString& participantId);  // Emitted when toggle camera/screen in main
    void remoteTrackEnded(const QString& participantId, bool isScreenShare);  // Emitted when remote track ends
    
private slots:
    void onConnected();
    void onDisconnected();
    void onConnectionStateChanged(livekit::ConnectionState state);
    void onParticipantJoined(const ParticipantInfo& info);
    void onParticipantLeft(const QString& identity);
    void onChatMessageReceived(const ChatMessage& message);
    void onVideoFrameReceived(const QString& participantIdentity,
                              const QString& trackSid,
                              const QImage& frame,
                              livekit::TrackSource source);
    void onLocalVideoFrameReady(const QImage& frame);
    void onLocalScreenFrameReady(const QImage& frame);
    void onTrackMutedStateChanged(const QString& trackSid, const QString& id,
                                  livekit::TrackKind kind, bool muted);
    void onTrackUnsubscribed(const QString& trackSid, const QString& participantIdentity);
    void onTrackSubscribed(const TrackInfo& track);
    void onTrackUnpublished(const QString& trackSid, const QString& participantIdentity,
                            livekit::TrackKind kind, livekit::TrackSource source);
    
private:
    void setupConnections();
    void updateParticipantsList();
    void addChatMessage(const ChatMessage& msg);
    
    // Core
    ConferenceManager* conferenceManager_;
    QString url_;
    QString token_;
    QString roomName_;
    QString userName_;
    bool isHost_;
    
    // Connection state
    QString connectionStatus_{"Connecting..."};
    QString connectionColor_{"#a0a0b0"};
    
    // UI state
    bool isChatVisible_{false};
    bool isParticipantsVisible_{false};
    bool sidebarVisible_{true};
    QString viewMode_{"speaker"};
    bool isFullscreen_{false};
    bool alwaysOnTop_{false};
    bool showScreenShareInMain_{true};  // When true, screen share shows in main view; when false, camera shows
    
    // Data
    QVariantList participants_;
    QVariantList chatMessages_;
    QString mainParticipantId_;
    bool pinnedMain_{false};
    
    // State tracking
    QMap<QString, bool> micState_;
    QMap<QString, bool> camState_;
    QMap<QString, bool> screenShareState_;  // participantId -> has screen share
    QMap<QString, bool> remoteShowScreenShareInMain_;  // participantId -> show screen in main (vs camera)
    QMap<QString, QString> nameMap_;
    QMap<QString, bool> mutedParticipants_;
    QMap<QString, bool> hiddenVideoParticipants_;
    
    // Track mapping: trackSid -> {participantId, isScreenShare}
    QMap<QString, QPair<QString, bool>> trackInfoMap_;
};

#endif // CONFERENCE_BACKEND_H
