#ifndef CORE_BASE_TIME_H
#define CORE_BASE_TIME_H

#include <chrono>
#include <cstdint>

namespace links {
namespace core {

/// Wall-clock milliseconds since the Unix epoch.
/// Replaces QDateTime::currentMSecsSinceEpoch(); same epoch, same units.
inline std::int64_t nowMsSinceEpoch()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/// Monotonic clock for measuring elapsed intervals. Replaces QElapsedTimer.
using SteadyClock = std::chrono::steady_clock;

inline std::int64_t elapsedMs(SteadyClock::time_point since)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               SteadyClock::now() - since)
        .count();
}

}  // namespace core
}  // namespace links

#endif  // CORE_BASE_TIME_H
