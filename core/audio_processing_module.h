#ifndef AUDIO_PROCESSING_MODULE_H
#define AUDIO_PROCESSING_MODULE_H

#include <memory>
#include <cstdint>

// Forward declaration
namespace webrtc {
class AudioProcessing;
}

/**
 * AudioProcessingModule - Wrapper for WebRTC Audio Processing Module
 * 
 * Provides echo cancellation (AEC), noise suppression (NS), 
 * and automatic gain control (AGC) for audio streams.
 * 
 * Two-tier configuration:
 *   - Basic: on/off toggles for AEC, NS, AGC, HPF
 *   - Advanced: NS level, AGC mode/target, AEC enhanced mode
 *
 * IMPORTANT: For AEC to work, the far-end (speaker/playback) audio must be
 * fed via processReverseStream() so the echo canceller has a reference signal.
 */
class AudioProcessingModule {
public:
    // ---- Noise suppression levels ----
    enum class NoiseSuppressionLevel {
        kLow = 0,
        kModerate = 1,
        kHigh = 2,
        kVeryHigh = 3
    };

    // ---- AGC mode ----
    enum class GainControlMode {
        kAdaptiveDigital = 0,   // AGC2 adaptive digital (default)
        kFixedDigital = 1       // AGC2 with fixed gain only
    };

    AudioProcessingModule();
    ~AudioProcessingModule();
    
    // Non-copyable
    AudioProcessingModule(const AudioProcessingModule&) = delete;
    AudioProcessingModule& operator=(const AudioProcessingModule&) = delete;
    
    // Move semantics
    AudioProcessingModule(AudioProcessingModule&&) noexcept;
    AudioProcessingModule& operator=(AudioProcessingModule&&) noexcept;
    
    /**
     * Initialize the audio processing module
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * Check if the module is initialized
     */
    bool isInitialized() const { return apm_ != nullptr; }
    
    // =========================================================================
    // Basic layer: on/off toggles
    // =========================================================================
    void setEchoCancellationEnabled(bool enabled);
    void setNoiseSuppressionEnabled(bool enabled);
    void setAutoGainControlEnabled(bool enabled);
    void setHighPassFilterEnabled(bool enabled);
    
    bool isEchoCancellationEnabled() const { return echoCancellationEnabled_; }
    bool isNoiseSuppressionEnabled() const { return noiseSuppressionEnabled_; }
    bool isAutoGainControlEnabled() const { return autoGainControlEnabled_; }
    bool isHighPassFilterEnabled() const { return highPassFilterEnabled_; }
    
    // =========================================================================
    // Advanced layer: fine-grained parameters
    // =========================================================================

    // -- Noise Suppression --
    void setNoiseSuppressionLevel(NoiseSuppressionLevel level);
    NoiseSuppressionLevel noiseSuppressionLevel() const { return nsLevel_; }

    // -- AGC --
    void setGainControlMode(GainControlMode mode);
    GainControlMode gainControlMode() const { return agcMode_; }
    
    /** Set AGC2 fixed digital gain in dB (0..50). Only used when mode is kFixedDigital. */
    void setFixedDigitalGainDb(float gainDb);
    float fixedDigitalGainDb() const { return fixedDigitalGainDb_; }
    
    /** Set AGC2 adaptive digital max gain in dB (0..50). */
    void setAdaptiveDigitalMaxGainDb(float maxGainDb);
    float adaptiveDigitalMaxGainDb() const { return adaptiveDigitalMaxGainDb_; }

    // -- Echo Canceller --
    /** Enable enhanced AEC filtering (enforce high-pass for AEC). */
    void setEchoEnhancedFilterEnabled(bool enabled);
    bool isEchoEnhancedFilterEnabled() const { return echoEnhancedFilter_; }
    
    /**
     * Process a capture (near-end / microphone) audio frame in-place.
     * 
     * @param data      Pointer to interleaved 16-bit PCM audio samples
     * @param samples   Number of samples (per channel)
     * @param sampleRate Sample rate in Hz (e.g., 48000)
     * @param channels  Number of audio channels (typically 1 for mono)
     * @return true if processing succeeded
     */
    bool processFrame(int16_t* data, int samples, int sampleRate, int channels);

    /**
     * Feed far-end (speaker/playback/render) audio into the APM so the echo
     * canceller can learn the echo path. This MUST be called for AEC to work.
     *
     * @param data      Pointer to interleaved 16-bit PCM audio samples
     * @param samples   Number of samples per channel in this buffer
     * @param sampleRate Sample rate in Hz (e.g., 48000)
     * @param channels  Number of audio channels
     * @return true if processing succeeded
     */
    bool processReverseStream(const int16_t* data, int samples, int sampleRate, int channels);

    /**
     * Set the estimated stream delay in ms between render and capture.
     * Helps AEC align the reference signal. Typical values: 40-120 ms.
     */
    void setStreamDelayMs(int delayMs);
    int streamDelayMs() const { return streamDelayMs_; }
    
private:
    void applyConfig();
    
    std::unique_ptr<webrtc::AudioProcessing> apm_;
    
    // Basic toggles
    bool echoCancellationEnabled_{true};
    bool noiseSuppressionEnabled_{true};
    bool autoGainControlEnabled_{true};
    bool highPassFilterEnabled_{true};
    
    // Advanced: NS
    NoiseSuppressionLevel nsLevel_{NoiseSuppressionLevel::kModerate};
    
    // Advanced: AGC
    GainControlMode agcMode_{GainControlMode::kAdaptiveDigital};
    float fixedDigitalGainDb_{0.0f};
    float adaptiveDigitalMaxGainDb_{50.0f};
    
    // Advanced: AEC
    bool echoEnhancedFilter_{true};
    
    // Stream delay for AEC
    int streamDelayMs_{0};
};

#endif // AUDIO_PROCESSING_MODULE_H
