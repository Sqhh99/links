#include "conference_manager.h"
#include "participant_metadata_parser.h"
#include "../room_event_delegate.h"
#include <nlohmann/json.hpp>

#include "../base/log.h"
#include "../base/strings.h"
#include "../base/time.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>
#include "livekit/audio_stream.h"
#include "livekit/local_participant.h"
#include "livekit/remote_participant.h"
#include "livekit/track.h"
#include "livekit/video_stream.h"

namespace core = links::core;

namespace {

bool networkStatsEquivalent(const NetworkStatsSnapshot& lhs,
                            const NetworkStatsSnapshot& rhs)
{
    return lhs.rttMs == rhs.rttMs
        && lhs.jitterMs == rhs.jitterMs
        && lhs.uplinkKbps == rhs.uplinkKbps
        && lhs.downlinkKbps == rhs.downlinkKbps
        && std::fabs(lhs.packetLossPercent - rhs.packetLossPercent) < 0.001
        && lhs.videoWidth == rhs.videoWidth
        && lhs.videoHeight == rhs.videoHeight
        && std::fabs(lhs.videoFps - rhs.videoFps) < 0.1
        && lhs.audioCodec == rhs.audioCodec
        && lhs.videoCodec == rhs.videoCodec
        && lhs.availableSendBandwidthKbps == rhs.availableSendBandwidthKbps
        && lhs.transportProtocol == rhs.transportProtocol;
}

} // namespace

ConferenceManager::ConferenceManager(const core::PlatformServices& services,
                                     const core::DeviceSelection& devices,
                                     const core::AudioProcessingConfig& audio)
    : services_(services),
      roomController_(std::make_unique<RoomController>()),
      roomDelegate_(std::make_unique<RoomEventDelegate>(*services.taskRunner)),
      participantStore_(std::make_unique<ParticipantStore>()),
      mediaPipeline_(std::make_unique<MediaPipeline>(participantStore_.get(),
                                                     *services.taskRunner,
                                                     *services.audioPlayers)),
      deviceController_(std::make_unique<DeviceController>(roomController_->room(),
                                                           services, devices, audio))
{
    core::logInfo("ConferenceManager created");

    roomController_->setDelegate(roomDelegate_.get());

    auto& bag = collaboratorConnections_;
    auto& delegate = *roomDelegate_;

    bag += delegate.participantConnected.connect(
        [this](std::string identity, std::string sid, std::string name, bool isHost) {
            onParticipantConnected(std::move(identity), std::move(sid), std::move(name), isHost);
        });
    bag += delegate.participantDisconnected.connect(
        [this](std::string identity, int reason) {
            onParticipantDisconnected(std::move(identity), reason);
        });
    bag += delegate.trackSubscribed.connect(
        [this](std::string trackSid, std::string identity, int kind, int source, bool muted,
               std::shared_ptr<livekit::Track> track,
               std::shared_ptr<livekit::RemoteTrackPublication> publication) {
            onTrackSubscribed(std::move(trackSid), std::move(identity), kind, source, muted,
                              std::move(track), std::move(publication));
        });
    bag += delegate.trackUnsubscribed.connect(
        [this](std::string trackSid, std::string identity) {
            onTrackUnsubscribed(std::move(trackSid), std::move(identity));
        });
    bag += delegate.trackMuted.connect(
        [this](std::string trackSid, std::string identity, int kind) {
            onTrackMuted(std::move(trackSid), std::move(identity), kind);
        });
    bag += delegate.trackUnmuted.connect(
        [this](std::string trackSid, std::string identity, int kind) {
            onTrackUnmuted(std::move(trackSid), std::move(identity), kind);
        });
    bag += delegate.trackUnpublished.connect(
        [this](std::string trackSid, std::string identity, int kind, int source) {
            onTrackUnpublished(std::move(trackSid), std::move(identity), kind, source);
        });
    bag += delegate.connectionQualityChanged.connect(
        [this](std::string identity, int quality) {
            onConnectionQualityChanged(std::move(identity), quality);
        });
    bag += delegate.connectionStateChanged.connect(
        [this](int state) { onConnectionStateChanged(state); });
    bag += delegate.roomDisconnected.connect(
        [this](int reason) { onRoomDisconnected(reason); });
    bag += delegate.dataReceived.connect(
        [this](std::vector<std::uint8_t> data, std::string identity, std::string topic) {
            onDataReceived(std::move(data), std::move(identity), std::move(topic));
        });
    bag += delegate.localTrackPublished.connect(
        [this](std::string publicationSid, int kind, int source) {
            onLocalTrackPublished(std::move(publicationSid), kind, source);
        });

    auto& device = *deviceController_;
    bag += device.localMicrophoneChanged.connect(
        [this](bool enabled) { localMicrophoneChanged.notify(enabled); });
    bag += device.localCameraChanged.connect(
        [this](bool enabled) { localCameraChanged.notify(enabled); });
    bag += device.localScreenShareChanged.connect(
        [this](bool enabled) { localScreenShareChanged.notify(enabled); });
    bag += device.localVideoFrameReady.connect(
        [this](const core::VideoFrame& frame) { localVideoFrameReady.notify(frame); });
    bag += device.localScreenFrameReady.connect(
        [this](const core::VideoFrame& frame) { localScreenFrameReady.notify(frame); });
    bag += device.preferredCameraChanged.connect(
        [this](const std::string& id) { preferredCameraChanged.notify(id); });
    bag += device.preferredMicrophoneChanged.connect(
        [this](const std::string& id) { preferredMicrophoneChanged.notify(id); });

    bag += mediaPipeline_->videoFrameReady.connect(
        [this](const std::string& identity, const std::string& trackSid,
               const core::VideoFrame& frame, livekit::TrackSource source) {
            videoFrameReceived.notify(identity, trackSid, frame, source);
        });

    // Feed far-end (remote speaker) audio into the local APM for echo cancellation.
    // Without this, the AEC has no reference signal and cannot cancel echoes.
    mediaPipeline_->setReverseAudioCallback(
        [this](const int16_t* data, int samples, int sampleRate, int channels) {
            feedReverseAudio(data, samples, sampleRate, channels);
        });

    networkStatsTimer_ = services_.timers->createTimer([this]() { pollLocalNetworkStats(); });
}

