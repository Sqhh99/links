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

#include <algorithm>

// Helper: convert our NS level enum to WebRTC's
static webrtc::AudioProcessing::Config::NoiseSuppression::Level
toWebrtcNsLevel(AudioProcessingModule::NoiseSuppressionLevel level)
{
    switch (level) {
    case AudioProcessingModule::NoiseSuppressionLevel::kLow:
        return webrtc::AudioProcessing::Config::NoiseSuppression::kLow;
    case AudioProcessingModule::NoiseSuppressionLevel::kModerate:
        return webrtc::AudioProcessing::Config::NoiseSuppression::kModerate;
    case AudioProcessingModule::NoiseSuppressionLevel::kHigh:
        return webrtc::AudioProcessing::Config::NoiseSuppression::kHigh;
    case AudioProcessingModule::NoiseSuppressionLevel::kVeryHigh:
        return webrtc::AudioProcessing::Config::NoiseSuppression::kVeryHigh;
    default:
        return webrtc::AudioProcessing::Config::NoiseSuppression::kModerate;
    }
}

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
    
    // Echo canceller
    config.echo_canceller.enabled = echoCancellationEnabled_;
    config.echo_canceller.mobile_mode = false;
    config.echo_canceller.enforce_high_pass_filtering = echoEnhancedFilter_;
    
    // Noise suppression
    config.noise_suppression.enabled = noiseSuppressionEnabled_;
    config.noise_suppression.level = toWebrtcNsLevel(nsLevel_);
    
    // AGC2
    config.gain_controller2.enabled = autoGainControlEnabled_;
    if (agcMode_ == GainControlMode::kAdaptiveDigital) {
        config.gain_controller2.adaptive_digital.enabled = true;
        config.gain_controller2.adaptive_digital.max_gain_db = adaptiveDigitalMaxGainDb_;
    } else {
        config.gain_controller2.adaptive_digital.enabled = false;
        config.gain_controller2.fixed_digital.gain_db = fixedDigitalGainDb_;
    }
    
    // High-pass filter
    config.high_pass_filter.enabled = highPassFilterEnabled_;
    
    builder.SetConfig(config);
    
    auto apm = builder.Create();
    if (apm) {
        apm_ = std::unique_ptr<webrtc::AudioProcessing>(apm.release());
#ifndef AUDIO_PROCESSING_TESTS
        Logger::instance().info(QString("WebRTC APM initialized (AEC=%1, NS=%2[lvl=%3], AGC=%4, HPF=%5)")
                               .arg(echoCancellationEnabled_)
                               .arg(noiseSuppressionEnabled_)
                               .arg(static_cast<int>(nsLevel_))
                               .arg(autoGainControlEnabled_)
                               .arg(highPassFilterEnabled_));
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
    
    // Echo canceller
    config.echo_canceller.enabled = echoCancellationEnabled_;
    config.echo_canceller.enforce_high_pass_filtering = echoEnhancedFilter_;
    
    // Noise suppression
    config.noise_suppression.enabled = noiseSuppressionEnabled_;
    config.noise_suppression.level = toWebrtcNsLevel(nsLevel_);
    
    // AGC2
    config.gain_controller2.enabled = autoGainControlEnabled_;
    if (agcMode_ == GainControlMode::kAdaptiveDigital) {
        config.gain_controller2.adaptive_digital.enabled = true;
        config.gain_controller2.adaptive_digital.max_gain_db = adaptiveDigitalMaxGainDb_;
    } else {
        config.gain_controller2.adaptive_digital.enabled = false;
        config.gain_controller2.fixed_digital.gain_db = fixedDigitalGainDb_;
    }
    
    // High-pass filter
    config.high_pass_filter.enabled = highPassFilterEnabled_;
    
    apm_->ApplyConfig(config);
    
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("APM config updated (AEC=%1, NS=%2[lvl=%3], AGC=%4, HPF=%5)")
                           .arg(echoCancellationEnabled_)
                           .arg(noiseSuppressionEnabled_)
                           .arg(static_cast<int>(nsLevel_))
                           .arg(autoGainControlEnabled_)
                           .arg(highPassFilterEnabled_));
