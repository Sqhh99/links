#ifndef CORE_CONFERENCE_CONFERENCE_TYPES_H
#define CORE_CONFERENCE_CONFERENCE_TYPES_H

#include <QString>
#include <QMetaType>
#include <cstdint>
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
    QString audioCodec;
    QString videoCodec;
    int availableSendBandwidthKbps{-1};
    QString transportProtocol;
};

struct TrackInfo {
    QString trackSid;
    QString participantIdentity;
    livekit::TrackKind kind;
    livekit::TrackSource source;
    bool isLocal;
    std::shared_ptr<livekit::Track> track;
};

Q_DECLARE_METATYPE(NetworkStatsSnapshot)

#endif // CORE_CONFERENCE_CONFERENCE_TYPES_H