ConferenceManager::~ConferenceManager()
{
    // First: no collaborator callback may run while members are torn down.
    collaboratorConnections_.clear();

    if (connected_) {
        disconnectFromRoom();
    }
}

void ConferenceManager::connectToRoom(const std::string& url, const std::string& token)
{
    core::logInfo(core::str::cat("Connecting to room: ", url));
    lastDisconnectReason_ = livekit::DisconnectReason::Unknown;
    deviceController_->setRoom(roomController_->room());

    try {
        livekit::RoomOptions options;
        options.auto_subscribe = true;
        options.dynacast = false;
        options.single_peer_connection = true;

        bool success = roomController_->connectToRoom(url, token, options);

        if (success) {
            core::logInfo("Connection initiated successfully");
            markConnected("connect_success", true);
        } else {
            core::logError("Connection failed");
            connectionError.notify("Failed to connect to room");
        }

    } catch (const std::exception& e) {
        const std::string error = core::str::cat("Connection failed: ", e.what());
        core::logError(error);
        connectionError.notify(error);
    }
}

void ConferenceManager::onLocalTrackPublished(std::string publicationSid, int kind, int source)
{
    (void)kind;
    if (!deviceController_) {
        return;
    }

    deviceController_->handleLocalTrackPublished(static_cast<livekit::TrackSource>(source),
                                                publicationSid);
}

void ConferenceManager::disconnectFromRoom()
{
    if (disconnecting_) {
        core::logWarning("Disconnect requested while cleanup is already in progress");
        return;
    }

    disconnecting_ = true;
    core::logInfo("Disconnecting from room");
    lastDisconnectReason_ = livekit::DisconnectReason::ClientInitiated;
    const bool wasConnected = connected_;
    connected_ = false;

    auto finalizeDisconnect = [this]() {
        deviceController_->resetLocalState();
        deviceController_->setRoom(roomController_->room());

        participantStore_->clear();
        participantIdentity_.clear();
        networkStatsTimer_->stop();
        resetNetworkMetrics();

        localConnectionQualityChanged.notify(static_cast<int>(localNetworkQuality_));
        localNetworkStatsUpdated.notify(localNetworkStats_);
        disconnected.notify();

        disconnecting_ = false;
    };

    try {
        deviceController_->stopCapturers();
    } catch (const std::exception& e) {
        core::logError(core::str::cat("Disconnect cleanup error while stopping capturers: ", e.what()));
    }

    try {
        // Clear streams before room reset so FFI listeners are removed safely.
        mediaPipeline_->stopAll();
    } catch (const std::exception& e) {
        core::logError(core::str::cat("Disconnect cleanup error while stopping media pipeline: ", e.what()));
    }

    if (roomController_->room()) {
        try {
            roomController_->clearDelegate();
        } catch (const std::exception& e) {
            core::logError(core::str::cat("Disconnect cleanup error while clearing room delegate: ", e.what()));
        }

        if (wasConnected) {
            try {
                deviceController_->unpublishLocalTracks();
            } catch (const std::exception& e) {
                core::logError(core::str::cat("Disconnect cleanup error while unpublishing local tracks: ", e.what()));
            }
        }

        try {
            // Ask the SDK for a graceful disconnect before tearing the room down.
            // Safe here because this always runs on the main thread, never from
            // inside a RoomDelegate callback (those arrive via queued connections).
            roomController_->disconnectFromRoom(livekit::DisconnectReason::ClientInitiated);
        } catch (const std::exception& e) {
            core::logError(core::str::cat("Disconnect cleanup error while disconnecting room: ", e.what()));
        }

        try {
            core::logInfo("Resetting room");
            roomController_->reset();
            core::logInfo("Room disconnected successfully");
        } catch (const std::exception& e) {
            core::logError(core::str::cat("Disconnect cleanup error while resetting room: ", e.what()));
        }
    }

    finalizeDisconnect();
}

