#ifndef ROOM_EVENT_DELEGATE_H
#define ROOM_EVENT_DELEGATE_H

#include <QObject>
#include "livekit/room_delegate.h"
#include "livekit/room_event_types.h"

/**
 * Bridge class that implements livekit::RoomDelegate and forwards
 * events to ConferenceManager via Qt's queued connections.
 */
class RoomEventDelegate : public QObject, public livekit::RoomDelegate {
    Q_OBJECT
public:
    explicit RoomEventDelegate(QObject* parent = nullptr);
    ~RoomEventDelegate() override = default;

    // RoomDelegate overrides - these are called from LiveKit SDK threads
    void onParticipantConnected(livekit::Room& room, 
                                const livekit::ParticipantConnectedEvent& event) override;
    void onParticipantDisconnected(livekit::Room& room, 
                                   const livekit::ParticipantDisconnectedEvent& event) override;
    void onTrackSubscribed(livekit::Room& room, 
                          const livekit::TrackSubscribedEvent& event) override;
    void onTrackUnsubscribed(livekit::Room& room, 
                            const livekit::TrackUnsubscribedEvent& event) override;
    void onTrackMuted(livekit::Room& room, 
                      const livekit::TrackMutedEvent& event) override;
    void onTrackUnmuted(livekit::Room& room, 
                        const livekit::TrackUnmutedEvent& event) override;
    void onTrackUnpublished(livekit::Room& room,
                            const livekit::TrackUnpublishedEvent& event) override;
    void onConnectionStateChanged(livekit::Room& room, 
                                  const livekit::ConnectionStateChangedEvent& event) override;
    void onUserPacketReceived(livekit::Room& room, 
                              const livekit::UserDataPacketEvent& event) override;
    void onLocalTrackPublished(livekit::Room& room,
                               const livekit::LocalTrackPublishedEvent& event) override;

signals:
    // Internal signals to queue events to main thread
    void participantConnectedQueued(QString identity, QString sid, QString name);
    void participantDisconnectedQueued(QString identity, int reason);
    void trackSubscribedQueued(QString trackSid, QString participantIdentity, 
                               int kind, int source, bool muted,
                               std::shared_ptr<livekit::Track> track,
                               std::shared_ptr<livekit::RemoteTrackPublication> publication);
    void trackUnsubscribedQueued(QString trackSid, QString participantIdentity);
    void trackMutedQueued(QString trackSid, QString participantIdentity, int kind);
    void trackUnmutedQueued(QString trackSid, QString participantIdentity, int kind);
    void trackUnpublishedQueued(QString trackSid, QString participantIdentity, int kind, int source);
    void connectionStateChangedQueued(int state);
    void dataReceivedQueued(QByteArray data, QString participantIdentity, QString topic);

};

#endif // ROOM_EVENT_DELEGATE_H
