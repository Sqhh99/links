#include "realtime_transcription_session.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

#if LINKS_ENABLE_SPEECH_TO_TEXT
#include <whisper.h>
#endif

namespace links::speech {
namespace {

AudioProcessingModule::NoiseSuppressionLevel toNoiseSuppressionLevel(int level)
{
    switch (std::clamp(level, 0, 3)) {
    case 0:
        return AudioProcessingModule::NoiseSuppressionLevel::kLow;
    case 1:
        return AudioProcessingModule::NoiseSuppressionLevel::kModerate;
    case 2:
        return AudioProcessingModule::NoiseSuppressionLevel::kHigh;
    case 3:
    default:
        return AudioProcessingModule::NoiseSuppressionLevel::kVeryHigh;
    }
}

AudioProcessingModule::GainControlMode toGainControlMode(int mode)
{
    return mode == 1
        ? AudioProcessingModule::GainControlMode::kFixedDigital
        : AudioProcessingModule::GainControlMode::kAdaptiveDigital;
}

}  // namespace

RealtimeTranscriptionSession::RealtimeTranscriptionSession()
    : segmenter_(UtteranceSegmenter::Config{})
{
    apmInitialized_ = apm_.initialize();
    setAudioProcessingOptions(audioOptions_);
}

RealtimeTranscriptionSession::~RealtimeTranscriptionSession()
{
    stop();
#if LINKS_ENABLE_SPEECH_TO_TEXT
    if (whisperContext_) {
        whisper_free(whisperContext_);
        whisperContext_ = nullptr;
    }
#endif
}

bool RealtimeTranscriptionSession::isAvailable() const
{
#if LINKS_ENABLE_SPEECH_TO_TEXT
    return true;
#else
    return false;
#endif
}

bool RealtimeTranscriptionSession::loadModel(const std::string& modelPath)
{
    bool loaded = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (modelLoaded_ && whisperContext_ && modelPath_ == modelPath) {
            loaded = true;
        } else {
            loaded = loadModelInternal(modelPath);
            modelLoaded_ = loaded;
            if (loaded) {
                modelPath_ = modelPath;
            }
        }
    }
    emitStatus(loaded ? "模型已加载" : "模型加载失败");
    return loaded;
}

bool RealtimeTranscriptionSession::start()
{
    std::string status;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isAvailable()) {
            status = "当前平台未启用语音转文字预览";
        } else if (!modelLoaded_) {
            status = "请先选择并加载模型";
        } else if (running_) {
            return true;
        } else {
            workerStopRequested_ = false;
            running_ = true;
            segmenter_.reset();
            processingBuffer_.clear();
            workerThread_ = std::thread(&RealtimeTranscriptionSession::runWorker, this);
        }
    }

    if (!status.empty()) {
        emitStatus(status);
        return false;
    }

    emitStatus("预览中");
    return true;
}

void RealtimeTranscriptionSession::stop()
{
    bool shouldEmitStopped = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ && !workerThread_.joinable()) {
            return;
        }
        running_ = false;
        workerStopRequested_ = true;
        while (!pendingSegments_.empty()) {
            pendingSegments_.pop();
        }
        processingBuffer_.clear();
        segmenter_.reset();
        shouldEmitStopped = true;
    }

    condition_.notify_all();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    if (shouldEmitStopped) {
        emitStatus("已停止");
    }
}

void RealtimeTranscriptionSession::reset()
{
    stop();
    std::lock_guard<std::mutex> lock(mutex_);
    modelLoaded_ = false;
    modelPath_.clear();
#if LINKS_ENABLE_SPEECH_TO_TEXT
    if (whisperContext_) {
        whisper_free(whisperContext_);
        whisperContext_ = nullptr;
    }
#endif
}

