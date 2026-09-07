#include "participant_store.h"

namespace {

template <typename Map, typename Key, typename Value>
Value valueOr(const Map& map, const Key& key, Value fallback)
{
    const auto it = map.find(key);
    return it == map.end() ? fallback : it->second;
}

}  // namespace

ParticipantInfo ParticipantStore::addParticipant(const std::string& identity,
                                                 const std::string& sid,
                                                 const std::string& name,
                                                 bool isHost)
{
    ParticipantInfo info;
    const auto existing = participants_.find(identity);
    if (existing != participants_.end()) {
        info = existing->second;
        info.sid = sid;
        info.name = name;
        info.isHost = isHost;
    } else {
        info.identity = identity;
        info.sid = sid;
        info.name = name;
        info.isMicrophoneEnabled = false;
        info.isCameraEnabled = false;
        info.isScreenSharing = false;
        info.isHost = isHost;
    }

    participants_[identity] = info;
    return info;
}

void ParticipantStore::removeParticipant(const std::string& identity)
{
    participants_.erase(identity);
}

bool ParticipantStore::contains(const std::string& identity) const
{
    return participants_.find(identity) != participants_.end();
}

ParticipantInfo ParticipantStore::participantInfo(const std::string& identity) const
{
    const auto it = participants_.find(identity);
    return it == participants_.end() ? ParticipantInfo{} : it->second;
}

std::vector<ParticipantInfo> ParticipantStore::participants() const
{
    std::vector<ParticipantInfo> out;
    out.reserve(participants_.size());
    for (const auto& entry : participants_) {
        out.push_back(entry.second);
    }
    return out;
}

int ParticipantStore::size() const
{
    return static_cast<int>(participants_.size());
}

void ParticipantStore::clear()
{
    participants_.clear();
    trackSources_.clear();
    trackKinds_.clear();
    screenShareActive_.clear();
}

void ParticipantStore::setTrackSource(const std::string& trackSid, livekit::TrackSource source)
{
    trackSources_[trackSid] = source;
}

void ParticipantStore::setTrackKind(const std::string& trackSid, livekit::TrackKind kind)
{
    trackKinds_[trackSid] = kind;
}

void ParticipantStore::removeTrack(const std::string& trackSid)
{
    trackSources_.erase(trackSid);
    trackKinds_.erase(trackSid);
}

bool ParticipantStore::hasTrackSource(const std::string& trackSid) const
{
    return trackSources_.find(trackSid) != trackSources_.end();
}

livekit::TrackSource ParticipantStore::trackSource(const std::string& trackSid) const
{
    return valueOr(trackSources_, trackSid, livekit::TrackSource::SOURCE_UNKNOWN);
}

livekit::TrackKind ParticipantStore::trackKind(const std::string& trackSid) const
{
    return valueOr(trackKinds_, trackSid, livekit::TrackKind::KIND_AUDIO);
}

void ParticipantStore::setScreenShareActive(const std::string& identity, bool active)
{
    screenShareActive_[identity] = active;
}

bool ParticipantStore::screenShareActive(const std::string& identity) const
{
    return valueOr(screenShareActive_, identity, false);
}

ParticipantInfo ParticipantStore::refreshParticipantInfo(const std::string& identity)
{
    const auto participant = participants_.find(identity);
    if (participant == participants_.end()) {
        return ParticipantInfo{};
    }

    ParticipantInfo updated = participant->second;
    updated.isMicrophoneEnabled = false;
    updated.isCameraEnabled = false;
    updated.isScreenSharing = false;

    // Update based on tracked sources/kinds (mirrors existing behavior).
    for (const auto& entry : trackSources_) {
        const std::string& trackSid = entry.first;
        const livekit::TrackSource source = entry.second;

        const auto kindIt = trackKinds_.find(trackSid);
        if (kindIt != trackKinds_.end()) {
            const livekit::TrackKind kind = kindIt->second;
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

    participant->second = updated;
    return updated;
}
