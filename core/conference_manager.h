#ifndef CONFERENCE_MANAGER_H
#define CONFERENCE_MANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QImage>
#include <QAudioFormat>
#include <QAudioSink>
#include <QSharedPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include <thread>
#include <atomic>
#include <map>
#include "livekit/livekit.h"
#include "camera_capturer.h"
#include "microphone_capturer.h"
#include "screen_capturer.h"

// Forward declarations
class RoomEventDelegate;

struct ParticipantInfo {
    QString identity;
    QString sid;
    QString name;
    bool isMicrophoneEnabled;
    bool isCameraEnabled;
    bool isScreenSharing;
    bool isHost{false};
};


struct ChatMessage {
    QString sender;
    QString senderIdentity;
    QString message;
    qint64 timestamp;
    bool isLocal;
};

struct TrackInfo {
    QString trackSid;
    QString participantIdentity;
    livekit::TrackKind kind;
    livekit::TrackSource source;
    bool isLocal;
    std::shared_ptr<livekit::Track> track;
};

class ConferenceManager : public QObject
{
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
    
    bool isMicrophoneEnabled() const { return microphoneEnabled_; }
    bool isCameraEnabled() const { return cameraEnabled_; }
    bool isScreenSharing() const { return screenShareEnabled_; }
    
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
    
    // Helpers
    void updateParticipantInfo(const QString& identity);
    void handleVideoFrame(const livekit::VideoFrameEvent& event, const QString& trackSid, const QString& participantIdentity);
    void handleAudioFrame(const livekit::AudioFrameEvent& event, const QString& trackSid, const QString& participantIdentity);
    void connectScreenSignals();
    void startVideoStreamReader(const QString& trackSid, const QString& participantIdentity,
                                std::shared_ptr<livekit::VideoStream> stream);
    void startAudioStreamReader(const QString& trackSid, const QString& participantIdentity,
                                std::shared_ptr<livekit::AudioStream> stream);
    void stopStreamReaders(const QString& trackSid);
    
    // LiveKit objects
    std::unique_ptr<livekit::Room> room_;
    std::unique_ptr<RoomEventDelegate> roomDelegate_;
    
    // Media capturers
    CameraCapturer* cameraCapturer_;
    MicrophoneCapturer* microphoneCapturer_;
    ScreenCapturer* screenCapturer_;
    std::shared_ptr<livekit::Track> localVideoTrack_;
    std::shared_ptr<livekit::Track> localAudioTrack_;
    std::shared_ptr<livekit::Track> localScreenTrack_;
    std::string screenTrackSid_;  // Actual SID from publication, used for unpublishing
    std::string cameraTrackSid_;  // Actual SID from publication, used for unpublishing
    
    // State
    QString roomName_;
    QString participantName_;
    bool connected_;
    bool cameraEnabled_;
    bool microphoneEnabled_;
    bool screenShareEnabled_;
    
    // Event listener
    int eventListenerId_;

    // Participants cache
    QMap<QString, ParticipantInfo> participants_;

    struct AudioPlayback {
        QSharedPointer<QAudioSink> sink;
        QIODevice* device{nullptr};
        QAudioFormat format;
    };

    QMap<QString, std::shared_ptr<livekit::VideoStream>> videoStreams_; // trackSid -> stream
    QMap<QString, std::shared_ptr<livekit::AudioStream>> audioStreams_; // trackSid -> stream
    std::map<QString, std::unique_ptr<std::thread>> videoStreamThreads_; // trackSid -> reader thread
    std::map<QString, std::unique_ptr<std::thread>> audioStreamThreads_; // trackSid -> reader thread
    QMap<QString, std::atomic<bool>*> streamStopFlags_; // trackSid -> stop flag
    QMap<QString, AudioPlayback> audioPlayers_; // trackSid -> player
    QMap<QString, livekit::TrackSource> trackSources_; // trackSid -> source
    QMap<QString, livekit::TrackKind> trackKinds_;     // trackSid -> kind
    QMap<QString, bool> screenShareActive_; // participant identity -> has screen share

    // Screen share debounce
    QElapsedTimer screenShareDebounceTimer_;
    bool screenShareTogglePending_{false};
    static constexpr int kScreenShareDebounceMs = 500;

    bool isScreenShareTrack(const TrackInfo& info, const std::shared_ptr<livekit::RemoteTrack>& track) const;
};

#endif // CONFERENCE_MANAGER_H
