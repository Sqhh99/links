#ifndef LINKS_CORE_SPEECH_REALTIME_TRANSCRIPTION_SESSION_H_
#define LINKS_CORE_SPEECH_REALTIME_TRANSCRIPTION_SESSION_H_

#include "speech_types.h"
#include "utterance_segmenter.h"
#include "../audio_processing_module.h"

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

struct whisper_context;

namespace links::speech {

class RealtimeTranscriptionSession {
public:
    using ResultCallback = std::function<void(const TranscriptionResult&)>;
    using StatusCallback = std::function<void(const std::string&)>;

    RealtimeTranscriptionSession();
    ~RealtimeTranscriptionSession();

    RealtimeTranscriptionSession(const RealtimeTranscriptionSession&) = delete;
    RealtimeTranscriptionSession& operator=(const RealtimeTranscriptionSession&) = delete;

    bool isAvailable() const;
    bool loadModel(const std::string& modelPath);
    bool start();
    void stop();
    void reset();

    void setAudioProcessingOptions(const AudioProcessingOptions& options);
    void setResultCallback(ResultCallback callback);
    void setStatusCallback(StatusCallback callback);

    bool pushPcm16(const std::vector<std::int16_t>& samples, int sampleRate, int channels);

private:
    struct PendingSegment {
        std::vector<std::int16_t> samples;
        std::int64_t startMs{0};
        std::int64_t endMs{0};
    };

    void emitStatus(const std::string& status) const;
    void enqueueSegment(SpeechSegment segment);
    void runWorker();
    std::vector<std::int16_t> downmixToMono(const std::vector<std::int16_t>& samples, int channels) const;
    std::vector<std::int16_t> resampleTo16k(const std::vector<std::int16_t>& monoSamples, int sampleRate) const;
    void applyAudioProcessingFrame(std::vector<std::int16_t>* frame, int sampleRate);
    bool loadModelInternal(const std::string& modelPath);
    std::string transcribeSegment(const PendingSegment& segment);

    AudioProcessingModule apm_;
    AudioProcessingOptions audioOptions_;
    bool apmInitialized_{false};
    bool modelLoaded_{false};
    bool running_{false};
    std::string modelPath_;
    whisper_context* whisperContext_{nullptr};

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<PendingSegment> pendingSegments_;
    std::thread workerThread_;
    bool workerStopRequested_{false};

    ResultCallback resultCallback_;
    StatusCallback statusCallback_;

    std::vector<std::int16_t> processingBuffer_;
    UtteranceSegmenter segmenter_;
};

}  // namespace links::speech

#endif  // LINKS_CORE_SPEECH_REALTIME_TRANSCRIPTION_SESSION_H_
