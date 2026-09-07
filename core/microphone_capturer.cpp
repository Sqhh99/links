#include "microphone_capturer.h"

#include "base/log.h"
#include "base/strings.h"

namespace core = links::core;

MicrophoneCapturer::MicrophoneCapturer(core::AudioInput& input)
    : input_(input)
{
    input_.setDataCallback([this](const std::int16_t* data, std::size_t count) {
        onSamples(data, count);
    });
    input_.setErrorCallback([this](const std::string& message) { error.notify(message); });
}

MicrophoneCapturer::~MicrophoneCapturer()
{
    stop();
}

bool MicrophoneCapturer::start()
{
    if (isActive_) {
        return true;
    }

    try {
        livekitAudioSource_ = std::make_shared<livekit::AudioSource>(48000, 1, 0);
        core::logInfo("LiveKit AudioSource created for microphone");
    } catch (const std::exception& e) {
        core::logError(core::str::cat("Failed to create AudioSource: ", e.what()));
        error.notify(core::str::cat("Failed to create audio source: ", e.what()));
        return false;
    }

    if (!input_.start(format_)) {
        core::logError("Failed to start audio input");
        livekitAudioSource_.reset();
        return false;
    }

    isActive_ = true;
    samplesProcessed_ = 0;
    lastLoggedSamples_ = 0;

    core::logInfo(core::str::cat("Microphone started (rate: ", format_.sampleRate,
                                 ", channels: ", format_.channels, ")"));
    return true;
}

void MicrophoneCapturer::stop()
{
    if (!isActive_) {
        return;
    }

    input_.stop();
    isActive_ = false;

    livekitAudioSource_.reset();
    audioBuffer_.clear();

    core::logInfo(core::str::cat("Microphone stopped (processed ", samplesProcessed_, " samples)"));
}

void MicrophoneCapturer::setDeviceById(const std::string& deviceId)
{
    if (isActive_) {
        core::logWarning("Cannot change microphone while active");
        return;
    }
    input_.setDeviceId(deviceId);
}

void MicrophoneCapturer::onSamples(const std::int16_t* data, std::size_t sampleCount)
{
    if (!livekitAudioSource_ || !data || sampleCount == 0) {
        return;
    }

    audioBuffer_.insert(audioBuffer_.end(), data, data + sampleCount);
    sendBufferedFrames();
}

void MicrophoneCapturer::sendBufferedFrames()
{
    if (!livekitAudioSource_) {
        return;
    }

    const int numChannels = format_.channels;
    const int frameSizeTotalSamples = kFrameSizeSamples * numChannels;

    // Process all complete 10ms frames in the buffer
    while (audioBuffer_.size() >= static_cast<std::size_t>(frameSizeTotalSamples)) {
        try {
            std::vector<int16_t> frameData(audioBuffer_.begin(),
                                           audioBuffer_.begin() + frameSizeTotalSamples);

            // Process through Audio Processing Module (in-place)
            if (apm_.isInitialized()) {
                apm_.processFrame(frameData.data(), kFrameSizeSamples,
                                  format_.sampleRate, numChannels);
            }

            livekit::AudioFrame frame(std::move(frameData),
                                      format_.sampleRate,
                                      numChannels,
                                      kFrameSizeSamples);

            livekitAudioSource_->captureFrame(frame);

            audioBuffer_.erase(audioBuffer_.begin(),
                               audioBuffer_.begin() + frameSizeTotalSamples);

            samplesProcessed_ += kFrameSizeSamples;

        } catch (const std::exception& e) {
            core::logError(core::str::cat("Failed to capture audio: ", e.what()));
            break;
        }
    }

    // Log every second (48000 samples at 48kHz). Was a function-local static,
    // which made the cadence global across instances; now per-capturer.
    if (samplesProcessed_ - lastLoggedSamples_ >= 48000) {
        core::logDebug(core::str::cat("Captured ", samplesProcessed_, " audio samples (buffer: ",
                                      static_cast<unsigned long long>(audioBuffer_.size()), ")"));
        lastLoggedSamples_ = samplesProcessed_;
    }
}

void MicrophoneCapturer::setEchoCancellationEnabled(bool enabled)
{
    apm_.setEchoCancellationEnabled(enabled);
}

void MicrophoneCapturer::setNoiseSuppressionEnabled(bool enabled)
{
    apm_.setNoiseSuppressionEnabled(enabled);
}

void MicrophoneCapturer::setAutoGainControlEnabled(bool enabled)
{
    apm_.setAutoGainControlEnabled(enabled);
}

void MicrophoneCapturer::setHighPassFilterEnabled(bool enabled)
{
    apm_.setHighPassFilterEnabled(enabled);
}

void MicrophoneCapturer::setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel level)
{
    apm_.setNoiseSuppressionLevel(level);
}

void MicrophoneCapturer::setGainControlMode(AudioProcessingModule::GainControlMode mode)
{
    apm_.setGainControlMode(mode);
}

void MicrophoneCapturer::setFixedDigitalGainDb(float gainDb)
{
    apm_.setFixedDigitalGainDb(gainDb);
}

void MicrophoneCapturer::setAdaptiveDigitalMaxGainDb(float maxGainDb)
{
    apm_.setAdaptiveDigitalMaxGainDb(maxGainDb);
}

void MicrophoneCapturer::setEchoEnhancedFilterEnabled(bool enabled)
{
    apm_.setEchoEnhancedFilterEnabled(enabled);
}

void MicrophoneCapturer::setStreamDelayMs(int delayMs)
{
    apm_.setStreamDelayMs(delayMs);
}

void MicrophoneCapturer::feedReverseStream(const int16_t* data, int samples,
                                            int sampleRate, int channels)
{
    if (apm_.isInitialized()) {
        apm_.processReverseStream(data, samples, sampleRate, channels);
    }
}
