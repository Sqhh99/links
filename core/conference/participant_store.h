#ifndef CORE_CONFERENCE_PARTICIPANT_STORE_H
#define CORE_CONFERENCE_PARTICIPANT_STORE_H

#include <map>
#include <string>
#include <vector>
#include "conference_types.h"

class ParticipantStore {
public:
    ParticipantInfo addParticipant(const std::string& identity,
                                   const std::string& sid,
                                   const std::string& name,
                                   bool isHost = false);
    void removeParticipant(const std::string& identity);
    bool contains(const std::string& identity) const;
    ParticipantInfo participantInfo(const std::string& identity) const;
    std::vector<ParticipantInfo> participants() const;
    int size() const;

    void clear();

    void setTrackSource(const std::string& trackSid, livekit::TrackSource source);
    void setTrackKind(const std::string& trackSid, livekit::TrackKind kind);
    void removeTrack(const std::string& trackSid);
    bool hasTrackSource(const std::string& trackSid) const;
    livekit::TrackSource trackSource(const std::string& trackSid) const;
    livekit::TrackKind trackKind(const std::string& trackSid) const;

    void setScreenShareActive(const std::string& identity, bool active);
    bool screenShareActive(const std::string& identity) const;

    ParticipantInfo refreshParticipantInfo(const std::string& identity);

private:
    // std::map like QMap keeps participants() in a deterministic key order.
    // The order differs from QMap's for identities above U+FFFF (UTF-8 byte
    // order vs UTF-16 code-unit order); identities are server-issued and ASCII.
    std::map<std::string, ParticipantInfo> participants_;
    std::map<std::string, livekit::TrackSource> trackSources_;
    std::map<std::string, livekit::TrackKind> trackKinds_;
    std::map<std::string, bool> screenShareActive_;
};

#endif // CORE_CONFERENCE_PARTICIPANT_STORE_H
