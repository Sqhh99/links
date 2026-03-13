#ifndef LINKS_CORE_SPEECH_UTTERANCE_SEGMENTER_H_
#define LINKS_CORE_SPEECH_UTTERANCE_SEGMENTER_H_

#include <cstdint>
#include <vector>

namespace links::speech {

struct SpeechSegment {
    std::vector<std::int16_t> samples;
    std::int64_t startMs{0};
    std::int64_t endMs{0};
};

class UtteranceSegmenter {
public:
    struct Config {
        int sampleRate{16000};
        int frameMs{10};
        int minSpeechMs{900};
        int endSilenceMs{1500};
        int maxSegmentMs{15000};
        double energyThreshold{450.0};
    };

    explicit UtteranceSegmenter(Config config = Config{});

    std::vector<SpeechSegment> appendFrame(const std::vector<std::int16_t>& frame);
    std::vector<SpeechSegment> flush();
    void reset();

private:
    void finalizeSegment(std::vector<SpeechSegment>* output);

    Config config_;
    std::vector<std::int16_t> currentSegment_;
    bool inSpeech_{false};
    int speechFrameCount_{0};
    int silenceFrameCount_{0};
    int frameIndex_{0};
    int speechStartFrame_{0};
};

}  // namespace links::speech

#endif  // LINKS_CORE_SPEECH_UTTERANCE_SEGMENTER_H_