void ConferenceManager::toggleMicrophone()
{
    if (!connected_) {
        core::logWarning("Ignoring toggleMicrophone: conference is not connected");
        return;
    }
    deviceController_->toggleMicrophone();
}

void ConferenceManager::toggleCamera()
{
    if (!connected_) {
        core::logWarning("Ignoring toggleCamera: conference is not connected");
        return;
    }
    deviceController_->toggleCamera();
}

void ConferenceManager::toggleScreenShare()
{
    if (!connected_) {
        core::logWarning("Ignoring toggleScreenShare: conference is not connected");
        return;
    }
    deviceController_->toggleScreenShare();
}

void ConferenceManager::setScreenShareMode(ScreenCapturer::Mode mode,
                                           core::MonitorId monitorId,
                                           core::WindowId windowId)
{
    deviceController_->setScreenShareMode(mode, monitorId, windowId);
}

void ConferenceManager::switchCamera(const std::string& deviceId)
{
    if (!connected_) {
        core::logWarning("Ignoring switchCamera: conference is not connected");
        return;
    }
    deviceController_->switchCamera(deviceId);
}

void ConferenceManager::switchMicrophone(const std::string& deviceId)
{
    if (!connected_) {
        core::logWarning("Ignoring switchMicrophone: conference is not connected");
        return;
    }
    deviceController_->switchMicrophone(deviceId);
}

bool ConferenceManager::isMicrophoneEnabled() const
{
    return deviceController_ && deviceController_->isMicrophoneEnabled();
}

bool ConferenceManager::isCameraEnabled() const
{
    return deviceController_ && deviceController_->isCameraEnabled();
}

bool ConferenceManager::isScreenSharing() const
{
    return deviceController_ && deviceController_->isScreenSharing();
}

// =============================================================================
// Audio processing settings (runtime-applicable during conference)
// =============================================================================

void ConferenceManager::applyAudioSettings()
{
    if (deviceController_) {
        deviceController_->applyAudioSettings();
    }
}

void ConferenceManager::setEchoCancellationEnabled(bool enabled)
{
    if (deviceController_) deviceController_->setEchoCancellationEnabled(enabled);
}

void ConferenceManager::setNoiseSuppressionEnabled(bool enabled)
{
    if (deviceController_) deviceController_->setNoiseSuppressionEnabled(enabled);
}

void ConferenceManager::setAutoGainControlEnabled(bool enabled)
{
    if (deviceController_) deviceController_->setAutoGainControlEnabled(enabled);
}

void ConferenceManager::setHighPassFilterEnabled(bool enabled)
{
    if (deviceController_) deviceController_->setHighPassFilterEnabled(enabled);
}

void ConferenceManager::setNoiseSuppressionLevel(int level)
{
    if (deviceController_) {
        auto nsLevel = static_cast<AudioProcessingModule::NoiseSuppressionLevel>(
            std::max(0, std::min(level, 3)));
        deviceController_->setNoiseSuppressionLevel(nsLevel);
    }
}

void ConferenceManager::setGainControlMode(int mode)
{
    if (deviceController_) {
        auto agcMode = static_cast<AudioProcessingModule::GainControlMode>(
            std::max(0, std::min(mode, 1)));
        deviceController_->setGainControlMode(agcMode);
    }
}

void ConferenceManager::setFixedDigitalGainDb(float gainDb)
{
    if (deviceController_) deviceController_->setFixedDigitalGainDb(gainDb);
}

void ConferenceManager::setAdaptiveDigitalMaxGainDb(float maxGainDb)
{
    if (deviceController_) deviceController_->setAdaptiveDigitalMaxGainDb(maxGainDb);
}

void ConferenceManager::setEchoEnhancedFilterEnabled(bool enabled)
{
    if (deviceController_) deviceController_->setEchoEnhancedFilterEnabled(enabled);
}

void ConferenceManager::feedReverseAudio(const int16_t* data, int samples,
                                          int sampleRate, int channels)
{
    if (deviceController_) deviceController_->feedReverseAudio(data, samples, sampleRate, channels);
}

void ConferenceManager::sendChatMessage(const std::string& message)
{
    if (!connected_ || core::str::trimWhitespace(message).empty()) {
        return;
    }

    try {
        auto localParticipant = roomController_->localParticipant();
        if (!localParticipant) {
            core::logWarning("No local participant");
            return;
        }

        // Wire format is unchanged: a compact JSON object with the same four
        // keys. nlohmann writes the ms timestamp as an integer, which is what
        // QJsonDocument produced for this magnitude, so older peers still parse it.
        nlohmann::json json;
        json["type"] = "chat";
        json["message"] = message;
        json["timestamp"] = core::nowMsSinceEpoch();
        json["sender"] = localParticipant->name();

        const std::string jsonData = json.dump();
        std::vector<uint8_t> data(jsonData.begin(), jsonData.end());
        localParticipant->publishData(data, true, {}, "chat");

        ChatMessage msg;
        msg.sender = std::string(localParticipant->name());
        msg.senderIdentity = std::string(localParticipant->identity());
        msg.message = message;
        msg.timestamp = core::nowMsSinceEpoch();
        msg.isLocal = true;

        chatMessageReceived.notify(msg);

        core::logDebug(core::str::cat("Chat message sent: ", message));

    } catch (const std::exception& e) {
        core::logError(core::str::cat("Failed to send chat message: ", e.what()));
    }
}

