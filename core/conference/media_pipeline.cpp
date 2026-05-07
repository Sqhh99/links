#include "media_pipeline.h"
#include "participant_store.h"
#include "../../utils/logger.h"
#include <QByteArray>
#include <QMetaObject>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>

namespace {

float clampSample(float value)
{
    return std::max(-1.0f, std::min(1.0f, value));
}

std::vector<float> mixAndResampleToFloat(const std::vector<int16_t>& input,
                                         int srcRate,
                                         int srcChannels,
                                         int dstRate,
                                         int dstChannels)
{
    if (input.empty() || srcRate <= 0 || dstRate <= 0 || srcChannels <= 0 || dstChannels <= 0) {
        return {};
    }

    const int srcFrames = static_cast<int>(input.size()) / srcChannels;
    if (srcFrames <= 0) {
        return {};
    }

    const double ratio = static_cast<double>(dstRate) / static_cast<double>(srcRate);
    const int dstFrames = std::max(1, static_cast<int>(std::llround(srcFrames * ratio)));
    std::vector<float> output(static_cast<size_t>(dstFrames * dstChannels), 0.0f);

    for (int dstFrame = 0; dstFrame < dstFrames; ++dstFrame) {
        const double srcPos = static_cast<double>(dstFrame) / ratio;
        const int srcIndex = std::min(srcFrames - 1, std::max(0, static_cast<int>(std::llround(srcPos))));

        float left = 0.0f;
        float right = 0.0f;
        if (srcChannels == 1) {
            left = right = static_cast<float>(input[srcIndex]) / 32768.0f;
        } else {
            const int base = srcIndex * srcChannels;
            left = static_cast<float>(input[base]) / 32768.0f;
            right = static_cast<float>(input[base + 1]) / 32768.0f;
        }

        for (int dstChannel = 0; dstChannel < dstChannels; ++dstChannel) {
            float sample = 0.0f;
            if (dstChannels == 1) {
                sample = (left + right) * 0.5f;
            } else {
                sample = (dstChannel % 2 == 0) ? left : right;
            }
            output[static_cast<size_t>(dstFrame * dstChannels + dstChannel)] = clampSample(sample);
        }
    }

    return output;
}

QByteArray convertFloatPcmToOutputBytes(const std::vector<float>& input, const QAudioFormat& format)
{
    if (input.empty()) {
        return {};
    }

    QByteArray output;
    const QAudioFormat::SampleFormat sampleFormat = format.sampleFormat();
    const int bytesPerSample = format.bytesPerSample();
    if (bytesPerSample <= 0) {
        return {};
    }

    output.resize(static_cast<int>(input.size() * static_cast<size_t>(bytesPerSample)));
    char* dst = output.data();

    for (size_t i = 0; i < input.size(); ++i) {
        const float sample = clampSample(input[i]);
        const size_t offset = i * static_cast<size_t>(bytesPerSample);
        switch (sampleFormat) {
        case QAudioFormat::UInt8: {
            const uint8_t value = static_cast<uint8_t>(std::lround((sample * 0.5f + 0.5f) * 255.0f));
            std::memcpy(dst + offset, &value, sizeof(value));
            break;
        }
        case QAudioFormat::Int16: {
            const int16_t value = static_cast<int16_t>(std::lround(sample * 32767.0f));
            std::memcpy(dst + offset, &value, sizeof(value));
            break;
        }
        case QAudioFormat::Int32: {
            const int32_t value = static_cast<int32_t>(std::lround(sample * 2147483647.0f));
            std::memcpy(dst + offset, &value, sizeof(value));
            break;
        }
        case QAudioFormat::Float: {
            std::memcpy(dst + offset, &sample, sizeof(sample));
            break;
        }
        default:
            return {};
        }
    }

    return output;
}

QAudioFormat choosePlaybackFormat(const QAudioDevice& device, int sampleRate, int channels)
{
    QAudioFormat requested;
    requested.setSampleRate(sampleRate);
    requested.setChannelCount(channels);
    requested.setSampleFormat(QAudioFormat::Int16);
    if (device.isFormatSupported(requested)) {
        return requested;
    }

    QAudioFormat fallback = device.preferredFormat();
    if (!device.isFormatSupported(fallback)) {
        fallback = requested;
    }
    return fallback;
}

} // namespace

MediaPipeline::MediaPipeline(ParticipantStore* participantStore, QObject* parent)
    : QObject(parent),
      participantStore_(participantStore)
{
}

