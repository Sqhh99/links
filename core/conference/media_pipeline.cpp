#include "media_pipeline.h"
#include "participant_store.h"
#include "../../utils/logger.h"
#include <QByteArray>
#include <QMetaObject>
#include <cstdint>

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

        QAudioFormat format;
        format.setSampleRate(frame.sample_rate());
        format.setChannelCount(frame.num_channels());
        format.setSampleFormat(QAudioFormat::Int16);

        QAudioDevice device = QMediaDevices::defaultAudioOutput();
        if (!device.isFormatSupported(format)) {
            Logger::instance().warning("Audio format not supported by output device, using preferred format");
            format = device.preferredFormat();
            format.setSampleFormat(QAudioFormat::Int16);
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
    QByteArray data(reinterpret_cast<const char*>(samples.data()),
                    static_cast<int>(samples.size() * sizeof(int16_t)));
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