std::vector<ParticipantInfo> ConferenceManager::getParticipants() const
{
    return participantStore_->participants();
}

int ConferenceManager::getParticipantCount() const
{
    return participantStore_->size() + 1;
}

void ConferenceManager::reconcileParticipants()
{
    reconcileParticipantsInternal("manual");
}

void ConferenceManager::onParticipantConnected(std::string identity,
                                                     std::string sid,
                                                     std::string name,
                                                     bool isHost)
{
    if (core::str::trimWhitespace(identity).empty()) {
        core::logWarning("Participant connected event has empty identity, triggering reconciliation");
        reconcileParticipantsInternal("participant_connected_empty_identity");
        return;
    }

    if (participantStore_->contains(identity)) {
        const ParticipantInfo before = participantStore_->participantInfo(identity);
        const ParticipantInfo updated = participantStore_->addParticipant(identity, sid, name, isHost);
        if (before.name != updated.name || before.sid != updated.sid || before.isHost != updated.isHost) {
            participantUpdated.notify(updated);
        }
        core::logDebug(core::str::cat("Duplicate participant connected reconciled: ", identity));
        reconcileParticipantsInternal("participant_connected_duplicate");
        return;
    }

    ParticipantInfo info = participantStore_->addParticipant(identity, sid, name, isHost);

    core::logInfo(core::str::cat("Participant joined: ", name.empty() ? identity : name));
    participantJoined.notify(info);
    reconcileParticipantsInternal("participant_connected_event");
}

void ConferenceManager::onParticipantDisconnected(std::string identity, int reason)
{
    (void)reason;

    if (core::str::trimWhitespace(identity).empty()) {
        core::logWarning("Participant disconnected event has empty identity, triggering reconciliation");
        reconcileParticipantsInternal("participant_disconnected_empty_identity");
        return;
    }

    if (!participantStore_->contains(identity)) {
        core::logDebug(core::str::cat("Duplicate participant disconnected ignored: ", identity));
        reconcileParticipantsInternal("participant_disconnected_duplicate");
        return;
    }

    participantStore_->removeParticipant(identity);

    core::logInfo(core::str::cat("Participant left: ", identity));
    participantLeft.notify(identity);
    reconcileParticipantsInternal("participant_disconnected_event");
}

void ConferenceManager::onTrackSubscribed(std::string trackSid, std::string participantIdentity,
                                                int kind, int source, bool muted,
                                                std::shared_ptr<livekit::Track> track,
                                                std::shared_ptr<livekit::RemoteTrackPublication> publication)
{
    (void)publication;

    TrackInfo info;
    info.trackSid = trackSid;
    info.participantIdentity = participantIdentity;
    info.kind = static_cast<livekit::TrackKind>(kind);
    info.source = static_cast<livekit::TrackSource>(source);
    info.isLocal = false;
    info.track = track;

    participantStore_->setTrackSource(trackSid, info.source);
    participantStore_->setTrackKind(trackSid, info.kind);

    if (info.kind == livekit::TrackKind::KIND_VIDEO
        && (info.source == livekit::TrackSource::SOURCE_UNKNOWN
            || info.source == livekit::TrackSource::SOURCE_CAMERA)) {
        const std::string trackName =
            track ? core::str::toLowerAscii(track->name()) : std::string();
        if (core::str::contains(trackName, "screen") || core::str::contains(trackName, "share")) {
            participantStore_->setTrackSource(trackSid, livekit::TrackSource::SOURCE_SCREENSHARE);
            info.source = livekit::TrackSource::SOURCE_SCREENSHARE;
        }
    }

    bool isScreenShare = (participantStore_->trackSource(trackSid) == livekit::TrackSource::SOURCE_SCREENSHARE
                          || participantStore_->trackSource(trackSid)
                              == livekit::TrackSource::SOURCE_SCREENSHARE_AUDIO);

    std::string kindStr = (info.kind == livekit::TrackKind::KIND_AUDIO) ? "audio" : "video";
    core::logInfo(core::str::cat("Track subscribed: ", kindStr, " from %2"));

    if (info.kind == livekit::TrackKind::KIND_VIDEO && track) {
        try {
            if (isScreenShare) {
                participantStore_->setScreenShareActive(participantIdentity, true);
            }

            livekit::VideoStream::Options videoOptions;
            auto videoStream = livekit::VideoStream::fromTrack(track, videoOptions);
            mediaPipeline_->setVideoStream(trackSid, videoStream);
            mediaPipeline_->startVideoStreamReader(trackSid, participantIdentity, videoStream);

            services_.timers->singleShot(std::chrono::milliseconds(100),
                [this, trackSid, identity = participantIdentity,
                 kind = info.kind, muted]() {
                    trackMutedStateChanged.notify(trackSid, identity, kind, muted);
                });
        } catch (const std::exception& e) {
            core::logError(core::str::cat("Failed to create video stream: ", e.what()));
        }
    } else if (info.kind == livekit::TrackKind::KIND_AUDIO && track) {
        try {
            livekit::AudioStream::Options audioOptions;
            auto audioStream = livekit::AudioStream::fromTrack(track, audioOptions);
            mediaPipeline_->setAudioStream(trackSid, audioStream);
            mediaPipeline_->startAudioStreamReader(trackSid, participantIdentity, audioStream);

            services_.timers->singleShot(std::chrono::milliseconds(100),
                [this, trackSid, identity = participantIdentity,
                 kind = info.kind, muted]() {
                    trackMutedStateChanged.notify(trackSid, identity, kind, muted);
                });
        } catch (const std::exception& e) {
            core::logError(core::str::cat("Failed to create audio stream: ", e.what()));
        }
    }

    trackSubscribed.notify(info);
    updateParticipantInfo(participantIdentity);
    reconcileParticipantsInternal("track_subscribed_event");
}

