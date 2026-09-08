#include "media_pipeline.h"

#include <utility>

#include "../base/log.h"
#include "../base/strings.h"
#include "../media/audio_resampler.h"
#include "participant_store.h"
#include <cstdint>
#include <vector>

namespace core = links::core;

MediaPipeline::MediaPipeline(ParticipantStore* participantStore,
                             core::TaskRunner& taskRunner,
                             core::AudioPlayerFactory& audioPlayers)
    : participantStore_(participantStore),
      taskRunner_(taskRunner),
      audioPlayerFactory_(audioPlayers)
{
}

MediaPipeline::~MediaPipeline()
{
    if (!videoStreams_.empty() || !audioStreams_.empty()
        || !videoStreamThreads_.empty() || !audioStreamThreads_.empty()
        || !streamStopFlags_.empty() || !audioPlayers_.empty()) {
        stopAll();
    }
}

void MediaPipeline::setVideoStream(const std::string& trackSid,
                                   std::shared_ptr<livekit::VideoStream> stream)
{
    videoStreams_[trackSid] = std::move(stream);
}

void MediaPipeline::setAudioStream(const std::string& trackSid,
                                   std::shared_ptr<livekit::AudioStream> stream)
{
    audioStreams_[trackSid] = std::move(stream);
}

bool MediaPipeline::hasVideoStream(const std::string& trackSid) const
{
    return videoStreams_.find(trackSid) != videoStreams_.end();
}

bool MediaPipeline::hasAudioStream(const std::string& trackSid) const
{
    return audioStreams_.find(trackSid) != audioStreams_.end();
}

void MediaPipeline::removeVideoStream(const std::string& trackSid)
{
    videoStreams_.erase(trackSid);
}

void MediaPipeline::removeAudioStream(const std::string& trackSid)
{
    audioStreams_.erase(trackSid);
}

void MediaPipeline::setReverseAudioCallback(ReverseAudioCallback callback)
{
    reverseAudioCallback_ = std::move(callback);
}

void MediaPipeline::startVideoStreamReader(const std::string& trackSid,
                                           const std::string& participantIdentity,
                                           std::shared_ptr<livekit::VideoStream> stream)
{
    auto stopFlag = std::make_shared<std::atomic<bool>>(false);
    streamStopFlags_[trackSid] = stopFlag;

    std::thread readerThread([this, trackSid, participantIdentity, stream, stopFlag]() {
        livekit::VideoFrameEvent event;
        while (!stopFlag->load()) {
            if (!stream->read(event)) {
                break;
            }

            core::postGuarded(taskRunner_, lifetime_,
                [this, event = std::move(event), trackSid, participantIdentity]() mutable {
                    handleVideoFrame(event, trackSid, participantIdentity);
                });
        }
    });

    videoStreamThreads_[trackSid] = std::make_unique<std::thread>(std::move(readerThread));
}

void MediaPipeline::startAudioStreamReader(const std::string& trackSid,
                                           const std::string& participantIdentity,
                                           std::shared_ptr<livekit::AudioStream> stream)
{
    auto stopFlag = std::make_shared<std::atomic<bool>>(false);
    streamStopFlags_[trackSid] = stopFlag;

    std::thread readerThread([this, trackSid, participantIdentity, stream, stopFlag]() {
        livekit::AudioFrameEvent event;
        while (!stopFlag->load()) {
            if (!stream->read(event)) {
                break;
            }

            core::postGuarded(taskRunner_, lifetime_,
                [this, event = std::move(event), trackSid, participantIdentity]() mutable {
                    handleAudioFrame(event, trackSid, participantIdentity);
                });
        }
    });

    audioStreamThreads_[trackSid] = std::make_unique<std::thread>(std::move(readerThread));
}

void MediaPipeline::stopTrack(const std::string& trackSid)
{
    stopStreamReaders(trackSid);

    videoStreams_.erase(trackSid);
    audioStreams_.erase(trackSid);

    const auto player = audioPlayers_.find(trackSid);
    if (player != audioPlayers_.end()) {
        if (player->second.player) {
            player->second.player->close();
        }
        audioPlayers_.erase(player);
    }
}

