#ifndef CORE_CONFERENCE_CONFERENCE_MANAGER_H
#define CORE_CONFERENCE_CONFERENCE_MANAGER_H

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../audio_processing_module.h"
#include "../base/executor.h"
#include "../base/signal.h"
#include "../base/timer.h"
#include "../media/video_frame.h"
#include "../platform_services.h"
#include "../screen_capturer.h"
#include "conference_types.h"
#include "device_config.h"
#include "device_controller.h"
#include "media_pipeline.h"
#include "network_stats_aggregator.h"
#include "participant_store.h"
#include "room_controller.h"
#include "livekit/livekit.h"

class RoomEventDelegate;

class ConferenceManager {
public:
    ConferenceManager(const links::core::PlatformServices& services,
                      const links::core::DeviceSelection& devices,
                      const links::core::AudioProcessingConfig& audio);
    ~ConferenceManager();

    // Connection
    void connectToRoom(const std::string& url, const std::string& token);
    void disconnectFromRoom();
    bool isConnected() const { return connected_; }
    livekit::DisconnectReason lastDisconnectReason() const { return lastDisconnectReason_; }

    // Media controls
    void toggleMicrophone();
    void toggleCamera();
    void toggleScreenShare();
    void setScreenShareMode(ScreenCapturer::Mode mode,
                            links::core::MonitorId monitorId,
                            links::core::WindowId windowId);

    // Device switching (while conference is active)
    void switchCamera(const std::string& deviceId);
    void switchMicrophone(const std::string& deviceId);

    bool isMicrophoneEnabled() const;
    bool isCameraEnabled() const;
    bool isScreenSharing() const;

    // Audio processing settings (runtime-applicable during conference)
    void applyAudioSettings(const links::core::AudioProcessingConfig& config);
    void setEchoCancellationEnabled(bool enabled);
    void setNoiseSuppressionEnabled(bool enabled);
    void setAutoGainControlEnabled(bool enabled);
    void setHighPassFilterEnabled(bool enabled);
    void setNoiseSuppressionLevel(int level);  // 0=Low,1=Moderate,2=High,3=VeryHigh
    void setGainControlMode(int mode);         // 0=AdaptiveDigital,1=FixedDigital
    void setFixedDigitalGainDb(float gainDb);
    void setAdaptiveDigitalMaxGainDb(float maxGainDb);
    void setEchoEnhancedFilterEnabled(bool enabled);

    /**
     * Feed far-end audio to the AEC. Called by MediaPipeline.
     */
    void feedReverseAudio(const int16_t* data, int samples, int sampleRate, int channels);

    // Chat
    void sendChatMessage(const std::string& message);

    // Participants
    std::vector<ParticipantInfo> getParticipants() const;
    int getParticipantCount() const;
    void reconcileParticipants();

    // Room info
    std::string getRoomName() const { return roomName_; }
    std::string getLocalParticipantName() const { return participantName_; }
    std::string getLocalParticipantIdentity() const { return participantIdentity_; }

    // -------------------------------------------------------------------
    // Events. All notified on the main thread -- RoomEventDelegate and
    // MediaPipeline have already hopped through the TaskRunner by this point,
    // which is the guarantee Qt::QueuedConnection used to provide.
    // -------------------------------------------------------------------
    template <typename... A> using Signal = links::core::Signal<A...>;

    Signal<> connected;
    Signal<> disconnected;
    Signal<int> roomDisconnected;
    Signal<livekit::ConnectionState> connectionStateChanged;

    Signal<const ParticipantInfo&> participantJoined;
    Signal<const std::string&> participantLeft;

    // Still emitted by core, but nothing in ui/ subscribes to these two --
    // ConferenceBackend never connected the Qt signals they replace either.
    Signal<const ParticipantInfo&> participantUpdated;
    Signal<const std::string&> connectionError;

    Signal<const TrackInfo&> trackSubscribed;
    Signal<const std::string&, const std::string&> trackUnsubscribed;
    Signal<const std::string&, const std::string&,
           livekit::TrackKind, livekit::TrackSource> trackUnpublished;
    Signal<const std::string&, const std::string&, livekit::TrackKind, bool> trackMutedStateChanged;

