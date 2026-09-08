#ifndef CORE_CONFERENCE_MEDIA_PIPELINE_H
#define CORE_CONFERENCE_MEDIA_PIPELINE_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include "../base/executor.h"
#include "../base/signal.h"
#include "../media/audio_player.h"
#include "../media/video_frame.h"
#include "livekit/livekit.h"

class ParticipantStore;

/**
 * Callback type for feeding far-end audio to the AEC.
 * Parameters: data, samples, sampleRate, channels
 */
using ReverseAudioCallback = std::function<void(const int16_t*, int, int, int)>;

class MediaPipeline {
public:
    MediaPipeline(ParticipantStore* participantStore,
                  links::core::TaskRunner& taskRunner,
                  links::core::AudioPlayerFactory& audioPlayers);
    ~MediaPipeline();

    void startVideoStreamReader(const std::string& trackSid,
                                const std::string& participantIdentity,
                                std::shared_ptr<livekit::VideoStream> stream);
    void startAudioStreamReader(const std::string& trackSid,
                                const std::string& participantIdentity,
                                std::shared_ptr<livekit::AudioStream> stream);
    void stopTrack(const std::string& trackSid);
    void stopAll();

    void setVideoStream(const std::string& trackSid, std::shared_ptr<livekit::VideoStream> stream);
    void setAudioStream(const std::string& trackSid, std::shared_ptr<livekit::AudioStream> stream);
    bool hasVideoStream(const std::string& trackSid) const;
    bool hasAudioStream(const std::string& trackSid) const;
    void removeVideoStream(const std::string& trackSid);
    void removeAudioStream(const std::string& trackSid);

    /**
     * Set a callback that will be invoked with every remote audio frame
     * so the AEC module can use it as a reference signal.
     */
    void setReverseAudioCallback(ReverseAudioCallback callback);

    // Notified on the main thread.
    links::core::Signal<const std::string&, const std::string&,
                        const links::core::VideoFrame&, livekit::TrackSource> videoFrameReady;
    links::core::Signal<const std::string&, bool> audioActivity;

private:
    struct AudioPlayback {
        std::unique_ptr<links::core::AudioPlayer> player;
        links::core::AudioFormat format;
        std::string deviceId;
    };

    void handleVideoFrame(const livekit::VideoFrameEvent& event,
                          const std::string& trackSid,
                          const std::string& participantIdentity);
    void handleAudioFrame(const livekit::AudioFrameEvent& event,
                          const std::string& trackSid,
                          const std::string& participantIdentity);
    void stopStreamReaders(const std::string& trackSid);

    ParticipantStore* participantStore_;
    links::core::TaskRunner& taskRunner_;
    links::core::AudioPlayerFactory& audioPlayerFactory_;

    std::map<std::string, std::shared_ptr<livekit::VideoStream>> videoStreams_;
    std::map<std::string, std::shared_ptr<livekit::AudioStream>> audioStreams_;
    std::map<std::string, std::unique_ptr<std::thread>> videoStreamThreads_;
    std::map<std::string, std::unique_ptr<std::thread>> audioStreamThreads_;

    // shared_ptr, not a raw owning pointer: the reader thread holds a strong
    // reference, so the flag stays alive even if the map entry is erased while
    // the thread is still winding down.
    std::map<std::string, std::shared_ptr<std::atomic<bool>>> streamStopFlags_;

    std::map<std::string, AudioPlayback> audioPlayers_;
    ReverseAudioCallback reverseAudioCallback_;

    // Must stay last: cancels in-flight posts from reader threads first.
    links::core::LifetimeToken lifetime_;
};

#endif // CORE_CONFERENCE_MEDIA_PIPELINE_H
