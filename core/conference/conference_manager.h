#ifndef CORE_CONFERENCE_CONFERENCE_MANAGER_H
#define CORE_CONFERENCE_CONFERENCE_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QImage>
#include <QList>
#include <memory>
#include "conference_types.h"
#include "room_controller.h"
#include "participant_store.h"
#include "media_pipeline.h"
#include "device_controller.h"
#include "../screen_capturer.h"
#include "livekit/livekit.h"

class RoomEventDelegate;

class ConferenceManager : public QObject {
    Q_OBJECT
    
public:
    explicit ConferenceManager(QObject* parent = nullptr);
    ~ConferenceManager() override;
    
    // Connection
    void connect(const QString& url, const QString& token);
    void disconnect();
    bool isConnected() const { return connected_; }
    
    // Media controls
    void toggleMicrophone();
    void toggleCamera();
    void toggleScreenShare();
    void setScreenShareMode(ScreenCapturer::Mode mode, QScreen* screen, WId windowId);
    
    // Device switching (while conference is active)
    void switchCamera(const QString& deviceId);
    void switchMicrophone(const QString& deviceId);
    
    bool isMicrophoneEnabled() const;
    bool isCameraEnabled() const;
    bool isScreenSharing() const;
    
    // Chat
    void sendChatMessage(const QString& message);
    
    // Participants
    QList<ParticipantInfo> getParticipants() const;
    int getParticipantCount() const;
    
    // Room info
    QString getRoomName() const { return roomName_; }
    QString getLocalParticipantName() const { return participantName_; }
    
signals:
    // Connection events
    void connected();
    void disconnected();
    void connectionStateChanged(livekit::ConnectionState state);
    void connectionError(const QString& error);
    
    // Participant events
    void participantJoined(const ParticipantInfo& info);
    void participantLeft(const QString& identity);
    void participantUpdated(const ParticipantInfo& info);
    
    // Track events
    void trackSubscribed(const TrackInfo& track);
    void trackUnsubscribed(const QString& trackSid, const QString& participantIdentity);
    void trackUnpublished(const QString& trackSid, const QString& participantIdentity,
                          livekit::TrackKind kind, livekit::TrackSource source);
    void trackMutedStateChanged(const QString& trackSid,
                               const QString& participantIdentity,
                               livekit::TrackKind kind,
                               bool muted);
    
    // Media events
    void localMicrophoneChanged(bool enabled);
    void localCameraChanged(bool enabled);
    void localScreenShareChanged(bool enabled);
    void localScreenFrameReady(const QImage& frame);
    
    // Chat events
    void chatMessageReceived(const ChatMessage& message);
    void localVideoFrameReady(const QImage& frame);
    void videoFrameReceived(const QString& participantIdentity,
                            const QString& trackSid,
                            const QImage& frame,
                            livekit::TrackSource source);
    void audioActivity(const QString& participantIdentity, bool hasAudio);
    
private:
    // Queued slots for RoomEventDelegate signals (thread-safe event handling)
    void onParticipantConnectedQueued(QString identity, QString sid, QString name);
    void onParticipantDisconnectedQueued(QString identity, int reason);
    void onTrackSubscribedQueued(QString trackSid, QString participantIdentity,
                                 int kind, int source, bool muted,
                                 std::shared_ptr<livekit::Track> track,
                                 std::shared_ptr<livekit::RemoteTrackPublication> publication);
    void onTrackUnsubscribedQueued(QString trackSid, QString participantIdentity);
    void onTrackMutedQueued(QString trackSid, QString participantIdentity, int kind);
    void onTrackUnmutedQueued(QString trackSid, QString participantIdentity, int kind);
    void onTrackUnpublishedQueued(QString trackSid, QString participantIdentity, int kind, int source);
    void onConnectionStateChangedQueued(int state);
    void onDataReceivedQueued(QByteArray data, QString participantIdentity, QString topic);
    
    void updateParticipantInfo(const QString& identity);

    std::unique_ptr<RoomController> roomController_;
    std::unique_ptr<RoomEventDelegate> roomDelegate_;
    std::unique_ptr<ParticipantStore> participantStore_;
    std::unique_ptr<MediaPipeline> mediaPipeline_;
    std::unique_ptr<DeviceController> deviceController_;

    QString roomName_;
    QString participantName_;
    bool connected_{false};
};

#endif // CORE_CONFERENCE_CONFERENCE_MANAGER_H
