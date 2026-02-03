#include "audio_processing_module.h"

// Conditionally include logger (not available in unit tests without Qt)
#ifndef AUDIO_PROCESSING_TESTS
#include "../utils/logger.h"
#define APM_LOG_INFO(msg) Logger::instance().info(msg)
#define APM_LOG_WARNING(msg) Logger::instance().warning(msg)
#define APM_LOG_ERROR(msg) Logger::instance().error(msg)
#else
// No-op logging for tests
#define APM_LOG_INFO(msg) (void)0
#define APM_LOG_WARNING(msg) (void)0
#define APM_LOG_ERROR(msg) (void)0
#endif

// WebRTC Audio Processing includes
#include "api/audio/audio_processing.h"
#include "api/scoped_refptr.h"

AudioProcessingModule::AudioProcessingModule() = default;

AudioProcessingModule::~AudioProcessingModule() = default;

AudioProcessingModule::AudioProcessingModule(AudioProcessingModule&&) noexcept = default;
AudioProcessingModule& AudioProcessingModule::operator=(AudioProcessingModule&&) noexcept = default;

bool AudioProcessingModule::initialize()
{
    if (apm_) {
        return true; // Already initialized
    }
    
    // Create WebRTC Audio Processing Module using AudioProcessingBuilder
    webrtc::AudioProcessingBuilder builder;
    
    webrtc::AudioProcessing::Config config;
    config.echo_canceller.enabled = echoCancellationEnabled_;
    config.echo_canceller.mobile_mode = false;
    config.noise_suppression.enabled = noiseSuppressionEnabled_;
    config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kModerate;
    config.gain_controller2.enabled = autoGainControlEnabled_;
    config.high_pass_filter.enabled = true;
    
    builder.SetConfig(config);
    
    auto apm = builder.Create();
    if (apm) {
        apm_ = std::unique_ptr<webrtc::AudioProcessing>(apm.release());
#ifndef AUDIO_PROCESSING_TESTS
        Logger::instance().info(QString("WebRTC APM initialized (AEC=%1, NS=%2, AGC=%3)")
                               .arg(echoCancellationEnabled_)
                               .arg(noiseSuppressionEnabled_)
                               .arg(autoGainControlEnabled_));
#endif
        return true;
    } else {
        APM_LOG_ERROR("Failed to create WebRTC Audio Processing Module");
        return false;
    }
}

void AudioProcessingModule::applyConfig()
{
    if (!apm_) {
        return;
    }
    
    webrtc::AudioProcessing::Config config = apm_->GetConfig();
    config.echo_canceller.enabled = echoCancellationEnabled_;
    config.noise_suppression.enabled = noiseSuppressionEnabled_;
    config.gain_controller2.enabled = autoGainControlEnabled_;
    apm_->ApplyConfig(config);
    
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("APM config updated (AEC=%1, NS=%2, AGC=%3)")
                           .arg(echoCancellationEnabled_)
                           .arg(noiseSuppressionEnabled_)
                           .arg(autoGainControlEnabled_));
#endif
}

void AudioProcessingModule::setEchoCancellationEnabled(bool enabled)
{
    echoCancellationEnabled_ = enabled;
    applyConfig();
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("Echo cancellation %1").arg(enabled ? "enabled" : "disabled"));
#endif
}

void AudioProcessingModule::setNoiseSuppressionEnabled(bool enabled)
{
    noiseSuppressionEnabled_ = enabled;
    applyConfig();
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("Noise suppression %1").arg(enabled ? "enabled" : "disabled"));
#endif
}

void AudioProcessingModule::setAutoGainControlEnabled(bool enabled)
{
    autoGainControlEnabled_ = enabled;
    applyConfig();
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("Auto gain control %1").arg(enabled ? "enabled" : "disabled"));
#endif
}

bool AudioProcessingModule::processFrame(int16_t* data, int samples, int sampleRate, int channels)
{
    if (!apm_ || !data || samples <= 0) {
        return false;
    }
    
    // APM expects 10ms frames, so we process in chunks
    const int frameSize = sampleRate / 100; // samples per 10ms
    int processedSamples = 0;
    
    webrtc::StreamConfig streamConfig(sampleRate, channels);
    
    while (processedSamples + frameSize <= samples) {
        int16_t* framePtr = data + processedSamples * channels;
        
        // Process the capture stream (near-end)
        int result = apm_->ProcessStream(
            framePtr,
            streamConfig,
            streamConfig,
            framePtr
        );
        
        if (result != webrtc::AudioProcessing::kNoError) {
#ifndef AUDIO_PROCESSING_TESTS
            Logger::instance().warning(QString("APM ProcessStream error: %1").arg(result));
#endif
            return false;
        }
        
        processedSamples += frameSize;
    }
    
    return true;
}
