#include <gtest/gtest.h>

#include <vector>

#include "core/conference/network_stats_aggregator.h"

namespace {

livekit::RtcStats makeInboundStats(std::uint64_t packetsReceived,
                                   std::int64_t packetsLost,
                                   double jitterSeconds,
                                   std::uint64_t bytesReceived)
{
    livekit::RtcInboundRtpStats inbound;
    inbound.received.packets_received = packetsReceived;
    inbound.received.packets_lost = packetsLost;
    inbound.received.jitter = jitterSeconds;
    inbound.inbound.bytes_received = bytesReceived;

    livekit::RtcStats stats;
    stats.stats = inbound;
    return stats;
}

livekit::RtcStats makeOutboundStats(std::uint64_t bytesSent)
{
    livekit::RtcOutboundRtpStats outbound;
    outbound.sent.bytes_sent = bytesSent;

    livekit::RtcStats stats;
    stats.stats = outbound;
    return stats;
}

livekit::RtcStats makeRemoteInboundStats(double rttSeconds)
{
    livekit::RtcRemoteInboundRtpStats remoteInbound;
    remoteInbound.remote_inbound.round_trip_time = rttSeconds;

    livekit::RtcStats stats;
    stats.stats = remoteInbound;
    return stats;
}

livekit::RtcStats makeCandidatePairStats(double rttSeconds)
{
    livekit::RtcCandidatePairStats candidatePair;
    candidatePair.candidate_pair.current_round_trip_time = rttSeconds;

    livekit::RtcStats stats;
    stats.stats = candidatePair;
    return stats;
}

} // namespace

TEST(NetworkStatsAggregatorTest, AggregatesRttJitterLossAndBitrate)
{
    const std::vector<livekit::RtcStats> firstSample{
        makeInboundStats(190, 10, 0.015, 300000),
        makeOutboundStats(200000),
        makeRemoteInboundStats(0.050),
    };

    const NetworkByteCounters emptyCounters;
    const auto firstResult = aggregateNetworkStats(firstSample, emptyCounters, 1000);

    EXPECT_EQ(firstResult.snapshot.rttMs, 50);
    EXPECT_EQ(firstResult.snapshot.jitterMs, 15);
    EXPECT_NEAR(firstResult.snapshot.packetLossPercent, 5.0, 0.001);
    EXPECT_EQ(firstResult.snapshot.uplinkKbps, -1);
    EXPECT_EQ(firstResult.snapshot.downlinkKbps, -1);

    const std::vector<livekit::RtcStats> secondSample{
        makeInboundStats(380, 20, 0.014, 420000),
        makeOutboundStats(260000),
        makeRemoteInboundStats(0.055),
    };
    const auto secondResult =
        aggregateNetworkStats(secondSample, firstResult.counters, 3000);

    EXPECT_EQ(secondResult.snapshot.rttMs, 55);
    EXPECT_EQ(secondResult.snapshot.jitterMs, 14);
    EXPECT_NEAR(secondResult.snapshot.packetLossPercent, 5.0, 0.001);
    EXPECT_EQ(secondResult.snapshot.uplinkKbps, 240);
    EXPECT_EQ(secondResult.snapshot.downlinkKbps, 480);
}

TEST(NetworkStatsAggregatorTest, UsesCandidatePairRttWhenRemoteInboundMissing)
{
    const std::vector<livekit::RtcStats> sample{
        makeInboundStats(100, 0, 0.010, 200000),
        makeOutboundStats(100000),
        makeCandidatePairStats(0.087),
    };

    const auto result = aggregateNetworkStats(sample, NetworkByteCounters{}, 1500);
    EXPECT_EQ(result.snapshot.rttMs, 87);
}

TEST(NetworkStatsAggregatorTest, MapsConnectionQualityLevels)
{
    EXPECT_EQ(toNetworkQualityLevel(livekit::ConnectionQuality::Poor),
              NetworkQualityLevel::Poor);
    EXPECT_EQ(toNetworkQualityLevel(livekit::ConnectionQuality::Good),
              NetworkQualityLevel::Good);
    EXPECT_EQ(toNetworkQualityLevel(livekit::ConnectionQuality::Excellent),
              NetworkQualityLevel::Excellent);
    EXPECT_EQ(toNetworkQualityLevel(livekit::ConnectionQuality::Lost),
              NetworkQualityLevel::Lost);
}

TEST(NetworkStatsAggregatorTest, DetectsMetricsAvailability)
{
    NetworkStatsSnapshot empty;
    EXPECT_FALSE(hasNetworkStatsData(empty));

    NetworkStatsSnapshot withRtt;
    withRtt.rttMs = 35;
    EXPECT_TRUE(hasNetworkStatsData(withRtt));
}
