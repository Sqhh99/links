#ifndef CORE_CONFERENCE_DEVICE_CONFIG_H
#define CORE_CONFERENCE_DEVICE_CONFIG_H

#include <string>

namespace links {
namespace core {

/**
 * The audio-processing settings core actually consumes.
 *
 * Passed in rather than read from a store: DeviceController used to call
 * Settings::instance() (QSettings) directly, which was the only place core
 * reached into persistence. Ownership of the QSettings file stays in ui/.
 */
struct AudioProcessingConfig {
    bool echoCancellation{true};
    bool noiseSuppression{true};
    bool autoGainControl{true};
    bool highPassFilter{true};
    int noiseSuppressionLevel{2};  // 0=Low,1=Moderate,2=High,3=VeryHigh
    int gainControlMode{0};        // 0=AdaptiveDigital,1=FixedDigital
    float fixedDigitalGainDb{0.0f};
    float adaptiveDigitalMaxGainDb{0.0f};
    bool echoEnhancedFilter{false};
};

struct DeviceSelection {
    std::string cameraId;
    std::string microphoneId;
    std::string speakerId;
};

}  // namespace core
}  // namespace links

#endif  // CORE_CONFERENCE_DEVICE_CONFIG_H
