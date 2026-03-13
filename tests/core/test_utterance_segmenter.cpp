#include <gtest/gtest.h>

#include "core/speech/utterance_segmenter.h"

namespace {

std::vector<std::int16_t> makeFrame(std::int16_t value)
{
    return std::vector<std::int16_t>(160, value);
}

}  // namespace

TEST(UtteranceSegmenterTest, ProducesSegmentAfterTrailingSilence)
{
    links::speech::UtteranceSegmenter segmenter;
    std::vector<links::speech::SpeechSegment> results;

    for (int index = 0; index < 40; ++index) {
        auto output = segmenter.appendFrame(makeFrame(1200));
        results.insert(results.end(), output.begin(), output.end());
    }
    for (int index = 0; index < 80; ++index) {
        auto output = segmenter.appendFrame(makeFrame(0));
        results.insert(results.end(), output.begin(), output.end());
    }

    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results.front().samples.empty());
    EXPECT_GE(results.front().endMs, results.front().startMs);
}

TEST(UtteranceSegmenterTest, IgnoresShortNoiseBursts)
{
    links::speech::UtteranceSegmenter segmenter;
    std::vector<links::speech::SpeechSegment> results;

    for (int index = 0; index < 5; ++index) {
        auto output = segmenter.appendFrame(makeFrame(1200));
        results.insert(results.end(), output.begin(), output.end());
    }
    for (int index = 0; index < 80; ++index) {
        auto output = segmenter.appendFrame(makeFrame(0));
        results.insert(results.end(), output.begin(), output.end());
    }

    EXPECT_TRUE(results.empty());
}
