#include "network_stats_aggregator.h"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <variant>
#include <QString>

namespace {

int toRoundedInt(double value)
{
    return static_cast<int>(std::lround(value));
}

void accumulateRttMs(double seconds, double& rttSumMs, int& rttSamples)
{
    if (std::isfinite(seconds) && seconds > 0.0) {
        rttSumMs += seconds * 1000.0;
        ++rttSamples;
    }
}

void accumulateJitterMs(double seconds, double& jitterSumMs, int& jitterSamples)
{
    if (std::isfinite(seconds) && seconds >= 0.0) {
        jitterSumMs += seconds * 1000.0;
        ++jitterSamples;
    }
}

} // namespace

NetworkStatsAggregationResult aggregateNetworkStats(
    const std::vector<livekit::RtcStats>& stats,
    const NetworkByteCounters& previousCounters,
    std::int64_t nowMs)
{
    NetworkStatsAggregationResult result;
    result.snapshot.sampledAtMs = nowMs;

    double rttSumMs = 0.0;
    int rttSamples = 0;
    double jitterSumMs = 0.0;
    int jitterSamples = 0;

    std::uint64_t bytesSent = 0;
    std::uint64_t bytesReceived = 0;
    std::uint64_t packetsReceived = 0;
    std::uint64_t packetsLost = 0;

    // Extended stats accumulators
    int bestVideoWidth = 0;
    int bestVideoHeight = 0;
    double bestVideoFps = -1.0;
    QString audioCodec;
    QString videoCodec;
    double availableOutgoingBitrate = -1.0;
    QString transportProtocol;

    for (const livekit::RtcStats& stat : stats) {
        std::visit(
            [&](const auto& typedStat) {
                using T = std::decay_t<decltype(typedStat)>;
                if constexpr (std::is_same_v<T, livekit::RtcInboundRtpStats>) {
                    bytesReceived += typedStat.inbound.bytes_received;
                    packetsReceived += typedStat.received.packets_received;
                    packetsLost += static_cast<std::uint64_t>(
                        std::max<std::int64_t>(0, typedStat.received.packets_lost));
                    accumulateJitterMs(typedStat.received.jitter, jitterSumMs, jitterSamples);

                    // Video resolution & FPS (pick the largest inbound video)
                    if (typedStat.stream.kind == "video"
                        && typedStat.inbound.frame_width > 0
                        && typedStat.inbound.frame_height > 0) {
                        int pixels = static_cast<int>(typedStat.inbound.frame_width)
                                     * static_cast<int>(typedStat.inbound.frame_height);
                        if (pixels > bestVideoWidth * bestVideoHeight) {
                            bestVideoWidth = static_cast<int>(typedStat.inbound.frame_width);
                            bestVideoHeight = static_cast<int>(typedStat.inbound.frame_height);
                        }
                        if (std::isfinite(typedStat.inbound.frames_per_second)
                            && typedStat.inbound.frames_per_second > bestVideoFps) {
                            bestVideoFps = typedStat.inbound.frames_per_second;
                        }
                    }
                } else if constexpr (std::is_same_v<T, livekit::RtcOutboundRtpStats>) {
                    bytesSent += typedStat.sent.bytes_sent;

                    // Outbound video resolution & FPS (prefer outbound if larger)
                    if (typedStat.stream.kind == "video"
                        && typedStat.outbound.frame_width > 0
                        && typedStat.outbound.frame_height > 0) {
                        int pixels = static_cast<int>(typedStat.outbound.frame_width)
                                     * static_cast<int>(typedStat.outbound.frame_height);
                        if (pixels > bestVideoWidth * bestVideoHeight) {
                            bestVideoWidth = static_cast<int>(typedStat.outbound.frame_width);
                            bestVideoHeight = static_cast<int>(typedStat.outbound.frame_height);
                        }
                        if (std::isfinite(typedStat.outbound.frames_per_second)
                            && typedStat.outbound.frames_per_second > bestVideoFps) {
                            bestVideoFps = typedStat.outbound.frames_per_second;
                        }
                    }
                } else if constexpr (std::is_same_v<T, livekit::RtcRemoteInboundRtpStats>) {
                    accumulateRttMs(typedStat.remote_inbound.round_trip_time, rttSumMs, rttSamples);
                } else if constexpr (std::is_same_v<T, livekit::RtcRemoteOutboundRtpStats>) {
                    accumulateRttMs(typedStat.remote_outbound.round_trip_time, rttSumMs, rttSamples);
                } else if constexpr (std::is_same_v<T, livekit::RtcCandidatePairStats>) {
                    accumulateRttMs(typedStat.candidate_pair.current_round_trip_time, rttSumMs, rttSamples);
                    // Available outgoing bitrate (bps -> kbps)
                    if (std::isfinite(typedStat.candidate_pair.available_outgoing_bitrate)
                        && typedStat.candidate_pair.available_outgoing_bitrate > 0.0) {
                        availableOutgoingBitrate = typedStat.candidate_pair.available_outgoing_bitrate / 1000.0;
                    }
                } else if constexpr (std::is_same_v<T, livekit::RtcCodecStats>) {
                    // Extract codec names (e.g. "audio/opus" -> "opus", "video/VP8" -> "VP8")
                    const QString mimeType = QString::fromStdString(typedStat.codec.mime_type);
                    if (mimeType.startsWith("audio/", Qt::CaseInsensitive) && audioCodec.isEmpty()) {
                        audioCodec = mimeType.mid(6);
                    } else if (mimeType.startsWith("video/", Qt::CaseInsensitive) && videoCodec.isEmpty()) {
                        videoCodec = mimeType.mid(6);
                    }
                } else if constexpr (std::is_same_v<T, livekit::RtcLocalCandidateStats>) {
                    // Transport protocol (e.g. "udp", "tcp")
                    if (transportProtocol.isEmpty() && !typedStat.candidate.protocol.empty()) {
                        transportProtocol = QString::fromStdString(typedStat.candidate.protocol).toUpper();
                    }
                }
            },
            stat.stats);
    }

    if (rttSamples > 0) {
        result.snapshot.rttMs = toRoundedInt(rttSumMs / static_cast<double>(rttSamples));
    }

    if (jitterSamples > 0) {
        result.snapshot.jitterMs = toRoundedInt(jitterSumMs / static_cast<double>(jitterSamples));
    }

    if (packetsReceived + packetsLost > 0) {
        const double loss = (static_cast<double>(packetsLost)
                             / static_cast<double>(packetsReceived + packetsLost)) * 100.0;
        result.snapshot.packetLossPercent = std::clamp(loss, 0.0, 100.0);
    }

    if (previousCounters.valid
        && previousCounters.sampledAtMs > 0
        && nowMs > previousCounters.sampledAtMs) {
        const std::int64_t deltaMs = nowMs - previousCounters.sampledAtMs;
        if (deltaMs >= 500) {
            if (bytesSent >= previousCounters.bytesSent) {
                const double uplinkKbps = (static_cast<double>(bytesSent - previousCounters.bytesSent) * 8.0)
                    / static_cast<double>(deltaMs);
                result.snapshot.uplinkKbps = std::max(0, toRoundedInt(uplinkKbps));
            }

            if (bytesReceived >= previousCounters.bytesReceived) {
                const double downlinkKbps =
                    (static_cast<double>(bytesReceived - previousCounters.bytesReceived) * 8.0)
                    / static_cast<double>(deltaMs);
                result.snapshot.downlinkKbps = std::max(0, toRoundedInt(downlinkKbps));
            }
        }
    }

    // Populate extended fields
    result.snapshot.videoWidth = bestVideoWidth;
    result.snapshot.videoHeight = bestVideoHeight;
    result.snapshot.videoFps = bestVideoFps;
    result.snapshot.audioCodec = audioCodec;
    result.snapshot.videoCodec = videoCodec;
    if (availableOutgoingBitrate >= 0.0) {
        result.snapshot.availableSendBandwidthKbps = std::max(0, toRoundedInt(availableOutgoingBitrate));
    }
    result.snapshot.transportProtocol = transportProtocol;

    result.counters.bytesSent = bytesSent;
    result.counters.bytesReceived = bytesReceived;
    result.counters.sampledAtMs = nowMs;
    result.counters.valid = true;

    return result;
}

NetworkQualityLevel toNetworkQualityLevel(livekit::ConnectionQuality quality)
{
    switch (quality) {
        case livekit::ConnectionQuality::Poor:
            return NetworkQualityLevel::Poor;
        case livekit::ConnectionQuality::Good:
            return NetworkQualityLevel::Good;
        case livekit::ConnectionQuality::Excellent:
            return NetworkQualityLevel::Excellent;
        case livekit::ConnectionQuality::Lost:
            return NetworkQualityLevel::Lost;
        default:
            return NetworkQualityLevel::Unknown;
    }
}

bool hasNetworkStatsData(const NetworkStatsSnapshot& snapshot)
{
    return snapshot.rttMs >= 0
        || snapshot.jitterMs >= 0
        || snapshot.packetLossPercent >= 0.0
        || snapshot.uplinkKbps >= 0
        || snapshot.downlinkKbps >= 0
        || snapshot.videoWidth > 0
        || !snapshot.audioCodec.isEmpty()
        || !snapshot.videoCodec.isEmpty()
        || snapshot.availableSendBandwidthKbps >= 0
        || !snapshot.transportProtocol.isEmpty();
}
