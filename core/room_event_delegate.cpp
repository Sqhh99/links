#include "room_event_delegate.h"
#include "../utils/logger.h"
#include "livekit/remote_participant.h"
#include "livekit/remote_track_publication.h"
#include "livekit/track.h"
#include <QMetaObject>

RoomEventDelegate::RoomEventDelegate(QObject* parent)
    : QObject(parent)
{
}

void RoomEventDelegate::onParticipantConnected(livekit::Room& room, 
                                                const livekit::ParticipantConnectedEvent& event)
{
    Q_UNUSED(room);
    if (!event.participant) return;
    
    QString identity = QString::fromStdString(event.participant->identity());
    QString sid = QString::fromStdString(event.participant->sid());
    QString name = QString::fromStdString(event.participant->name());
    
    Logger::instance().info(QString("RoomEventDelegate: Participant connected: %1").arg(name));
    
    emit participantConnectedQueued(identity, sid, name);
}

void RoomEventDelegate::onParticipantDisconnected(livekit::Room& room, 
                                                   const livekit::ParticipantDisconnectedEvent& event)
{
    Q_UNUSED(room);
    if (!event.participant) return;
    
    QString identity = QString::fromStdString(event.participant->identity());
    int reason = static_cast<int>(event.reason);
    
    Logger::instance().info(QString("RoomEventDelegate: Participant disconnected: %1").arg(identity));
    
    emit participantDisconnectedQueued(identity, reason);
}

void RoomEventDelegate::onTrackSubscribed(livekit::Room& room, 
                                          const livekit::TrackSubscribedEvent& event)
{
    Q_UNUSED(room);
    if (!event.track || !event.participant) return;
    
    QString trackSid = QString::fromStdString(event.track->sid());
    QString participantIdentity = QString::fromStdString(event.participant->identity());
    int kind = static_cast<int>(event.track->kind());
    int source = event.track->source().has_value() ? static_cast<int>(event.track->source().value()) : 0;
    bool muted = event.track->muted();
    
    Logger::instance().info(QString("RoomEventDelegate: Track subscribed: %1 from %2")
        .arg(trackSid).arg(participantIdentity));
    
    emit trackSubscribedQueued(trackSid, participantIdentity, kind, source, muted,
                               event.track, event.publication);
}

void RoomEventDelegate::onTrackUnsubscribed(livekit::Room& room, 
                                            const livekit::TrackUnsubscribedEvent& event)
{
    Q_UNUSED(room);
    if (!event.track || !event.participant) return;
    
    QString trackSid = QString::fromStdString(event.track->sid());
    QString participantIdentity = QString::fromStdString(event.participant->identity());
    
    Logger::instance().info(QString("RoomEventDelegate: Track unsubscribed: %1 from %2")
        .arg(trackSid).arg(participantIdentity));
    
    emit trackUnsubscribedQueued(trackSid, participantIdentity);
}

void RoomEventDelegate::onTrackMuted(livekit::Room& room, 
                                      const livekit::TrackMutedEvent& event)
{
    Q_UNUSED(room);
    if (!event.publication || !event.participant) return;
    
    QString trackSid = QString::fromStdString(event.publication->sid());
    QString participantIdentity = QString::fromStdString(event.participant->identity());
    int kind = static_cast<int>(event.publication->kind());
    
    Logger::instance().info(QString("RoomEventDelegate: Track muted: %1 from %2")
        .arg(trackSid).arg(participantIdentity));
    
    emit trackMutedQueued(trackSid, participantIdentity, kind);
}

void RoomEventDelegate::onTrackUnmuted(livekit::Room& room, 
                                        const livekit::TrackUnmutedEvent& event)
{
    Q_UNUSED(room);
    if (!event.publication || !event.participant) return;
    
    QString trackSid = QString::fromStdString(event.publication->sid());
    QString participantIdentity = QString::fromStdString(event.participant->identity());
    int kind = static_cast<int>(event.publication->kind());
    
    Logger::instance().info(QString("RoomEventDelegate: Track unmuted: %1 from %2")
        .arg(trackSid).arg(participantIdentity));
    
    emit trackUnmutedQueued(trackSid, participantIdentity, kind);
}

void RoomEventDelegate::onTrackUnpublished(livekit::Room& room,
                                            const livekit::TrackUnpublishedEvent& event)
{
    Q_UNUSED(room);
    if (!event.publication || !event.participant) return;
    
    QString trackSid = QString::fromStdString(event.publication->sid());
    QString participantIdentity = QString::fromStdString(event.participant->identity());
    int kind = static_cast<int>(event.publication->kind());
    int source = static_cast<int>(event.publication->source());
    
    Logger::instance().info(QString("RoomEventDelegate: Track unpublished: %1 from %2")
        .arg(trackSid).arg(participantIdentity));
    
    emit trackUnpublishedQueued(trackSid, participantIdentity, kind, source);
}
void RoomEventDelegate::onConnectionStateChanged(livekit::Room& room, 
                                                  const livekit::ConnectionStateChangedEvent& event)
{
    Q_UNUSED(room);
    int state = static_cast<int>(event.state);
    
    Logger::instance().info(QString("RoomEventDelegate: Connection state changed: %1").arg(state));
    
    emit connectionStateChangedQueued(state);
}

void RoomEventDelegate::onUserPacketReceived(livekit::Room& room, 
                                              const livekit::UserDataPacketEvent& event)
{
    Q_UNUSED(room);
    
    QByteArray data(reinterpret_cast<const char*>(event.data.data()), 
                    static_cast<int>(event.data.size()));
    QString participantIdentity;
    if (event.participant) {
        participantIdentity = QString::fromStdString(event.participant->identity());
    }
    QString topic = QString::fromStdString(event.topic);
    
    Logger::instance().debug(QString("RoomEventDelegate: Data received from %1, topic: %2")
        .arg(participantIdentity).arg(topic));
    
    emit dataReceivedQueued(data, participantIdentity, topic);
}

void RoomEventDelegate::onLocalTrackPublished(livekit::Room& room,
                                               const livekit::LocalTrackPublishedEvent& event)
{
    Q_UNUSED(room);
    Q_UNUSED(event);
    // Log for debugging, actual handling is done synchronously in publish calls
    Logger::instance().info("RoomEventDelegate: Local track published");
}
