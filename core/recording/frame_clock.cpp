#include "frame_clock.h"

#include <algorithm>

namespace links::recording {

std::int64_t computeNextFrameStartUs(std::int64_t elapsedUs,
                                     std::int64_t lastFrameStartUs,
                                     std::int64_t frameIntervalUs,
                                     std::int64_t attemptIndex)
{
    const std::int64_t safeInterval = std::max<std::int64_t>(1, frameIntervalUs);
    std::int64_t startUs = elapsedUs;
    if (startUs < 0) {
        startUs = std::max<std::int64_t>(0, attemptIndex) * safeInterval;
    }

    if (lastFrameStartUs >= 0 && startUs <= lastFrameStartUs) {
        startUs = lastFrameStartUs + safeInterval;
    }

    return startUs;
}

} // namespace links::recording
