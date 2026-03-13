#include "utterance_segmenter.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace links::speech {
namespace {

double averageAbsEnergy(const std::vector<std::int16_t>& frame)
{
    if (frame.empty()) {
        return 0.0;
    }

    std::int64_t sum = 0;
    for (std::int16_t sample : frame) {
        sum += std::abs(static_cast<int>(sample));
    }
    return static_cast<double>(sum) / static_cast<double>(frame.size());
}

}  // namespace

UtteranceSegmenter::UtteranceSegmenter(Config config)
    : config_(std::move(config))
{
}

std::vector<SpeechSegment> UtteranceSegmenter::appendFrame(const std::vector<std::int16_t>& frame)
{
    std::vector<SpeechSegment> output;
    if (frame.empty()) {
        return output;
    }

    const double energy = averageAbsEnergy(frame);
    const bool voiced = energy >= config_.energyThreshold;
    const int maxFrames = std::max(1, config_.maxSegmentMs / config_.frameMs);
    const int minSpeechFrames = std::max(1, config_.minSpeechMs / config_.frameMs);
    const int silenceFrames = std::max(1, config_.endSilenceMs / config_.frameMs);

    if (voiced) {
        if (!inSpeech_) {
            inSpeech_ = true;
            speechStartFrame_ = frameIndex_;
            speechFrameCount_ = 0;
            silenceFrameCount_ = 0;
            currentSegment_.clear();
        }
        ++speechFrameCount_;
        silenceFrameCount_ = 0;
    } else if (inSpeech_) {
        ++silenceFrameCount_;
    }

    if (inSpeech_) {
        currentSegment_.insert(currentSegment_.end(), frame.begin(), frame.end());
        if (static_cast<int>(currentSegment_.size()) >= maxFrames * static_cast<int>(frame.size())) {
            finalizeSegment(&output);
        } else if (!voiced && silenceFrameCount_ >= silenceFrames) {
            if (speechFrameCount_ >= minSpeechFrames) {
                finalizeSegment(&output);
            } else {
                currentSegment_.clear();
                inSpeech_ = false;
                speechFrameCount_ = 0;
                silenceFrameCount_ = 0;
                speechStartFrame_ = frameIndex_ + 1;
            }
        }
    }

    ++frameIndex_;
    return output;
}

std::vector<SpeechSegment> UtteranceSegmenter::flush()
{
    std::vector<SpeechSegment> output;
    if (inSpeech_ && speechFrameCount_ > 0) {
        finalizeSegment(&output);
    }
    reset();
    return output;
}

void UtteranceSegmenter::reset()
{
    currentSegment_.clear();
    inSpeech_ = false;
    speechFrameCount_ = 0;
    silenceFrameCount_ = 0;
    frameIndex_ = 0;
    speechStartFrame_ = 0;
}

void UtteranceSegmenter::finalizeSegment(std::vector<SpeechSegment>* output)
{
    if (!output || currentSegment_.empty()) {
        reset();
        return;
    }

    const std::int64_t startMs = static_cast<std::int64_t>(speechStartFrame_) * config_.frameMs;
    const std::int64_t endMs = static_cast<std::int64_t>(frameIndex_ + 1) * config_.frameMs;
    output->push_back(SpeechSegment{currentSegment_, startMs, endMs});
    currentSegment_.clear();
    inSpeech_ = false;
    speechFrameCount_ = 0;
    silenceFrameCount_ = 0;
    speechStartFrame_ = frameIndex_ + 1;
}

}  // namespace links::speech