void RealtimeTranscriptionSession::setAudioProcessingOptions(const AudioProcessingOptions& options)
{
    std::lock_guard<std::mutex> lock(mutex_);
    audioOptions_ = options;
    if (!apmInitialized_) {
        return;
    }

    apm_.setEchoCancellationEnabled(audioOptions_.echoCancellationEnabled);
    apm_.setNoiseSuppressionEnabled(audioOptions_.noiseSuppressionEnabled);
    apm_.setAutoGainControlEnabled(audioOptions_.autoGainControlEnabled);
    apm_.setHighPassFilterEnabled(audioOptions_.highPassFilterEnabled);
    apm_.setNoiseSuppressionLevel(toNoiseSuppressionLevel(audioOptions_.noiseSuppressionLevel));
    apm_.setGainControlMode(toGainControlMode(audioOptions_.gainControlMode));
    apm_.setFixedDigitalGainDb(audioOptions_.fixedDigitalGainDb);
    apm_.setAdaptiveDigitalMaxGainDb(audioOptions_.adaptiveDigitalMaxGainDb);
    apm_.setEchoEnhancedFilterEnabled(audioOptions_.echoEnhancedFilterEnabled);
}

void RealtimeTranscriptionSession::setResultCallback(ResultCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    resultCallback_ = std::move(callback);
}

void RealtimeTranscriptionSession::setStatusCallback(StatusCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    statusCallback_ = std::move(callback);
}

bool RealtimeTranscriptionSession::pushPcm16(const std::vector<std::int16_t>& samples, int sampleRate, int channels)
{
    if (samples.empty() || sampleRate <= 0 || channels <= 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
        return false;
    }

    std::vector<std::int16_t> mono = downmixToMono(samples, channels);
    processingBuffer_.insert(processingBuffer_.end(), mono.begin(), mono.end());

    const int frameSamples = std::max(1, sampleRate / 100);
    while (static_cast<int>(processingBuffer_.size()) >= frameSamples) {
        std::vector<std::int16_t> frame(processingBuffer_.begin(), processingBuffer_.begin() + frameSamples);
        processingBuffer_.erase(processingBuffer_.begin(), processingBuffer_.begin() + frameSamples);
        applyAudioProcessingFrame(&frame, sampleRate);
        std::vector<std::int16_t> frame16k = resampleTo16k(frame, sampleRate);
        auto readySegments = segmenter_.appendFrame(frame16k);
        for (auto& segment : readySegments) {
            enqueueSegment(std::move(segment));
        }
    }
    return true;
}

void RealtimeTranscriptionSession::emitStatus(const std::string& status) const
{
    StatusCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = statusCallback_;
    }
    if (callback) {
        callback(status);
    }
}

void RealtimeTranscriptionSession::enqueueSegment(SpeechSegment segment)
{
    pendingSegments_.push(PendingSegment{std::move(segment.samples), segment.startMs, segment.endMs});
    condition_.notify_one();
}

void RealtimeTranscriptionSession::runWorker()
{
    for (;;) {
        PendingSegment segment;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this]() {
                return workerStopRequested_ || !pendingSegments_.empty();
            });

            if (workerStopRequested_ && pendingSegments_.empty()) {
                break;
            }

            segment = std::move(pendingSegments_.front());
            pendingSegments_.pop();
        }

        emitStatus("正在识别");
        const std::string text = transcribeSegment(segment);
        if (text.empty()) {
            emitStatus("预览中");
            continue;
        }

        ResultCallback resultCallback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            resultCallback = resultCallback_;
        }
        if (resultCallback) {
            resultCallback(TranscriptionResult{text, segment.startMs, segment.endMs});
        }
        emitStatus("预览中");
    }
}

std::vector<std::int16_t> RealtimeTranscriptionSession::downmixToMono(const std::vector<std::int16_t>& samples, int channels) const
{
    if (channels == 1) {
        return samples;
    }

    const std::size_t frameCount = samples.size() / static_cast<std::size_t>(channels);
    std::vector<std::int16_t> mono(frameCount, 0);
    for (std::size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        int sum = 0;
        for (int channel = 0; channel < channels; ++channel) {
            sum += samples[frameIndex * static_cast<std::size_t>(channels) + static_cast<std::size_t>(channel)];
        }
        mono[frameIndex] = static_cast<std::int16_t>(sum / channels);
    }
    return mono;
}

