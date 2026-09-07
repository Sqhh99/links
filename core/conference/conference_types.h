#ifndef CORE_CONFERENCE_CONFERENCE_TYPES_H
#define CORE_CONFERENCE_CONFERENCE_TYPES_H

#include <cstdint>
#include <memory>
#include <string>
#include "livekit/livekit.h"

// All strings here are UTF-8. The LiveKit SDK already speaks UTF-8 std::string,
// so this is the natural representation; the Qt boundary in ui/backend converts
// with QString::fromStdString / toStdString, both of which are UTF-8.

struct ParticipantInfo {
    std::string identity;
    std::string sid;
    std::string name;
    bool isMicrophoneEnabled;
    bool isCameraEnabled;
    bool isScreenSharing;
    bool isHost{false};
};

struct ChatMessage {
    std::string sender;
    std::string senderIdentity;
    std::string message;
    std::int64_t timestamp;
    bool isLocal;
};

enum class NetworkQualityLevel {
    Unknown = 0,
    Poor,
    Good,
    Excellent,
    Lost,
};

struct NetworkStatsSnapshot {
    int rttMs{-1};
    int jitterMs{-1};
    double packetLossPercent{-1.0};
    int uplinkKbps{-1};
    int downlinkKbps{-1};
    std::int64_t sampledAtMs{0};

    // Extended fields (populated from RTC stats when available)
    int videoWidth{0};
    int videoHeight{0};
    double videoFps{-1.0};
    std::string audioCodec;
    std::string videoCodec;
    int availableSendBandwidthKbps{-1};
    std::string transportProtocol;
};

struct TrackInfo {
    std::string trackSid;
    std::string participantIdentity;
    livekit::TrackKind kind;
    livekit::TrackSource source;
    bool isLocal;
    std::shared_ptr<livekit::Track> track;
};

#endif // CORE_CONFERENCE_CONFERENCE_TYPES_H
