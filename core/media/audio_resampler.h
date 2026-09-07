#ifndef CORE_MEDIA_AUDIO_RESAMPLER_H
#define CORE_MEDIA_AUDIO_RESAMPLER_H

#include <cstdint>
#include <vector>

#include "audio_format.h"

namespace links {
namespace core {

/**
 * Nearest-neighbour resample + channel mix to float, then pack to the target
 * sample format. Extracted verbatim from MediaPipeline so it is unit-testable
 * and free of Qt; the arithmetic is unchanged.
 */
std::vector<float> mixAndResampleToFloat(const std::vector<std::int16_t>& input,
                                         int srcRate,
                                         int srcChannels,
                                         int dstRate,
                                         int dstChannels);

std::vector<std::uint8_t> packFloatPcm(const std::vector<float>& input,
                                       const AudioFormat& format);

}  // namespace core
}  // namespace links

#endif  // CORE_MEDIA_AUDIO_RESAMPLER_H
