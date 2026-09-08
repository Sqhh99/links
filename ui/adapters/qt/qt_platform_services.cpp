#include "qt_platform_services.h"

#include "../../../utils/settings.h"

namespace links {
namespace qt_adapter {

QtPlatformServices::QtPlatformServices(QObject* parent)
    : QObject(parent),
      taskRunner_(this),
      timers_(this),
      camera_(this),
      microphone_(this)
{
    services_.taskRunner = &taskRunner_;
    services_.background = &background_;
    services_.timers = &timers_;
    services_.devices = &devices_;
    services_.camera = &camera_;
    services_.microphone = &microphone_;
    services_.audioPlayers = &audioPlayers_;
}

core::DeviceSelection QtPlatformServices::readDeviceSelection()
{
    auto& settings = Settings::instance();
    core::DeviceSelection out;
    out.cameraId = settings.getSelectedCameraId().toStdString();
    out.microphoneId = settings.getSelectedMicrophoneId().toStdString();
    out.speakerId = settings.getSelectedSpeakerId().toStdString();
    return out;
}

core::AudioProcessingConfig QtPlatformServices::readAudioConfig()
{
    auto& settings = Settings::instance();
    core::AudioProcessingConfig out;
    out.echoCancellation = settings.isEchoCancellationEnabled();
    out.noiseSuppression = settings.isNoiseSuppressionEnabled();
    out.autoGainControl = settings.isAutoGainControlEnabled();
    out.highPassFilter = settings.isHighPassFilterEnabled();
    out.noiseSuppressionLevel = settings.noiseSuppressionLevel();
    out.gainControlMode = settings.gainControlMode();
    out.fixedDigitalGainDb = settings.fixedDigitalGainDb();
    out.adaptiveDigitalMaxGainDb = settings.adaptiveDigitalMaxGainDb();
    out.echoEnhancedFilter = settings.isEchoEnhancedFilterEnabled();
    return out;
}

}  // namespace qt_adapter
}  // namespace links
