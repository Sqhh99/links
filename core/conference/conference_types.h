#ifndef CORE_CONFERENCE_CONFERENCE_TYPES_H
#define CORE_CONFERENCE_CONFERENCE_TYPES_H

#include <QString>
#include <memory>
#include "livekit/livekit.h"

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

#endif // CORE_CONFERENCE_CONFERENCE_TYPES_H
