#include "audio_resampler.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace links {
namespace core {
namespace {

float clampSample(float value)
{
    return std::max(-1.0f, std::min(1.0f, value));
}

}  // namespace

std::vector<float> mixAndResampleToFloat(const std::vector<std::int16_t>& input,
                                         int srcRate,
                                         int srcChannels,
                                         int dstRate,
                                         int dstChannels)
{
    if (input.empty() || srcRate <= 0 || dstRate <= 0 || srcChannels <= 0 || dstChannels <= 0) {
        return {};
    }

    const int srcFrames = static_cast<int>(input.size()) / srcChannels;
    if (srcFrames <= 0) {
        return {};
    }

    const double ratio = static_cast<double>(dstRate) / static_cast<double>(srcRate);
    const int dstFrames = std::max(1, static_cast<int>(std::llround(srcFrames * ratio)));
    std::vector<float> output(static_cast<std::size_t>(dstFrames * dstChannels), 0.0f);

    for (int dstFrame = 0; dstFrame < dstFrames; ++dstFrame) {
        const double srcPos = static_cast<double>(dstFrame) / ratio;
        const int srcIndex = std::min(srcFrames - 1, std::max(0, static_cast<int>(std::llround(srcPos))));

        float left = 0.0f;
        float right = 0.0f;
        if (srcChannels == 1) {
            left = right = static_cast<float>(input[srcIndex]) / 32768.0f;
        } else {
            const int base = srcIndex * srcChannels;
            left = static_cast<float>(input[base]) / 32768.0f;
            right = static_cast<float>(input[base + 1]) / 32768.0f;
        }

        for (int dstChannel = 0; dstChannel < dstChannels; ++dstChannel) {
            float sample = 0.0f;
            if (dstChannels == 1) {
                sample = (left + right) * 0.5f;
            } else {
                sample = (dstChannel % 2 == 0) ? left : right;
            }
            output[static_cast<std::size_t>(dstFrame * dstChannels + dstChannel)] = clampSample(sample);
        }
    }

    return output;
}

std::vector<std::uint8_t> packFloatPcm(const std::vector<float>& input, const AudioFormat& format)
{
    if (input.empty()) {
        return {};
    }

    const int bytesPerSample = format.bytesPerSample();
    if (bytesPerSample <= 0) {
        return {};
    }

    std::vector<std::uint8_t> output(input.size() * static_cast<std::size_t>(bytesPerSample));
    std::uint8_t* dst = output.data();

    for (std::size_t i = 0; i < input.size(); ++i) {
        const float sample = clampSample(input[i]);
        const std::size_t offset = i * static_cast<std::size_t>(bytesPerSample);
        switch (format.sampleFormat) {
        case SampleFormat::UInt8: {
            const std::uint8_t value =
                static_cast<std::uint8_t>(std::lround((sample * 0.5f + 0.5f) * 255.0f));
            std::memcpy(dst + offset, &value, sizeof(value));
            break;
        }
        case SampleFormat::Int16: {
            const std::int16_t value = static_cast<std::int16_t>(std::lround(sample * 32767.0f));
            std::memcpy(dst + offset, &value, sizeof(value));
            break;
        }
        case SampleFormat::Int32: {
            const std::int32_t value = static_cast<std::int32_t>(std::lround(sample * 2147483647.0f));
            std::memcpy(dst + offset, &value, sizeof(value));
            break;
        }
        case SampleFormat::Float: {
            std::memcpy(dst + offset, &sample, sizeof(sample));
            break;
        }
        case SampleFormat::Unknown:
            return {};
        }
    }

    return output;
}

}  // namespace core
}  // namespace links
