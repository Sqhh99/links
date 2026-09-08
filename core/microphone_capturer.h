#ifndef MICROPHONE_CAPTURER_H
#define MICROPHONE_CAPTURER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "audio_processing_module.h"
#include "base/signal.h"
#include "livekit/audio_source.h"
#include "media/audio_input.h"

/**
 * Turns raw microphone PCM into 10 ms LiveKit audio frames.
 *
 * The QAudioSource plumbing moved behind links::core::AudioInput; the parts
 * that matter -- the 480-sample framing buffer, the WebRTC
 * AudioProcessingModule and the AEC reverse stream -- stayed here.
 */
class MicrophoneCapturer
{
public:
    explicit MicrophoneCapturer(links::core::AudioInput& input);
    ~MicrophoneCapturer();

    bool start();
    void stop();
    bool isActive() const { return isActive_; }

    std::shared_ptr<livekit::AudioSource> getAudioSource() const { return livekitAudioSource_; }

    /// Must be called before start(). Empty id selects the system default.
    void setDeviceById(const std::string& deviceId);

    // Audio processing options (AEC, NS, AGC) - delegates to AudioProcessingModule
    void setEchoCancellationEnabled(bool enabled);
    void setNoiseSuppressionEnabled(bool enabled);
    void setAutoGainControlEnabled(bool enabled);
    void setHighPassFilterEnabled(bool enabled);

    // Advanced audio processing parameters
    void setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel level);
    void setGainControlMode(AudioProcessingModule::GainControlMode mode);
    void setFixedDigitalGainDb(float gainDb);
    void setAdaptiveDigitalMaxGainDb(float maxGainDb);
    void setEchoEnhancedFilterEnabled(bool enabled);
    void setStreamDelayMs(int delayMs);

    /**
     * Feed far-end (speaker) audio into the APM for echo cancellation.
     * Must be called with playback audio data for AEC to work.
     */
    void feedReverseStream(const int16_t* data, int samples, int sampleRate, int channels);

    AudioProcessingModule* audioProcessingModule() { return &apm_; }

    links::core::Signal<const std::string&> error;

private:
    void onSamples(const std::int16_t* data, std::size_t sampleCount);
    void sendBufferedFrames();

    links::core::AudioInput& input_;
    std::shared_ptr<livekit::AudioSource> livekitAudioSource_;
    links::core::AudioFormat format_{48000, 1, links::core::SampleFormat::Int16};

    bool isActive_{false};
    std::int64_t samplesProcessed_{0};
    std::int64_t lastLoggedSamples_{0};

    AudioProcessingModule apm_;

    // At 48kHz mono, 10ms = 480 samples
    static constexpr int kFrameSizeSamples = 480;
    std::vector<int16_t> audioBuffer_;
};

#endif // MICROPHONE_CAPTURER_H
