#ifndef CORE_RECORDING_FRAME_CLOCK_H
#define CORE_RECORDING_FRAME_CLOCK_H

#include <cstdint>

namespace links::recording {

std::int64_t computeNextFrameStartUs(std::int64_t elapsedUs,
                                     std::int64_t lastFrameStartUs,
                                     std::int64_t frameIntervalUs,
                                     std::int64_t attemptIndex);

} // namespace links::recording

#endif // CORE_RECORDING_FRAME_CLOCK_H
