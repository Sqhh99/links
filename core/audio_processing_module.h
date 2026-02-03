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
 * This class is designed to be reusable and decoupled from any specific
 * audio capture implementation.
 */
class AudioProcessingModule {
public:
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
    
    /**
     * Configure audio processing options
     */
    void setEchoCancellationEnabled(bool enabled);
    void setNoiseSuppressionEnabled(bool enabled);
    void setAutoGainControlEnabled(bool enabled);
    
    bool isEchoCancellationEnabled() const { return echoCancellationEnabled_; }
    bool isNoiseSuppressionEnabled() const { return noiseSuppressionEnabled_; }
    bool isAutoGainControlEnabled() const { return autoGainControlEnabled_; }
    
    /**
     * Process an audio frame in-place
     * 
     * @param data      Pointer to interleaved 16-bit PCM audio samples
     * @param samples   Number of samples (per channel)
     * @param sampleRate Sample rate in Hz (e.g., 48000)
     * @param channels  Number of audio channels (typically 1 for mono)
     * @return true if processing succeeded
     */
    bool processFrame(int16_t* data, int samples, int sampleRate, int channels);
    
private:
    void applyConfig();
    
    std::unique_ptr<webrtc::AudioProcessing> apm_;
    bool echoCancellationEnabled_{true};
    bool noiseSuppressionEnabled_{true};
    bool autoGainControlEnabled_{true};
};

#endif // AUDIO_PROCESSING_MODULE_H
