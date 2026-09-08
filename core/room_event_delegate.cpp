#include "room_event_delegate.h"

#include <utility>

#include "base/log.h"
#include "base/strings.h"
#include "conference/participant_metadata_parser.h"
#include "livekit/local_track_publication.h"
#include "livekit/participant.h"
#include "livekit/remote_participant.h"
#include "livekit/remote_track_publication.h"
#include "livekit/track.h"

namespace core = links::core;

RoomEventDelegate::RoomEventDelegate(core::TaskRunner& taskRunner)
    : taskRunner_(taskRunner)
{
}

void RoomEventDelegate::onParticipantConnected(livekit::Room& room,
                                                const livekit::ParticipantConnectedEvent& event)
{
    (void)room;
    if (!event.participant) return;

    std::string identity = event.participant->identity();
    std::string sid = event.participant->sid();
    std::string name = event.participant->name();
    const bool isHost = links::conference::parseIsHostFromParticipantMetadata(event.participant->metadata());

    core::logInfo(core::str::cat("RoomEventDelegate: Participant connected: ", name));

    core::postGuarded(taskRunner_, lifetime_,
        [this, identity = std::move(identity), sid = std::move(sid),
         name = std::move(name), isHost]() mutable {
            participantConnected.notify(identity, sid, name, isHost);
        });
}

void RoomEventDelegate::onParticipantDisconnected(livekit::Room& room,
                                                   const livekit::ParticipantDisconnectedEvent& event)
{
    (void)room;
    if (!event.participant) return;

    std::string identity = event.participant->identity();
    const int reason = static_cast<int>(event.reason);

    core::logInfo(core::str::cat("RoomEventDelegate: Participant disconnected: ", identity));

    core::postGuarded(taskRunner_, lifetime_,
        [this, identity = std::move(identity), reason]() mutable {
            participantDisconnected.notify(identity, reason);
        });
}

void RoomEventDelegate::onTrackSubscribed(livekit::Room& room,
                                          const livekit::TrackSubscribedEvent& event)
{
    (void)room;
    if (!event.track || !event.participant) return;

    std::string trackSid = event.track->sid();
    std::string participantIdentity = event.participant->identity();
    const int kind = static_cast<int>(event.track->kind());
    const int source = event.track->source().has_value()
        ? static_cast<int>(event.track->source().value()) : 0;
    const bool muted = event.track->muted();

    // The two shared_ptrs are captured by value, keeping the SDK objects alive
    // across the thread hop exactly as the previous queued signal did.
    auto track = event.track;
    auto publication = event.publication;

    core::logInfo(core::str::cat("RoomEventDelegate: Track subscribed: ", trackSid,
                                 " from ", participantIdentity));

    core::postGuarded(taskRunner_, lifetime_,
        [this, trackSid = std::move(trackSid),
         participantIdentity = std::move(participantIdentity),
         kind, source, muted, track = std::move(track),
         publication = std::move(publication)]() mutable {
            trackSubscribed.notify(trackSid, participantIdentity, kind, source, muted,
                                   track, publication);
        });
}

void RoomEventDelegate::onTrackUnsubscribed(livekit::Room& room,
                                            const livekit::TrackUnsubscribedEvent& event)
{
    (void)room;
    if (!event.track || !event.participant) return;

    std::string trackSid = event.track->sid();
    std::string participantIdentity = event.participant->identity();

    core::logInfo(core::str::cat("RoomEventDelegate: Track unsubscribed: ", trackSid,
                                 " from ", participantIdentity));

    core::postGuarded(taskRunner_, lifetime_,
        [this, trackSid = std::move(trackSid),
         participantIdentity = std::move(participantIdentity)]() mutable {
            trackUnsubscribed.notify(trackSid, participantIdentity);
        });
}

void RoomEventDelegate::onTrackMuted(livekit::Room& room,
                                      const livekit::TrackMutedEvent& event)
{
    (void)room;
    if (!event.publication || !event.participant) return;

    std::string trackSid = event.publication->sid();
    std::string participantIdentity = event.participant->identity();
    const int kind = static_cast<int>(event.publication->kind());

    core::logInfo(core::str::cat("RoomEventDelegate: Track muted: ", trackSid,
                                 " from ", participantIdentity));

    core::postGuarded(taskRunner_, lifetime_,
        [this, trackSid = std::move(trackSid),
         participantIdentity = std::move(participantIdentity), kind]() mutable {
            trackMuted.notify(trackSid, participantIdentity, kind);
        });
}