void ConferenceManager::onTrackUnsubscribed(std::string trackSid, std::string participantIdentity)
{
    core::logInfo(core::str::cat("Track unsubscribed: ", trackSid, " from %2"));

    mediaPipeline_->stopTrack(trackSid);

    trackUnsubscribed.notify(trackSid, participantIdentity);

    livekit::TrackKind kind = participantStore_->trackKind(trackSid);
    trackMutedStateChanged.notify(trackSid, participantIdentity, kind, true);

    participantStore_->removeTrack(trackSid);

    updateParticipantInfo(participantIdentity);
    reconcileParticipantsInternal("track_unsubscribed_event");
}

void ConferenceManager::onTrackMuted(std::string trackSid, std::string participantIdentity, int kind)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    std::string kindStr = (trackKind == livekit::TrackKind::KIND_AUDIO) ? "AUDIO" : "VIDEO";
    core::logInfo(core::str::cat("Track muted: sid=", trackSid, ", identity=", participantIdentity, ", kind=", kindStr));

    participantStore_->setTrackKind(trackSid, trackKind);
    trackMutedStateChanged.notify(trackSid, participantIdentity, trackKind, true);
}

void ConferenceManager::onTrackUnmuted(std::string trackSid, std::string participantIdentity, int kind)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    std::string kindStr = (trackKind == livekit::TrackKind::KIND_AUDIO) ? "AUDIO" : "VIDEO";
    core::logInfo(core::str::cat("Track unmuted: sid=", trackSid, ", identity=", participantIdentity, ", kind=", kindStr));

    participantStore_->setTrackKind(trackSid, trackKind);
    trackMutedStateChanged.notify(trackSid, participantIdentity, trackKind, false);
}

void ConferenceManager::onTrackUnpublished(std::string trackSid, std::string participantIdentity, int kind, int source)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    livekit::TrackSource trackSource = static_cast<livekit::TrackSource>(source);

    core::logInfo(core::str::cat("Track unpublished: sid=", trackSid, ", identity=", kind, ", kind=", source, ", source=%4"));

    trackUnpublished.notify(trackSid, participantIdentity, trackKind, trackSource);

    participantStore_->removeTrack(trackSid);
    reconcileParticipantsInternal("track_unpublished_event");
}

void ConferenceManager::onConnectionQualityChanged(std::string participantIdentity, int quality)
{
    if (core::str::trimWhitespace(participantIdentity).empty()) {
        return;
    }

    const std::string localIdentity = resolveLocalParticipantIdentity();
    if (localIdentity.empty() || participantIdentity != localIdentity) {
        return;
    }

    const auto mappedQuality =
        toNetworkQualityLevel(static_cast<livekit::ConnectionQuality>(quality));
    if (mappedQuality == localNetworkQuality_) {
        return;
    }

    localNetworkQuality_ = mappedQuality;
    localConnectionQualityChanged.notify(static_cast<int>(localNetworkQuality_));

    if (!hasNetworkStatsData(localNetworkStats_) || usingEstimatedNetworkStats_) {
        const NetworkStatsSnapshot estimated =
            buildEstimatedNetworkSnapshot(localNetworkQuality_, core::nowMsSinceEpoch());
        usingEstimatedNetworkStats_ = true;
        if (!networkStatsEquivalent(localNetworkStats_, estimated)) {
            localNetworkStats_ = estimated;
            core::logDebug(core::str::cat("Estimated network stats applied from quality: rtt=", localNetworkStats_.rttMs, "ms, jitter=", localNetworkStats_.jitterMs, "ms, loss=", core::str::num(localNetworkStats_.packetLossPercent, 1), "%"));
            localNetworkStatsUpdated.notify(localNetworkStats_);
        }
    }
}