MediaPipeline::~MediaPipeline()
{
    if (!videoStreams_.isEmpty() || !audioStreams_.isEmpty()
        || !videoStreamThreads_.empty() || !audioStreamThreads_.empty()
        || !streamStopFlags_.isEmpty() || !audioPlayers_.isEmpty()) {
        stopAll();
    }
}

void MediaPipeline::setVideoStream(const QString& trackSid,
                                   std::shared_ptr<livekit::VideoStream> stream)
{
    videoStreams_[trackSid] = std::move(stream);
}

void MediaPipeline::setAudioStream(const QString& trackSid,
                                   std::shared_ptr<livekit::AudioStream> stream)
{
    audioStreams_[trackSid] = std::move(stream);
}

bool MediaPipeline::hasVideoStream(const QString& trackSid) const
{
    return videoStreams_.contains(trackSid);
}

bool MediaPipeline::hasAudioStream(const QString& trackSid) const
{
    return audioStreams_.contains(trackSid);
}

void MediaPipeline::removeVideoStream(const QString& trackSid)
{
    videoStreams_.remove(trackSid);
}

void MediaPipeline::removeAudioStream(const QString& trackSid)
{
    audioStreams_.remove(trackSid);
}

void MediaPipeline::setReverseAudioCallback(ReverseAudioCallback callback)
{
    reverseAudioCallback_ = std::move(callback);
}

void MediaPipeline::startVideoStreamReader(const QString& trackSid,
                                           const QString& participantIdentity,
                                           std::shared_ptr<livekit::VideoStream> stream)
{
    auto* stopFlag = new std::atomic<bool>(false);
    streamStopFlags_[trackSid] = stopFlag;

    std::thread readerThread([this, trackSid, participantIdentity, stream, stopFlag]() {
        livekit::VideoFrameEvent event;
        while (!stopFlag->load()) {
            if (!stream->read(event)) {
                break;
            }

            QMetaObject::invokeMethod(this, [this, event = std::move(event), trackSid, participantIdentity]() mutable {
                handleVideoFrame(event, trackSid, participantIdentity);
            }, Qt::QueuedConnection);
        }
    });

    videoStreamThreads_[trackSid] = std::make_unique<std::thread>(std::move(readerThread));
}

void MediaPipeline::startAudioStreamReader(const QString& trackSid,
                                           const QString& participantIdentity,
                                           std::shared_ptr<livekit::AudioStream> stream)
{
    auto* stopFlag = new std::atomic<bool>(false);
    streamStopFlags_[trackSid] = stopFlag;

    std::thread readerThread([this, trackSid, participantIdentity, stream, stopFlag]() {
        livekit::AudioFrameEvent event;
        while (!stopFlag->load()) {
            if (!stream->read(event)) {
                break;
            }

            QMetaObject::invokeMethod(this, [this, event = std::move(event), trackSid, participantIdentity]() mutable {
                handleAudioFrame(event, trackSid, participantIdentity);
            }, Qt::QueuedConnection);
        }
    });

    audioStreamThreads_[trackSid] = std::make_unique<std::thread>(std::move(readerThread));
}

void MediaPipeline::stopTrack(const QString& trackSid)
{
    stopStreamReaders(trackSid);

    if (videoStreams_.contains(trackSid)) {
        videoStreams_.remove(trackSid);
    }
    if (audioStreams_.contains(trackSid)) {
        audioStreams_.remove(trackSid);
    }
    if (audioPlayers_.contains(trackSid)) {
        auto player = audioPlayers_.take(trackSid);
        if (player.sink) {
            player.sink->stop();
        }
    }
}

void MediaPipeline::stopAll()
{
    for (auto it = streamStopFlags_.begin(); it != streamStopFlags_.end(); ++it) {
        if (it.value()) {
            it.value()->store(true);
        }
    }

    for (auto& [trackSid, threadPtr] : videoStreamThreads_) {
        if (threadPtr && threadPtr->joinable()) {
            threadPtr->join();
        }
    }
    videoStreamThreads_.clear();

    for (auto& [trackSid, threadPtr] : audioStreamThreads_) {
        if (threadPtr && threadPtr->joinable()) {
            threadPtr->join();
        }
    }
    audioStreamThreads_.clear();

    for (auto it = streamStopFlags_.begin(); it != streamStopFlags_.end(); ++it) {
        delete it.value();
    }
    streamStopFlags_.clear();

    Logger::instance().info("Cleaning up video streams");
    videoStreams_.clear();

    Logger::instance().info("Cleaning up audio streams");
    audioStreams_.clear();

    for (auto& player : audioPlayers_) {
        if (player.sink) {
            player.sink->stop();
        }
    }
    audioPlayers_.clear();
}

