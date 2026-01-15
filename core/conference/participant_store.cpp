#include "participant_store.h"

ParticipantInfo ParticipantStore::addParticipant(const QString& identity,
                                                 const QString& sid,
                                                 const QString& name)
{
    ParticipantInfo info;
    info.identity = identity;
    info.sid = sid;
    info.name = name;
    info.isMicrophoneEnabled = false;
    info.isCameraEnabled = false;
    info.isScreenSharing = false;

    participants_[identity] = info;
    return info;
}

void ParticipantStore::removeParticipant(const QString& identity)
{
    participants_.remove(identity);
}

bool ParticipantStore::contains(const QString& identity) const
{
    return participants_.contains(identity);
}

ParticipantInfo ParticipantStore::participantInfo(const QString& identity) const
{
    return participants_.value(identity);
}

QList<ParticipantInfo> ParticipantStore::participants() const
{
    return participants_.values();
}

int ParticipantStore::size() const
{
    return participants_.size();
}

void ParticipantStore::clear()
{
    participants_.clear();
    trackSources_.clear();
    trackKinds_.clear();
    screenShareActive_.clear();
}

void ParticipantStore::setTrackSource(const QString& trackSid, livekit::TrackSource source)
{
    trackSources_[trackSid] = source;
}

void ParticipantStore::setTrackKind(const QString& trackSid, livekit::TrackKind kind)
{
    trackKinds_[trackSid] = kind;
}

void ParticipantStore::removeTrack(const QString& trackSid)
{
    trackSources_.remove(trackSid);
    trackKinds_.remove(trackSid);
}

bool ParticipantStore::hasTrackSource(const QString& trackSid) const
{
    return trackSources_.contains(trackSid);
}

livekit::TrackSource ParticipantStore::trackSource(const QString& trackSid) const
{
    return trackSources_.value(trackSid, livekit::TrackSource::SOURCE_UNKNOWN);
}

livekit::TrackKind ParticipantStore::trackKind(const QString& trackSid) const
{
    return trackKinds_.value(trackSid, livekit::TrackKind::KIND_AUDIO);
}

void ParticipantStore::setScreenShareActive(const QString& identity, bool active)
{
    screenShareActive_[identity] = active;
}

bool ParticipantStore::screenShareActive(const QString& identity) const
{
    return screenShareActive_.value(identity, false);
}

ParticipantInfo ParticipantStore::refreshParticipantInfo(const QString& identity)
{
    if (!participants_.contains(identity)) {
        return ParticipantInfo{};
    }

    ParticipantInfo updated = participants_[identity];
    updated.isMicrophoneEnabled = false;
    updated.isCameraEnabled = false;
    updated.isScreenSharing = false;

    // Update based on tracked sources/kinds (mirrors existing behavior).
    for (auto it = trackSources_.begin(); it != trackSources_.end(); ++it) {
        const QString trackSid = it.key();
        const livekit::TrackSource source = it.value();

        if (trackKinds_.contains(trackSid)) {
            const livekit::TrackKind kind = trackKinds_[trackSid];
            if (kind == livekit::TrackKind::KIND_AUDIO
                && source == livekit::TrackSource::SOURCE_MICROPHONE) {
                updated.isMicrophoneEnabled = true;
            }
            if (kind == livekit::TrackKind::KIND_VIDEO
                && source == livekit::TrackSource::SOURCE_CAMERA) {
                updated.isCameraEnabled = true;
            }
            if (kind == livekit::TrackKind::KIND_VIDEO
                && (source == livekit::TrackSource::SOURCE_SCREENSHARE
                    || source == livekit::TrackSource::SOURCE_SCREENSHARE_AUDIO)) {
                updated.isScreenSharing = true;
            }
        }
    }

    participants_[identity] = updated;
    return updated;
}