void ConferenceManager::onConnectionStateChanged(int state)
{
    livekit::ConnectionState connState = static_cast<livekit::ConnectionState>(state);
    core::logInfo(core::str::cat("Connection state changed: ", state));

    if (connState == livekit::ConnectionState::Connected) {
        markConnected("connection_connected_state", false);
    } else if (connState == livekit::ConnectionState::Reconnecting) {
        networkStatsTimer_->stop();
        resetNetworkMetrics();
        localConnectionQualityChanged.notify(static_cast<int>(localNetworkQuality_));
        localNetworkStatsUpdated.notify(localNetworkStats_);
    } else if (connState == livekit::ConnectionState::Disconnected) {
        const bool hadMic = deviceController_->isMicrophoneEnabled();
        const bool hadCam = deviceController_->isCameraEnabled();
        const bool hadScreenShare = deviceController_->isScreenSharing();

        deviceController_->stopCapturers();
        deviceController_->resetLocalState();

        if (hadMic) {
            localMicrophoneChanged.notify(false);
        }
        if (hadCam) {
            localCameraChanged.notify(false);
        }
        if (hadScreenShare) {
            localScreenShareChanged.notify(false);
        }

        connected_ = false;
        participantStore_->clear();
        participantIdentity_.clear();
        networkStatsTimer_->stop();
        resetNetworkMetrics();
        localConnectionQualityChanged.notify(static_cast<int>(localNetworkQuality_));
        localNetworkStatsUpdated.notify(localNetworkStats_);
        disconnected.notify();
    }

    connectionStateChanged.notify(connState);
}

void ConferenceManager::markConnected(const char* source, bool emitStateSignal)
{
    const bool wasConnected = connected_;
    connected_ = true;

    const auto roomInfo = roomController_->roomInfo();
    roomName_ = std::string(roomInfo.name);

    auto localParticipant = roomController_->localParticipant();
    if (localParticipant) {
        participantName_ = std::string(localParticipant->name());
        participantIdentity_ = std::string(localParticipant->identity());
    }

    reconcileParticipantsInternal(source);
    if (!networkStatsTimer_->isActive()) {
        // 1 s repeating, same cadence as the QTimer it replaces.
        networkStatsTimer_->start(std::chrono::milliseconds(1000), /*repeat=*/true);
    }
    pollLocalNetworkStats();

    if (!wasConnected) {
        connected.notify();
        if (emitStateSignal) {
            connectionStateChanged.notify(livekit::ConnectionState::Connected);
        }
    }
}

void ConferenceManager::onRoomDisconnected(int reason)
{
    lastDisconnectReason_ = static_cast<livekit::DisconnectReason>(reason);
    core::logInfo(core::str::cat("Room disconnected reason received: ", reason));
    roomDisconnected.notify(reason);
}

void ConferenceManager::onDataReceived(std::vector<std::uint8_t> data,
                                       std::string participantIdentity,
                                       std::string topic)
{
    (void)topic;

    try {
        const auto doc = nlohmann::json::parse(data.begin(), data.end(), nullptr,
                                               /*allow_exceptions=*/false);

        if (doc.is_discarded() || !doc.is_object()) {
            return;
        }

        const std::string type = doc.value("type", std::string());

        if (type == "chat") {
            ChatMessage msg;
            msg.sender = doc.value("sender", std::string());
            msg.senderIdentity = participantIdentity;
            msg.message = doc.value("message", std::string());
            msg.timestamp = doc.value("timestamp", std::int64_t{0});
            msg.isLocal = false;

            core::logDebug(core::str::cat("Chat message received from ", msg.sender));
            chatMessageReceived.notify(msg);
        }

    } catch (const std::exception& e) {
        core::logError(core::str::cat("Failed to parse data: ", e.what()));
    }
}

void ConferenceManager::updateParticipantInfo(const std::string& identity)
{
    if (!participantStore_->contains(identity)) {
        return;
    }

    if (!roomController_->room()) {
        participantUpdated.notify(participantStore_->participantInfo(identity));
        return;
    }

    auto participant = roomController_->remoteParticipant(identity);
    if (!participant) {
        participantUpdated.notify(participantStore_->participantInfo(identity));
        return;
    }

    ParticipantInfo updated = participantStore_->refreshParticipantInfo(identity);
    participantUpdated.notify(updated);
}