void MediaPipeline::handleVideoFrame(const livekit::VideoFrameEvent& event,
                                     const QString& trackSid,
                                     const QString& participantIdentity)
{
    const auto& frame = event.frame;
    if (frame.width() == 0 || frame.height() == 0) {
        return;
    }

    livekit::TrackSource source = livekit::TrackSource::SOURCE_UNKNOWN;
    if (participantStore_ && participantStore_->hasTrackSource(trackSid)) {
        source = participantStore_->trackSource(trackSid);
    }
    if (source == livekit::TrackSource::SOURCE_SCREENSHARE && participantStore_) {
        participantStore_->setScreenShareActive(participantIdentity, true);
    }

    QImage image(frame.data(), frame.width(), frame.height(), QImage::Format_RGBA8888);
    QImage imageCopy = image.copy();

    emit videoFrameReady(participantIdentity, trackSid, imageCopy, source);
}

void MediaPipeline::handleAudioFrame(const livekit::AudioFrameEvent& event,
                                     const QString& trackSid,
                                     const QString& participantIdentity)
{
    const auto& frame = event.frame;

    emit audioActivity(participantIdentity, true);

    auto playbackIt = audioPlayers_.find(trackSid);
    if (playbackIt == audioPlayers_.end()) {
        playbackIt = audioPlayers_.insert(trackSid, AudioPlayback{});
    }

    AudioPlayback& playback = playbackIt.value();

    bool needRecreate = !playback.sink
        || playback.format.sampleRate() != frame.sample_rate()
        || playback.format.channelCount() != frame.num_channels();

    if (needRecreate) {
        if (playback.sink) {
            playback.sink->stop();
        }

        QAudioDevice device = QMediaDevices::defaultAudioOutput();
        QAudioFormat format = choosePlaybackFormat(device, frame.sample_rate(), frame.num_channels());
        if (format.sampleRate() != frame.sample_rate()
            || format.channelCount() != frame.num_channels()
            || format.sampleFormat() != QAudioFormat::Int16) {
            Logger::instance().warning("Audio format not supported by output device, using preferred format");
        }

        playback.format = format;
        playback.sink = QSharedPointer<QAudioSink>::create(device, format);
        playback.device = playback.sink ? playback.sink->start() : nullptr;
    }

    if (!playback.device) {
        Logger::instance().warning("Audio output device unavailable");
        return;
    }

    const auto& samples = frame.data();
    const std::vector<float> floatPcm =
        mixAndResampleToFloat(samples,
                              frame.sample_rate(),
                              frame.num_channels(),
                              playback.format.sampleRate(),
                              playback.format.channelCount());
    const QByteArray data = convertFloatPcmToOutputBytes(floatPcm, playback.format);
    if (data.isEmpty()) {
        Logger::instance().warning("Failed to convert remote audio frame to playback format");
        return;
    }
    
    // Feed far-end audio to the AEC so it can learn the echo path.
    // This is essential for echo cancellation to work correctly.
    if (reverseAudioCallback_ && !samples.empty()) {
        int numSamples = static_cast<int>(samples.size()) / frame.num_channels();
        reverseAudioCallback_(samples.data(), numSamples,
                              frame.sample_rate(), frame.num_channels());
    }
    
    playback.device->write(data);
}

void MediaPipeline::stopStreamReaders(const QString& trackSid)
{
    if (streamStopFlags_.contains(trackSid)) {
        streamStopFlags_[trackSid]->store(true);
    }

    if (videoStreamThreads_.count(trackSid) > 0) {
        auto& threadPtr = videoStreamThreads_[trackSid];
        if (threadPtr && threadPtr->joinable()) {
            threadPtr->join();
        }
        videoStreamThreads_.erase(trackSid);
    }

    if (audioStreamThreads_.count(trackSid) > 0) {
        auto& threadPtr = audioStreamThreads_[trackSid];
        if (threadPtr && threadPtr->joinable()) {
            threadPtr->join();
        }
        audioStreamThreads_.erase(trackSid);
    }

    if (streamStopFlags_.contains(trackSid)) {
        delete streamStopFlags_[trackSid];
        streamStopFlags_.remove(trackSid);
    }
}