#endif
}

// =============================================================================
// Basic layer setters
// =============================================================================

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

void AudioProcessingModule::setHighPassFilterEnabled(bool enabled)
{
    highPassFilterEnabled_ = enabled;
    applyConfig();
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("High-pass filter %1").arg(enabled ? "enabled" : "disabled"));
#endif
}

// =============================================================================
// Advanced layer setters
// =============================================================================

void AudioProcessingModule::setNoiseSuppressionLevel(NoiseSuppressionLevel level)
{
    nsLevel_ = level;
    applyConfig();
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("Noise suppression level set to %1").arg(static_cast<int>(level)));
#endif
}

void AudioProcessingModule::setGainControlMode(GainControlMode mode)
{
    agcMode_ = mode;
    applyConfig();
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("AGC mode set to %1")
                           .arg(mode == GainControlMode::kAdaptiveDigital ? "AdaptiveDigital" : "FixedDigital"));
#endif
}

void AudioProcessingModule::setFixedDigitalGainDb(float gainDb)
{
    fixedDigitalGainDb_ = std::max(0.0f, std::min(gainDb, 50.0f));
    applyConfig();
}

void AudioProcessingModule::setAdaptiveDigitalMaxGainDb(float maxGainDb)
{
    adaptiveDigitalMaxGainDb_ = std::max(0.0f, std::min(maxGainDb, 50.0f));
    applyConfig();
}

void AudioProcessingModule::setEchoEnhancedFilterEnabled(bool enabled)
{
    echoEnhancedFilter_ = enabled;
    applyConfig();
#ifndef AUDIO_PROCESSING_TESTS
    Logger::instance().info(QString("AEC enhanced filter %1").arg(enabled ? "enabled" : "disabled"));
#endif
}

void AudioProcessingModule::setStreamDelayMs(int delayMs)
{
    streamDelayMs_ = std::max(0, delayMs);
    if (apm_) {
        apm_->set_stream_delay_ms(streamDelayMs_);
    }
}

// =============================================================================
// Stream processing
// =============================================================================

bool AudioProcessingModule::processFrame(int16_t* data, int samples, int sampleRate, int channels)
{
    if (!apm_ || !data || samples <= 0) {
        return false;
    }
    
    // Set stream delay before processing (helps AEC align render & capture)
    if (streamDelayMs_ > 0) {
        apm_->set_stream_delay_ms(streamDelayMs_);
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

bool AudioProcessingModule::processReverseStream(const int16_t* data, int samples,
                                                  int sampleRate, int channels)
{
    if (!apm_ || !data || samples <= 0) {
        return false;
    }
    
    // Process in 10ms chunks, same as the capture side.
    const int frameSize = sampleRate / 100;
    int processedSamples = 0;
    
    webrtc::StreamConfig streamConfig(sampleRate, channels);
    
    while (processedSamples + frameSize <= samples) {
        const int16_t* framePtr = data + processedSamples * channels;
        
        // Feed far-end audio so AEC can learn the echo path.
        // ProcessReverseStream uses src/dest; we don't need the output so
        // we allocate a small scratch buffer on the stack.
        // The 16-bit int overload: src -> dest (we can use a temp dest).
        int16_t tempDest[48000 / 100 * 2]; // max 10ms at 48kHz stereo = 960
        int result = apm_->ProcessReverseStream(
            framePtr,
            streamConfig,
            streamConfig,
            tempDest
        );
        
        if (result != webrtc::AudioProcessing::kNoError) {
#ifndef AUDIO_PROCESSING_TESTS
            Logger::instance().warning(QString("APM ProcessReverseStream error: %1").arg(result));
#endif
            return false;
        }
        
        processedSamples += frameSize;
    }
    
    return true;
}