std::vector<std::int16_t> RealtimeTranscriptionSession::resampleTo16k(const std::vector<std::int16_t>& monoSamples, int sampleRate) const
{
    if (monoSamples.empty()) {
        return {};
    }
    if (sampleRate == 16000) {
        return monoSamples;
    }

    const double ratio = 16000.0 / static_cast<double>(sampleRate);
    const std::size_t outputSize = static_cast<std::size_t>(std::ceil(monoSamples.size() * ratio));
    std::vector<std::int16_t> result(outputSize, 0);
    for (std::size_t i = 0; i < outputSize; ++i) {
        const double sourceIndex = static_cast<double>(i) / ratio;
        const std::size_t left = static_cast<std::size_t>(sourceIndex);
        const std::size_t right = std::min(left + 1, monoSamples.size() - 1);
        const double fraction = sourceIndex - static_cast<double>(left);
        const double sample = monoSamples[left] * (1.0 - fraction) + monoSamples[right] * fraction;
        result[i] = static_cast<std::int16_t>(sample);
    }
    return result;
}

void RealtimeTranscriptionSession::applyAudioProcessingFrame(std::vector<std::int16_t>* frame, int sampleRate)
{
    if (!frame || frame->empty() || !apmInitialized_) {
        return;
    }
    apm_.processFrame(frame->data(), static_cast<int>(frame->size()), sampleRate, 1);
}

bool RealtimeTranscriptionSession::loadModelInternal(const std::string& modelPath)
{
#if LINKS_ENABLE_SPEECH_TO_TEXT
    if (modelPath.empty()) {
        return false;
    }

    if (whisperContext_) {
        whisper_free(whisperContext_);
        whisperContext_ = nullptr;
    }

    whisper_context_params params = whisper_context_default_params();
    params.use_gpu = true;
    whisperContext_ = whisper_init_from_file_with_params(modelPath.c_str(), params);
    return whisperContext_ != nullptr;
#else
    (void)modelPath;
    return false;
#endif
}

std::string RealtimeTranscriptionSession::transcribeSegment(const PendingSegment& segment)
{
#if LINKS_ENABLE_SPEECH_TO_TEXT
    whisper_context* context = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        context = whisperContext_;
    }
    if (!context) {
        return std::string();
    }

    std::vector<float> pcmFloat(segment.samples.size(), 0.0f);
    for (std::size_t i = 0; i < segment.samples.size(); ++i) {
        pcmFloat[i] = static_cast<float>(segment.samples[i]) / 32768.0f;
    }

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
    params.print_progress = false;
    params.print_realtime = false;
    params.print_special = false;
    params.print_timestamps = false;
    params.no_timestamps = true;
    params.token_timestamps = false;
    params.translate = false;
    params.single_segment = false;
    params.no_context = false;
    params.n_threads = 4;
    params.language = "zh";
    params.detect_language = false;
    params.initial_prompt = "以下是普通话的简体中文语音转写结果。";
    params.carry_initial_prompt = false;
    params.suppress_blank = true;
    params.suppress_nst = true;
    params.temperature = 0.0f;
    params.temperature_inc = 0.0f;
    params.max_len = 96;
    params.max_tokens = 96;
    params.n_max_text_ctx = 224;
    params.beam_search.beam_size = 5;

    if (whisper_full(context, params, pcmFloat.data(), static_cast<int>(pcmFloat.size())) != 0) {
        return std::string();
    }

    std::string text;
    const int segmentCount = whisper_full_n_segments(context);
    for (int index = 0; index < segmentCount; ++index) {
        const char* segmentText = whisper_full_get_segment_text(context, index);
        if (segmentText) {
            text += segmentText;
        }
    }
    return text;
#else
    (void)segment;
    return std::string();
#endif
}

}  // namespace links::speech