    Signal<bool> localMicrophoneChanged;
    Signal<bool> localCameraChanged;
    Signal<bool> localScreenShareChanged;
    Signal<const links::core::VideoFrame&> localScreenFrameReady;
    Signal<const links::core::VideoFrame&> localVideoFrameReady;
    Signal<const std::string&, const std::string&,
           const links::core::VideoFrame&, livekit::TrackSource> videoFrameReceived;

    Signal<const ChatMessage&> chatMessageReceived;
    Signal<int> localConnectionQualityChanged;
    Signal<const NetworkStatsSnapshot&> localNetworkStatsUpdated;

    /// Forwarded from DeviceController so ui/ can persist the correction.
    Signal<const std::string&> preferredCameraChanged;
    Signal<const std::string&> preferredMicrophoneChanged;

private:
    // Handlers for RoomEventDelegate events (already on the main thread).
    void onParticipantConnected(std::string identity, std::string sid, std::string name, bool isHost);
    void onParticipantDisconnected(std::string identity, int reason);
    void onTrackSubscribed(std::string trackSid, std::string participantIdentity,
                           int kind, int source, bool muted,
                           std::shared_ptr<livekit::Track> track,
                           std::shared_ptr<livekit::RemoteTrackPublication> publication);
    void onTrackUnsubscribed(std::string trackSid, std::string participantIdentity);
    void onTrackMuted(std::string trackSid, std::string participantIdentity, int kind);
    void onTrackUnmuted(std::string trackSid, std::string participantIdentity, int kind);
    void onTrackUnpublished(std::string trackSid, std::string participantIdentity, int kind, int source);
    void onConnectionQualityChanged(std::string participantIdentity, int quality);
    void onConnectionStateChanged(int state);
    void onRoomDisconnected(int reason);
    void onDataReceived(std::vector<std::uint8_t> data, std::string participantIdentity, std::string topic);
    void onLocalTrackPublished(std::string publicationSid, int kind, int source);

    void updateParticipantInfo(const std::string& identity);
    void reconcileParticipantsInternal(const char* source);
    void markConnected(const char* source, bool emitStateSignal);
    void pollLocalNetworkStats();
    void applyNetworkPollResult(AsyncNetworkPollResult result, std::uint64_t pollSeq);
    std::string resolveLocalParticipantIdentity() const;
    std::vector<std::shared_ptr<livekit::Track>> collectTrackStatsSources() const;
    NetworkStatsSnapshot buildEstimatedNetworkSnapshot(NetworkQualityLevel quality,
                                                       std::int64_t nowMs) const;
    void resetNetworkMetrics();

    const links::core::PlatformServices& services_;

    std::unique_ptr<RoomController> roomController_;
    std::unique_ptr<RoomEventDelegate> roomDelegate_;
    std::unique_ptr<ParticipantStore> participantStore_;
    std::unique_ptr<MediaPipeline> mediaPipeline_;
    std::unique_ptr<DeviceController> deviceController_;

    std::string roomName_;
    std::string participantName_;
    std::string participantIdentity_;
    bool connected_{false};
    bool disconnecting_{false};
    livekit::DisconnectReason lastDisconnectReason_{livekit::DisconnectReason::Unknown};
    std::unique_ptr<links::core::Timer> networkStatsTimer_;
    NetworkQualityLevel localNetworkQuality_{NetworkQualityLevel::Unknown};
    NetworkStatsSnapshot localNetworkStats_;
    NetworkByteCounters previousNetworkByteCounters_;
    bool usingEstimatedNetworkStats_{false};
    bool networkStatsPollInFlight_{false};
    std::uint64_t networkStatsPollSeq_{0};
    std::set<std::string> lastPolledTrackSids_;

    // Subscriptions to the collaborators; cleared first in the destructor.
    links::core::ConnectionBag collaboratorConnections_;

    // Must stay last: cancels background-poll results still in flight.
    links::core::LifetimeToken lifetime_;
};

#endif // CORE_CONFERENCE_CONFERENCE_MANAGER_H