void MediaPipeline::stopAll()
{
    for (auto& entry : streamStopFlags_) {
        if (entry.second) {
            entry.second->store(true);
        }
    }

    for (auto& entry : videoStreamThreads_) {
        if (entry.second && entry.second->joinable()) {
            entry.second->join();
        }
    }
    videoStreamThreads_.clear();

    for (auto& entry : audioStreamThreads_) {
        if (entry.second && entry.second->joinable()) {
            entry.second->join();
        }
    }
    audioStreamThreads_.clear();

    streamStopFlags_.clear();

    core::logInfo("Cleaning up video streams");
    videoStreams_.clear();

    core::logInfo("Cleaning up audio streams");
    audioStreams_.clear();

    for (auto& entry : audioPlayers_) {
        if (entry.second.player) {
            entry.second.player->close();
        }
    }
    audioPlayers_.clear();
}

void MediaPipeline::handleVideoFrame(const livekit::VideoFrameEvent& event,
                                     const std::string& trackSid,
                                     const std::string& participantIdentity)
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

    // One deep copy here, as before (QImage wrap + .copy()); every later hop is
    // a shared_ptr bump.
    const core::VideoFrame image = core::VideoFrame::copyFrom(
        frame.data(), frame.width(), frame.height(),
        frame.width() * 4, core::PixelFormat::RGBA8888);

    videoFrameReady.notify(participantIdentity, trackSid, image, source);
}

void MediaPipeline::handleAudioFrame(const livekit::AudioFrameEvent& event,
                                     const std::string& trackSid,
                                     const std::string& participantIdentity)
{
    const auto& frame = event.frame;

    audioActivity.notify(participantIdentity, true);

    AudioPlayback& playback = audioPlayers_[trackSid];
    if (!playback.player) {
        playback.player = audioPlayerFactory_.createPlayer();
    }
    if (!playback.player) {
        core::logWarning("Audio output device unavailable");
        return;
    }

    const std::string deviceId = audioPlayerFactory_.defaultDeviceId();
    const core::AudioFormat desiredFormat =
        playback.player->negotiate(frame.sampleRate(), frame.numChannels());

    // Reopen when the default device changed or the negotiated format moved --
    // the same condition the QAudioDevice/QAudioFormat comparison expressed.
    const bool needReopen = !playback.player->isOpen()
        || playback.deviceId != deviceId
        || playback.format != desiredFormat;

    if (needReopen) {
        playback.player->close();

        if (desiredFormat.sampleRate != frame.sampleRate()
            || desiredFormat.channels != frame.numChannels()
            || desiredFormat.sampleFormat != core::SampleFormat::Int16) {
            core::logWarning("Audio format not supported by output device, using preferred format");
        }

        playback.deviceId = deviceId;
        playback.format = desiredFormat;
        playback.player->open(desiredFormat);
    }

    if (!playback.player->isOpen()) {
        core::logWarning("Audio output device unavailable");
        return;
    }

    const auto& samples = frame.data();
    const std::vector<float> floatPcm =
        core::mixAndResampleToFloat(samples,
                                    frame.sampleRate(),
                                    frame.numChannels(),
                                    playback.format.sampleRate,
                                    playback.format.channels);
    const std::vector<std::uint8_t> data = core::packFloatPcm(floatPcm, playback.format);
    if (data.empty()) {
        core::logWarning("Failed to convert remote audio frame to playback format");
        return;
    }

    // Feed far-end audio to the AEC so it can learn the echo path.
    // This is essential for echo cancellation to work correctly.
    //
    // Order matters and must not be changed: the AEC is fed the ORIGINAL,
    // un-resampled samples at the source rate, and it is fed BEFORE the write.
    if (reverseAudioCallback_ && !samples.empty()) {
        int numSamples = static_cast<int>(samples.size()) / frame.numChannels();
        reverseAudioCallback_(samples.data(), numSamples,
                              frame.sampleRate(), frame.numChannels());
    }

    playback.player->write(data.data(), data.size());
}

void MediaPipeline::stopStreamReaders(const std::string& trackSid)
{
    const auto flag = streamStopFlags_.find(trackSid);
    if (flag != streamStopFlags_.end() && flag->second) {
        flag->second->store(true);
    }

    const auto videoThread = videoStreamThreads_.find(trackSid);
    if (videoThread != videoStreamThreads_.end()) {
        if (videoThread->second && videoThread->second->joinable()) {
            videoThread->second->join();
        }
        videoStreamThreads_.erase(videoThread);
    }

    const auto audioThread = audioStreamThreads_.find(trackSid);
    if (audioThread != audioStreamThreads_.end()) {
        if (audioThread->second && audioThread->second->joinable()) {
            audioThread->second->join();
        }
        audioStreamThreads_.erase(audioThread);
    }

    streamStopFlags_.erase(trackSid);
}
