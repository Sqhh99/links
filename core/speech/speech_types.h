#ifndef LINKS_CORE_SPEECH_SPEECH_TYPES_H_
#define LINKS_CORE_SPEECH_SPEECH_TYPES_H_

#include <cstdint>
#include <string>

namespace links::speech {

struct ModelDescriptor {
    std::string path;
    std::string displayName;
    std::string format;
};

struct TranscriptionResult {
    std::string text;
    std::int64_t startMs{0};
    std::int64_t endMs{0};
};

struct AudioProcessingOptions {
    bool echoCancellationEnabled{true};
    bool noiseSuppressionEnabled{true};
    bool autoGainControlEnabled{true};
    bool highPassFilterEnabled{true};
    int noiseSuppressionLevel{1};
    int gainControlMode{0};
    float fixedDigitalGainDb{0.0f};
    float adaptiveDigitalMaxGainDb{50.0f};
    bool echoEnhancedFilterEnabled{true};
};

}  // namespace links::speech

#endif  // LINKS_CORE_SPEECH_SPEECH_TYPES_H_