void ConferenceManager::reconcileParticipantsInternal(const char* source)
{
    if (!roomController_ || !participantStore_) {
        return;
    }

    if (!roomController_->room()) {
        return;
    }

    const auto remoteParticipants = roomController_->remoteParticipants();
    std::map<std::string, ParticipantInfo> remoteSnapshot;
    for (const auto& participant : remoteParticipants) {
        if (!participant) {
            continue;
        }

        const std::string identity = std::string(participant->identity());
        if (identity.empty()) {
            continue;
        }

        ParticipantInfo info;
        info.identity = identity;
        info.sid = std::string(participant->sid());
        info.name = std::string(participant->name());
        info.isMicrophoneEnabled = false;
        info.isCameraEnabled = false;
        info.isScreenSharing = false;
        info.isHost = links::conference::parseIsHostFromParticipantMetadata(participant->metadata());
        remoteSnapshot[identity] = info;
    }

    const std::vector<ParticipantInfo> storedParticipants = participantStore_->participants();
    std::set<std::string> storedIds;

    std::vector<std::string> removedIds;
    std::vector<std::string> addedIds;

    for (const ParticipantInfo& info : storedParticipants) {
        if (info.identity.empty()) {
            continue;
        }

        storedIds.insert(info.identity);
        if (remoteSnapshot.find(info.identity) == remoteSnapshot.end()) {
            participantStore_->removeParticipant(info.identity);
            participantLeft.notify(info.identity);
            removedIds.push_back(info.identity);
        }
    }

    for (const auto& entry : remoteSnapshot) {
        const std::string& identity = entry.first;
        const ParticipantInfo& snapshotInfo = entry.second;
        if (storedIds.find(identity) != storedIds.end()) {
            const ParticipantInfo currentInfo = participantStore_->participantInfo(identity);
            if (currentInfo.sid != snapshotInfo.sid
                || currentInfo.name != snapshotInfo.name
                || currentInfo.isHost != snapshotInfo.isHost) {
                participantStore_->addParticipant(
                    identity, snapshotInfo.sid, snapshotInfo.name, snapshotInfo.isHost);
            }
            continue;
        }

        ParticipantInfo added = participantStore_->addParticipant(
            identity, snapshotInfo.sid, snapshotInfo.name, snapshotInfo.isHost);
        participantJoined.notify(added);
        addedIds.push_back(identity);
    }

    if (!removedIds.empty() || !addedIds.empty()) {
        std::string message = core::str::cat(
            "Participant reconciliation (", source ? source : "unknown",
            "): before=", static_cast<unsigned long long>(storedParticipants.size()),
            ", after=", participantStore_->size());
        if (!addedIds.empty()) {
            message += core::str::cat(", added=[", core::str::join(addedIds, ","), "]");
        }
        if (!removedIds.empty()) {
            message += core::str::cat(", removed=[", core::str::join(removedIds, ","), "]");
        }
        core::logInfo(message);
    }
}

void ConferenceManager::pollLocalNetworkStats()
{
    if (!connected_ || !roomController_ || !roomController_->room()) {
        return;
    }

    const auto tracks = collectTrackStatsSources();
    const std::int64_t nowMs = core::nowMsSinceEpoch();
    std::set<std::string> currentTrackSids;
    currentTrackSids.reserve(static_cast<int>(tracks.size()));
    for (const auto& track : tracks) {
        if (!track) {
            continue;
        }

        const std::string sid = std::string(track->sid());
        if (!sid.empty()) {
            currentTrackSids.insert(sid);
        }
    }

    const bool trackSetChanged = (currentTrackSids != lastPolledTrackSids_);
    if (trackSetChanged) {
        lastPolledTrackSids_ = currentTrackSids;
        previousNetworkByteCounters_ = NetworkByteCounters{};

        // Invalidate in-flight results derived from a different track set.
        if (networkStatsPollInFlight_) {
            ++networkStatsPollSeq_;
            networkStatsPollInFlight_ = false;
        }
    }

    if (tracks.empty()) {
        previousNetworkByteCounters_ = NetworkByteCounters{};
        if (!usingEstimatedNetworkStats_) {
            NetworkStatsSnapshot snapshot;
            snapshot.sampledAtMs = nowMs;
            if (!networkStatsEquivalent(snapshot, localNetworkStats_)) {
                localNetworkStats_ = snapshot;
                localNetworkStatsUpdated.notify(localNetworkStats_);
            }
        }
        return;
    }

    if (networkStatsPollInFlight_) {
        return;
    }

    const NetworkByteCounters baselineCounters = previousNetworkByteCounters_;
    const std::uint64_t pollSeq = ++networkStatsPollSeq_;
    networkStatsPollInFlight_ = true;

    // Runs on Qt's global thread pool (the same one QtConcurrent::run used),
    // then hops the result back to the main thread. The pollSeq generation
    // counter still discards stale results, exactly as before.
    services_.background->run([this, tracks, baselineCounters, nowMs, pollSeq]() {
        AsyncNetworkPollResult result;
        try {
            std::vector<livekit::RtcStats> aggregatedStats;
            for (const auto& track : tracks) {
                if (!track) {
                    continue;
                }

                try {
                    auto statsFuture = track->getStats();
                    std::vector<livekit::RtcStats> stats = statsFuture.get();
                    aggregatedStats.insert(aggregatedStats.end(), stats.begin(), stats.end());
                } catch (...) {
                    // Ignore individual track failures to keep polling robust.
                }
            }

            if (!aggregatedStats.empty()) {
                result.hasData = true;
                result.aggregation = aggregateNetworkStats(aggregatedStats, baselineCounters, nowMs);
            }
        } catch (const std::exception& e) {
            core::logWarning(core::str::cat("Asynchronous network stats polling failed: ", e.what()));
        } catch (...) {
            core::logWarning("Asynchronous network stats polling failed with unknown error");
        }

        core::postGuarded(*services_.taskRunner, lifetime_,
            [this, result = std::move(result), pollSeq]() mutable {
                applyNetworkPollResult(std::move(result), pollSeq);
            });
    });
}