void RoomEventDelegate::onTrackUnmuted(livekit::Room& room,
                                        const livekit::TrackUnmutedEvent& event)
{
    (void)room;
    if (!event.publication || !event.participant) return;

    std::string trackSid = event.publication->sid();
    std::string participantIdentity = event.participant->identity();
    const int kind = static_cast<int>(event.publication->kind());

    core::logInfo(core::str::cat("RoomEventDelegate: Track unmuted: ", trackSid,
                                 " from ", participantIdentity));

    core::postGuarded(taskRunner_, lifetime_,
        [this, trackSid = std::move(trackSid),
         participantIdentity = std::move(participantIdentity), kind]() mutable {
            trackUnmuted.notify(trackSid, participantIdentity, kind);
        });
}

void RoomEventDelegate::onTrackUnpublished(livekit::Room& room,
                                            const livekit::TrackUnpublishedEvent& event)
{
    (void)room;
    if (!event.publication || !event.participant) return;

    std::string trackSid = event.publication->sid();
    std::string participantIdentity = event.participant->identity();
    const int kind = static_cast<int>(event.publication->kind());
    const int source = static_cast<int>(event.publication->source());

    core::logInfo(core::str::cat("RoomEventDelegate: Track unpublished: ", trackSid,
                                 " from ", participantIdentity));

    core::postGuarded(taskRunner_, lifetime_,
        [this, trackSid = std::move(trackSid),
         participantIdentity = std::move(participantIdentity), kind, source]() mutable {
            trackUnpublished.notify(trackSid, participantIdentity, kind, source);
        });
}

void RoomEventDelegate::onConnectionQualityChanged(
    livekit::Room& room,
    const livekit::ConnectionQualityChangedEvent& event)
{
    (void)room;
    if (!event.participant) {
        return;
    }

    std::string participantIdentity = event.participant->identity();
    const int quality = static_cast<int>(event.quality);

    core::logInfo(core::str::cat("RoomEventDelegate: Connection quality changed for ",
                                 participantIdentity, ": ", quality));

    core::postGuarded(taskRunner_, lifetime_,
        [this, participantIdentity = std::move(participantIdentity), quality]() mutable {
            connectionQualityChanged.notify(participantIdentity, quality);
        });
}

void RoomEventDelegate::onConnectionStateChanged(livekit::Room& room,
                                                  const livekit::ConnectionStateChangedEvent& event)
{
    (void)room;
    const int state = static_cast<int>(event.state);

    core::logInfo(core::str::cat("RoomEventDelegate: Connection state changed: ", state));

    core::postGuarded(taskRunner_, lifetime_, [this, state]() {
        connectionStateChanged.notify(state);
    });
}

void RoomEventDelegate::onDisconnected(livekit::Room& room,
                                       const livekit::DisconnectedEvent& event)
{
    (void)room;
    const int reason = static_cast<int>(event.reason);

    core::logInfo(core::str::cat("RoomEventDelegate: Room disconnected, reason: ", reason));

    core::postGuarded(taskRunner_, lifetime_, [this, reason]() {
        roomDisconnected.notify(reason);
    });
}

void RoomEventDelegate::onUserPacketReceived(livekit::Room& room,
                                              const livekit::UserDataPacketEvent& event)
{
    (void)room;

    std::vector<std::uint8_t> data(event.data.begin(), event.data.end());
    std::string participantIdentity;
    if (event.participant) {
        participantIdentity = event.participant->identity();
    }
    std::string topic = event.topic;

    core::logDebug(core::str::cat("RoomEventDelegate: Data received from ", participantIdentity,
                                  ", topic: ", topic));

    core::postGuarded(taskRunner_, lifetime_,
        [this, data = std::move(data),
         participantIdentity = std::move(participantIdentity),
         topic = std::move(topic)]() mutable {
            dataReceived.notify(data, participantIdentity, topic);
        });
}

void RoomEventDelegate::onLocalTrackPublished(livekit::Room& room,
                                               const livekit::LocalTrackPublishedEvent& event)
{
    (void)room;
    if (!event.publication) {
        return;
    }

    std::string publicationSid = event.publication->sid();
    const int kind = static_cast<int>(event.publication->kind());
    const int source = static_cast<int>(event.publication->source());

    core::logInfo(core::str::cat("RoomEventDelegate: Local track published: sid=", publicationSid,
                                 " source=", source));

    core::postGuarded(taskRunner_, lifetime_,
        [this, publicationSid = std::move(publicationSid), kind, source]() mutable {
            localTrackPublished.notify(publicationSid, kind, source);
        });
}
