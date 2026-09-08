#include "audio_processing_module.h"

#include "base/log.h"
#include "base/strings.h"

// The AUDIO_PROCESSING_TESTS guards that used to wrap every log call are gone:
// core::log has no Qt dependency, so this file compiles identically in the app
// and in the unit tests.
#define APM_LOG_INFO(msg) links::core::logInfo(msg)
#define APM_LOG_WARNING(msg) links::core::logWarning(msg)
#define APM_LOG_ERROR(msg) links::core::logError(msg)

namespace core = links::core;

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
        core::logInfo(core::str::cat("WebRTC APM initialized (AEC=", echoCancellationEnabled_, ", NS=", noiseSuppressionEnabled_, "[lvl=", static_cast<int>(nsLevel_), "], AGC=", autoGainControlEnabled_, ", HPF=", highPassFilterEnabled_, ")"));
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
    
    core::logInfo(core::str::cat("APM config updated (AEC=", echoCancellationEnabled_, ", NS=", noiseSuppressionEnabled_, "[lvl=", static_cast<int>(nsLevel_), "], AGC=", autoGainControlEnabled_, ", HPF=", highPassFilterEnabled_, ")"));
}

// =============================================================================
// Basic layer setters
// =============================================================================

void AudioProcessingModule::setEchoCancellationEnabled(bool enabled)
{
    echoCancellationEnabled_ = enabled;
    applyConfig();
    core::logInfo(core::str::cat("Echo cancellation ", enabled ? "enabled" : "disabled"));
}

void AudioProcessingModule::setNoiseSuppressionEnabled(bool enabled)
{
    noiseSuppressionEnabled_ = enabled;
    applyConfig();
    core::logInfo(core::str::cat("Noise suppression ", enabled ? "enabled" : "disabled"));
}

void AudioProcessingModule::setAutoGainControlEnabled(bool enabled)
{
    autoGainControlEnabled_ = enabled;
    applyConfig();
    core::logInfo(core::str::cat("Auto gain control ", enabled ? "enabled" : "disabled"));
}

void AudioProcessingModule::setHighPassFilterEnabled(bool enabled)
{
    highPassFilterEnabled_ = enabled;
    applyConfig();
    core::logInfo(core::str::cat("High-pass filter ", enabled ? "enabled" : "disabled"));
}

// =============================================================================
// Advanced layer setters
// =============================================================================

void AudioProcessingModule::setNoiseSuppressionLevel(NoiseSuppressionLevel level)
{
    nsLevel_ = level;
    applyConfig();
    core::logInfo(core::str::cat("Noise suppression level set to ", static_cast<int>(level)));
}

void AudioProcessingModule::setGainControlMode(GainControlMode mode)
{
    agcMode_ = mode;
    applyConfig();
    core::logInfo(core::str::cat("AGC mode set to ", mode == GainControlMode::kAdaptiveDigital ? "AdaptiveDigital" : "FixedDigital"));
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
    core::logInfo(core::str::cat("AEC enhanced filter ", enabled ? "enabled" : "disabled"));
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
            core::logWarning(core::str::cat("APM ProcessStream error: ", result));
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
    
    // Allocate scratch buffer once, sized to the actual 10 ms frame dimensions.
    // This avoids the previous fixed-size stack buffer that could overflow
    // with sample rates above 48 kHz or channel counts above 2.
    std::vector<int16_t> tempDest(frameSize * channels);
    
    while (processedSamples + frameSize <= samples) {
        const int16_t* framePtr = data + processedSamples * channels;
        
        // Feed far-end audio so AEC can learn the echo path.
        // ProcessReverseStream uses src/dest; we don't need the output.
        int result = apm_->ProcessReverseStream(
            framePtr,
            streamConfig,
            streamConfig,
            tempDest.data()
        );
        
        if (result != webrtc::AudioProcessing::kNoError) {
            core::logWarning(core::str::cat("APM ProcessReverseStream error: ", result));
            return false;
        }
        
        processedSamples += frameSize;
    }
    
    return true;
}
