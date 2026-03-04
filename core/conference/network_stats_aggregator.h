#ifndef CORE_CONFERENCE_NETWORK_STATS_AGGREGATOR_H
#define CORE_CONFERENCE_NETWORK_STATS_AGGREGATOR_H

#include <cstdint>
#include <vector>
#include "conference_types.h"
#include "livekit/room_event_types.h"
#include "livekit/stats.h"

struct NetworkByteCounters {
    std::uint64_t bytesSent{0};
    std::uint64_t bytesReceived{0};
    std::int64_t sampledAtMs{0};
    bool valid{false};
};

struct NetworkStatsAggregationResult {
    NetworkStatsSnapshot snapshot;
    NetworkByteCounters counters;
};

NetworkStatsAggregationResult aggregateNetworkStats(
    const std::vector<livekit::RtcStats>& stats,
    const NetworkByteCounters& previousCounters,
    std::int64_t nowMs);

NetworkQualityLevel toNetworkQualityLevel(livekit::ConnectionQuality quality);
bool hasNetworkStatsData(const NetworkStatsSnapshot& snapshot);

#endif // CORE_CONFERENCE_NETWORK_STATS_AGGREGATOR_H