void ConferenceManager::applyNetworkPollResult(AsyncNetworkPollResult asyncResult,
                                               std::uint64_t pollSeq)
{
    if (pollSeq != networkStatsPollSeq_) {
        return;
    }

    networkStatsPollInFlight_ = false;
    if (!connected_ || !roomController_ || !roomController_->room()) {
        return;
    }

    if (!asyncResult.hasData) {
        return;
    }

    const NetworkStatsAggregationResult& aggregated = asyncResult.aggregation;
    previousNetworkByteCounters_ = aggregated.counters;

    // Grace period: keep estimated stats for the first 5 seconds after connection
    // to avoid a visual "blip" to empty values. After that, always use real data.
    const std::int64_t now = core::nowMsSinceEpoch();
    const bool withinGracePeriod = usingEstimatedNetworkStats_
        && (now - localNetworkStats_.sampledAtMs) < 5000;
    if (!hasNetworkStatsData(aggregated.snapshot) && withinGracePeriod) {
        return;
    }
    usingEstimatedNetworkStats_ = false;

    if (!networkStatsEquivalent(localNetworkStats_, aggregated.snapshot)) {
        localNetworkStats_ = aggregated.snapshot;
        core::logDebug(core::str::cat(
            "LiveKit network stats updated: rtt=", localNetworkStats_.rttMs,
            "ms, jitter=", localNetworkStats_.jitterMs,
            "ms, loss=", core::str::num(localNetworkStats_.packetLossPercent, 1),
            "%, up=", localNetworkStats_.uplinkKbps,
            "kbps, down=", localNetworkStats_.downlinkKbps,
            "kbps, bw=", localNetworkStats_.availableSendBandwidthKbps,
            "kbps, proto=", localNetworkStats_.transportProtocol.empty()
                ? std::string("none") : localNetworkStats_.transportProtocol,
            ", video=", localNetworkStats_.videoWidth, "x", localNetworkStats_.videoHeight,
            "@", core::str::num(localNetworkStats_.videoFps, 1),
            "fps, acodec=", localNetworkStats_.audioCodec.empty()
                ? std::string("none") : localNetworkStats_.audioCodec,
            ", vcodec=", localNetworkStats_.videoCodec.empty()
                ? std::string("none") : localNetworkStats_.videoCodec));
        localNetworkStatsUpdated.notify(localNetworkStats_);
    }
}

std::string ConferenceManager::resolveLocalParticipantIdentity() const
{
    if (!participantIdentity_.empty()) {
        return participantIdentity_;
    }

    if (!roomController_) {
        return {};
    }

    const auto localParticipant = roomController_->localParticipant();
    if (!localParticipant) {
        return {};
    }

    return std::string(localParticipant->identity());
}

std::vector<std::shared_ptr<livekit::Track>> ConferenceManager::collectTrackStatsSources() const
{
    std::vector<std::shared_ptr<livekit::Track>> tracks;
    if (!roomController_ || !roomController_->room()) {
        return tracks;
    }

    std::unordered_set<std::string> collectedTrackSids;
    auto appendTrack = [&](const std::shared_ptr<livekit::Track>& track) {
        if (!track) {
            return;
        }

        const std::string sid = track->sid();
        if (sid.empty()) {
            return;
        }

        if (collectedTrackSids.insert(sid).second) {
            tracks.push_back(track);
        }
    };

    if (const auto localParticipant = roomController_->localParticipant()) {
        for (const auto& publicationEntry : localParticipant->trackPublications()) {
            const auto& publication = publicationEntry.second;
            if (!publication) {
                continue;
            }
            appendTrack(publication->track());
        }
    }

    const auto remoteParticipants = roomController_->remoteParticipants();
    for (const auto& participant : remoteParticipants) {
        if (!participant) {
            continue;
        }

        for (const auto& publicationEntry : participant->trackPublications()) {
            const auto& publication = publicationEntry.second;
            if (!publication) {
                continue;
            }
            appendTrack(publication->track());
        }
    }

    return tracks;
}

NetworkStatsSnapshot ConferenceManager::buildEstimatedNetworkSnapshot(
    NetworkQualityLevel quality,
    std::int64_t nowMs) const
{
    NetworkStatsSnapshot snapshot;
    snapshot.sampledAtMs = nowMs;

    switch (quality) {
        case NetworkQualityLevel::Excellent:
            snapshot.rttMs = 60;
            snapshot.jitterMs = 8;
            snapshot.packetLossPercent = 0.2;
            break;
        case NetworkQualityLevel::Good:
            snapshot.rttMs = 120;
            snapshot.jitterMs = 18;
            snapshot.packetLossPercent = 1.0;
            break;
        case NetworkQualityLevel::Poor:
            snapshot.rttMs = 260;
            snapshot.jitterMs = 45;
            snapshot.packetLossPercent = 4.0;
            break;
        case NetworkQualityLevel::Lost:
        case NetworkQualityLevel::Unknown:
        default:
            break;
    }

    return snapshot;
}

void ConferenceManager::resetNetworkMetrics()
{
    localNetworkQuality_ = NetworkQualityLevel::Unknown;
    localNetworkStats_ = NetworkStatsSnapshot{};
    previousNetworkByteCounters_ = NetworkByteCounters{};
    usingEstimatedNetworkStats_ = false;
    networkStatsPollInFlight_ = false;
    ++networkStatsPollSeq_;
    lastPolledTrackSids_.clear();
}
