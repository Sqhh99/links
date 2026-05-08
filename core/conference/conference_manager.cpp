#include "conference_manager.h"
#include "participant_metadata_parser.h"
#include "../room_event_delegate.h"
#include "../../utils/logger.h"
#include <QFutureWatcher>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>
#include "livekit/audio_stream.h"
#include "livekit/local_participant.h"
#include "livekit/remote_participant.h"
#include "livekit/track.h"
#include "livekit/video_stream.h"

namespace {

struct AsyncNetworkPollResult {
    bool hasData{false};
    NetworkStatsAggregationResult aggregation;
};

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

ConferenceManager::ConferenceManager(QObject* parent)
    : QObject(parent),
      roomController_(std::make_unique<RoomController>()),
      roomDelegate_(std::make_unique<RoomEventDelegate>()),
      participantStore_(std::make_unique<ParticipantStore>()),
      mediaPipeline_(std::make_unique<MediaPipeline>(participantStore_.get())),
      deviceController_(std::make_unique<DeviceController>(roomController_->room()))
{
    Logger::instance().info("ConferenceManager created");
    qRegisterMetaType<livekit::TrackSource>("livekit::TrackSource");
    qRegisterMetaType<livekit::TrackKind>("livekit::TrackKind");
    qRegisterMetaType<NetworkStatsSnapshot>("NetworkStatsSnapshot");

    roomController_->setDelegate(roomDelegate_.get());

    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::participantConnectedQueued,
                     this, &ConferenceManager::onParticipantConnectedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::participantDisconnectedQueued,
                     this, &ConferenceManager::onParticipantDisconnectedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::trackSubscribedQueued,
                     this, &ConferenceManager::onTrackSubscribedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::trackUnsubscribedQueued,
                     this, &ConferenceManager::onTrackUnsubscribedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::trackMutedQueued,
                     this, &ConferenceManager::onTrackMutedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::trackUnmutedQueued,
                     this, &ConferenceManager::onTrackUnmutedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::trackUnpublishedQueued,
                     this, &ConferenceManager::onTrackUnpublishedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::connectionQualityChangedQueued,
                     this, &ConferenceManager::onConnectionQualityChangedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::connectionStateChangedQueued,
                     this, &ConferenceManager::onConnectionStateChangedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::roomDisconnectedQueued,
                     this, &ConferenceManager::onRoomDisconnectedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::dataReceivedQueued,
                     this, &ConferenceManager::onDataReceivedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::localTrackPublishedQueued,
                     this, &ConferenceManager::onLocalTrackPublishedQueued);

    QObject::connect(deviceController_.get(), &DeviceController::localMicrophoneChanged,
                     this, &ConferenceManager::localMicrophoneChanged);
    QObject::connect(deviceController_.get(), &DeviceController::localCameraChanged,
                     this, &ConferenceManager::localCameraChanged);
    QObject::connect(deviceController_.get(), &DeviceController::localScreenShareChanged,
                     this, &ConferenceManager::localScreenShareChanged);
    QObject::connect(deviceController_.get(), &DeviceController::localVideoFrameReady,
                     this, &ConferenceManager::localVideoFrameReady);
    QObject::connect(deviceController_.get(), &DeviceController::localScreenFrameReady,
                     this, &ConferenceManager::localScreenFrameReady);

    QObject::connect(mediaPipeline_.get(), &MediaPipeline::videoFrameReady,
                     this, &ConferenceManager::videoFrameReceived);
    QObject::connect(mediaPipeline_.get(), &MediaPipeline::audioActivity,
                     this, &ConferenceManager::audioActivity);

    // Feed far-end (remote speaker) audio into the local APM for echo cancellation.
    // Without this, the AEC has no reference signal and cannot cancel echoes.
    mediaPipeline_->setReverseAudioCallback(
        [this](const int16_t* data, int samples, int sampleRate, int channels) {
            feedReverseAudio(data, samples, sampleRate, channels);
        });

    networkStatsTimer_.setInterval(1000);
    networkStatsTimer_.setSingleShot(false);
    QObject::connect(&networkStatsTimer_, &QTimer::timeout,
                     this, &ConferenceManager::pollLocalNetworkStats);
}

ConferenceManager::~ConferenceManager()
{
    if (connected_) {
        disconnect();
    }
}

void ConferenceManager::connect(const QString& url, const QString& token)
{
    Logger::instance().info("Connecting to room: " + url);
    lastDisconnectReason_ = livekit::DisconnectReason::Unknown;
    deviceController_->setRoom(roomController_->room());

    try {
        livekit::RoomOptions options;
        options.auto_subscribe = true;
        options.dynacast = false;
        options.single_peer_connection = true;

        bool success = roomController_->connectToRoom(url, token, options);

        if (success) {
            Logger::instance().info("Connection initiated successfully");
            markConnected("connect_success", true);
        } else {
            Logger::instance().error("Connection failed");
            emit connectionError("Failed to connect to room");
        }

    } catch (const std::exception& e) {
        QString error = QString("Connection failed: %1").arg(e.what());
        Logger::instance().error(error);
        emit connectionError(error);
    }
}

void ConferenceManager::onLocalTrackPublishedQueued(QString publicationSid, int kind, int source)
{
    Q_UNUSED(kind);
    if (!deviceController_) {
        return;
    }

    deviceController_->handleLocalTrackPublished(static_cast<livekit::TrackSource>(source),
                                                publicationSid);
}

void ConferenceManager::disconnect()
{
    if (disconnecting_) {
        Logger::instance().warning("Disconnect requested while cleanup is already in progress");
        return;
    }

    disconnecting_ = true;
    Logger::instance().info("Disconnecting from room");
    lastDisconnectReason_ = livekit::DisconnectReason::ClientInitiated;
    const bool wasConnected = connected_;
    connected_ = false;

    auto finalizeDisconnect = [this]() {
        deviceController_->resetLocalState();
        deviceController_->setRoom(roomController_->room());

        participantStore_->clear();
        participantIdentity_.clear();
        networkStatsTimer_.stop();
        resetNetworkMetrics();

        emit localConnectionQualityChanged(static_cast<int>(localNetworkQuality_));
        emit localNetworkStatsUpdated(localNetworkStats_);
        emit disconnected();

        disconnecting_ = false;
    };

    try {
        deviceController_->stopCapturers();
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Disconnect cleanup error while stopping capturers: %1")
                                 .arg(e.what()));
    }

    try {
        // Clear streams before room reset so FFI listeners are removed safely.
        mediaPipeline_->stopAll();
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Disconnect cleanup error while stopping media pipeline: %1")
                                 .arg(e.what()));
    }

    if (roomController_->room()) {
        try {
            roomController_->clearDelegate();
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Disconnect cleanup error while clearing room delegate: %1")
                                     .arg(e.what()));
        }

        if (wasConnected) {
            try {
                deviceController_->unpublishLocalTracks();
            } catch (const std::exception& e) {
                Logger::instance().error(QString("Disconnect cleanup error while unpublishing local tracks: %1")
                                         .arg(e.what()));
            }
        }

        try {
            Logger::instance().info("Resetting room");
            roomController_->reset();
            Logger::instance().info("Room disconnected successfully");
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Disconnect cleanup error while resetting room: %1")
                                     .arg(e.what()));
        }
    }

    finalizeDisconnect();
}

void ConferenceManager::toggleMicrophone()
{
    if (!connected_) {
        Logger::instance().warning("Ignoring toggleMicrophone: conference is not connected");
        return;
    }
    deviceController_->toggleMicrophone();
}

void ConferenceManager::toggleCamera()
{
    if (!connected_) {
        Logger::instance().warning("Ignoring toggleCamera: conference is not connected");
        return;
    }
    deviceController_->toggleCamera();
}

void ConferenceManager::toggleScreenShare()
{
    if (!connected_) {
        Logger::instance().warning("Ignoring toggleScreenShare: conference is not connected");
        return;
    }
    deviceController_->toggleScreenShare();
}

void ConferenceManager::setScreenShareMode(ScreenCapturer::Mode mode, QScreen* screen, WId windowId)
{
    deviceController_->setScreenShareMode(mode, screen, windowId);
}

void ConferenceManager::switchCamera(const QString& deviceId)
{
    if (!connected_) {
        Logger::instance().warning("Ignoring switchCamera: conference is not connected");
        return;
    }
    deviceController_->switchCamera(deviceId);
}

void ConferenceManager::switchMicrophone(const QString& deviceId)
{
    if (!connected_) {
        Logger::instance().warning("Ignoring switchMicrophone: conference is not connected");
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

void ConferenceManager::sendChatMessage(const QString& message)
{
    if (!connected_ || message.trimmed().isEmpty()) {
        return;
    }

    try {
        auto localParticipant = roomController_->localParticipant();
        if (!localParticipant) {
            Logger::instance().warning("No local participant");
            return;
        }

        QJsonObject json;
        json["type"] = "chat";
        json["message"] = message;
        json["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        json["sender"] = QString::fromStdString(localParticipant->name());

        QJsonDocument doc(json);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        std::vector<uint8_t> data(jsonData.begin(), jsonData.end());
        localParticipant->publishData(data, true, {}, "chat");

        ChatMessage msg;
        msg.sender = QString::fromStdString(localParticipant->name());
        msg.senderIdentity = QString::fromStdString(localParticipant->identity());
        msg.message = message;
        msg.timestamp = QDateTime::currentMSecsSinceEpoch();
        msg.isLocal = true;

        emit chatMessageReceived(msg);

        Logger::instance().debug("Chat message sent: " + message);

    } catch (const std::exception& e) {
        Logger::instance().error(QString("Failed to send chat message: %1").arg(e.what()));
    }
}

QList<ParticipantInfo> ConferenceManager::getParticipants() const
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

void ConferenceManager::onParticipantConnectedQueued(QString identity,
                                                     QString sid,
                                                     QString name,
                                                     bool isHost)
{
    if (identity.trimmed().isEmpty()) {
        Logger::instance().warning("Participant connected event has empty identity, triggering reconciliation");
        reconcileParticipantsInternal("participant_connected_empty_identity");
        return;
    }

    if (participantStore_->contains(identity)) {
        const ParticipantInfo before = participantStore_->participantInfo(identity);
        const ParticipantInfo updated = participantStore_->addParticipant(identity, sid, name, isHost);
        if (before.name != updated.name || before.sid != updated.sid || before.isHost != updated.isHost) {
            emit participantUpdated(updated);
        }
        Logger::instance().debug(QString("Duplicate participant connected reconciled: %1").arg(identity));
        reconcileParticipantsInternal("participant_connected_duplicate");
        return;
    }

    ParticipantInfo info = participantStore_->addParticipant(identity, sid, name, isHost);

    Logger::instance().info(QString("Participant joined: %1")
                                .arg(name.isEmpty() ? identity : name));
    emit participantJoined(info);
    reconcileParticipantsInternal("participant_connected_event");
}

void ConferenceManager::onParticipantDisconnectedQueued(QString identity, int reason)
{
    Q_UNUSED(reason);

    if (identity.trimmed().isEmpty()) {
        Logger::instance().warning("Participant disconnected event has empty identity, triggering reconciliation");
        reconcileParticipantsInternal("participant_disconnected_empty_identity");
        return;
    }

    if (!participantStore_->contains(identity)) {
        Logger::instance().debug(QString("Duplicate participant disconnected ignored: %1").arg(identity));
        reconcileParticipantsInternal("participant_disconnected_duplicate");
        return;
    }

    participantStore_->removeParticipant(identity);

    Logger::instance().info("Participant left: " + identity);
    emit participantLeft(identity);
    reconcileParticipantsInternal("participant_disconnected_event");
}

void ConferenceManager::onTrackSubscribedQueued(QString trackSid, QString participantIdentity,
                                                int kind, int source, bool muted,
                                                std::shared_ptr<livekit::Track> track,
                                                std::shared_ptr<livekit::RemoteTrackPublication> publication)
{
    Q_UNUSED(publication);

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
        QString trackName = track ? QString::fromStdString(track->name()).toLower() : "";
        if (trackName.contains("screen") || trackName.contains("share")) {
            participantStore_->setTrackSource(trackSid, livekit::TrackSource::SOURCE_SCREENSHARE);
            info.source = livekit::TrackSource::SOURCE_SCREENSHARE;
        }
    }

    bool isScreenShare = (participantStore_->trackSource(trackSid) == livekit::TrackSource::SOURCE_SCREENSHARE
                          || participantStore_->trackSource(trackSid)
                              == livekit::TrackSource::SOURCE_SCREENSHARE_AUDIO);

    QString kindStr = (info.kind == livekit::TrackKind::KIND_AUDIO) ? "audio" : "video";
    Logger::instance().info(QString("Track subscribed: %1 from %2")
                           .arg(kindStr, participantIdentity));

    if (info.kind == livekit::TrackKind::KIND_VIDEO && track) {
        try {
            if (isScreenShare) {
                participantStore_->setScreenShareActive(participantIdentity, true);
            }

            livekit::VideoStream::Options videoOptions;
            auto videoStream = livekit::VideoStream::fromTrack(track, videoOptions);
            mediaPipeline_->setVideoStream(trackSid, videoStream);
            mediaPipeline_->startVideoStreamReader(trackSid, participantIdentity, videoStream);

            QTimer::singleShot(100, this, [this, trackSid, identity = participantIdentity,
                                           kind = info.kind, muted]() {
                emit trackMutedStateChanged(trackSid, identity, kind, muted);
            });
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Failed to create video stream: %1").arg(e.what()));
        }
    } else if (info.kind == livekit::TrackKind::KIND_AUDIO && track) {
        try {
            livekit::AudioStream::Options audioOptions;
            auto audioStream = livekit::AudioStream::fromTrack(track, audioOptions);
            mediaPipeline_->setAudioStream(trackSid, audioStream);
            mediaPipeline_->startAudioStreamReader(trackSid, participantIdentity, audioStream);

            QTimer::singleShot(100, this, [this, trackSid, identity = participantIdentity,
                                           kind = info.kind, muted]() {
                emit trackMutedStateChanged(trackSid, identity, kind, muted);
            });
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Failed to create audio stream: %1").arg(e.what()));
        }
    }

    emit trackSubscribed(info);
    updateParticipantInfo(participantIdentity);
    reconcileParticipantsInternal("track_subscribed_event");
}

void ConferenceManager::onTrackUnsubscribedQueued(QString trackSid, QString participantIdentity)
{
    Logger::instance().info(QString("Track unsubscribed: %1 from %2")
                           .arg(trackSid, participantIdentity));

    mediaPipeline_->stopTrack(trackSid);

    emit trackUnsubscribed(trackSid, participantIdentity);

    livekit::TrackKind kind = participantStore_->trackKind(trackSid);
    emit trackMutedStateChanged(trackSid, participantIdentity, kind, true);

    participantStore_->removeTrack(trackSid);

    updateParticipantInfo(participantIdentity);
    reconcileParticipantsInternal("track_unsubscribed_event");
}

void ConferenceManager::onTrackMutedQueued(QString trackSid, QString participantIdentity, int kind)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    QString kindStr = (trackKind == livekit::TrackKind::KIND_AUDIO) ? "AUDIO" : "VIDEO";
    Logger::instance().info(QString("Track muted: sid=%1, identity=%2, kind=%3")
        .arg(trackSid).arg(participantIdentity).arg(kindStr));

    participantStore_->setTrackKind(trackSid, trackKind);
    emit trackMutedStateChanged(trackSid, participantIdentity, trackKind, true);
}

void ConferenceManager::onTrackUnmutedQueued(QString trackSid, QString participantIdentity, int kind)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    QString kindStr = (trackKind == livekit::TrackKind::KIND_AUDIO) ? "AUDIO" : "VIDEO";
    Logger::instance().info(QString("Track unmuted: sid=%1, identity=%2, kind=%3")
        .arg(trackSid).arg(participantIdentity).arg(kindStr));

    participantStore_->setTrackKind(trackSid, trackKind);
    emit trackMutedStateChanged(trackSid, participantIdentity, trackKind, false);
}

void ConferenceManager::onTrackUnpublishedQueued(QString trackSid, QString participantIdentity, int kind, int source)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    livekit::TrackSource trackSource = static_cast<livekit::TrackSource>(source);

    Logger::instance().info(QString("Track unpublished: sid=%1, identity=%2, kind=%3, source=%4")
        .arg(trackSid, participantIdentity)
        .arg(kind).arg(source));

    emit trackUnpublished(trackSid, participantIdentity, trackKind, trackSource);

    participantStore_->removeTrack(trackSid);
    reconcileParticipantsInternal("track_unpublished_event");
}

void ConferenceManager::onConnectionQualityChangedQueued(QString participantIdentity, int quality)
{
    if (participantIdentity.trimmed().isEmpty()) {
        return;
    }

    const QString localIdentity = resolveLocalParticipantIdentity();
    if (localIdentity.isEmpty() || participantIdentity != localIdentity) {
        return;
    }

    const auto mappedQuality =
        toNetworkQualityLevel(static_cast<livekit::ConnectionQuality>(quality));
    if (mappedQuality == localNetworkQuality_) {
        return;
    }

    localNetworkQuality_ = mappedQuality;
    emit localConnectionQualityChanged(static_cast<int>(localNetworkQuality_));

    if (!hasNetworkStatsData(localNetworkStats_) || usingEstimatedNetworkStats_) {
        const NetworkStatsSnapshot estimated =
            buildEstimatedNetworkSnapshot(localNetworkQuality_, QDateTime::currentMSecsSinceEpoch());
        usingEstimatedNetworkStats_ = true;
        if (!networkStatsEquivalent(localNetworkStats_, estimated)) {
            localNetworkStats_ = estimated;
            Logger::instance().debug(
                QString("Estimated network stats applied from quality: rtt=%1ms, jitter=%2ms, loss=%3%")
                    .arg(localNetworkStats_.rttMs)
                    .arg(localNetworkStats_.jitterMs)
                    .arg(localNetworkStats_.packetLossPercent, 0, 'f', 1));
            emit localNetworkStatsUpdated(localNetworkStats_);
        }
    }
}

void ConferenceManager::onConnectionStateChangedQueued(int state)
{
    livekit::ConnectionState connState = static_cast<livekit::ConnectionState>(state);
    Logger::instance().info(QString("Connection state changed: %1").arg(state));

    if (connState == livekit::ConnectionState::Connected) {
        markConnected("connection_connected_state", false);
    } else if (connState == livekit::ConnectionState::Reconnecting) {
        networkStatsTimer_.stop();
        resetNetworkMetrics();
        emit localConnectionQualityChanged(static_cast<int>(localNetworkQuality_));
        emit localNetworkStatsUpdated(localNetworkStats_);
    } else if (connState == livekit::ConnectionState::Disconnected) {
        const bool hadMic = deviceController_->isMicrophoneEnabled();
        const bool hadCam = deviceController_->isCameraEnabled();
        const bool hadScreenShare = deviceController_->isScreenSharing();

        deviceController_->stopCapturers();
        deviceController_->resetLocalState();

        if (hadMic) {
            emit localMicrophoneChanged(false);
        }
        if (hadCam) {
            emit localCameraChanged(false);
        }
        if (hadScreenShare) {
            emit localScreenShareChanged(false);
        }

        connected_ = false;
        participantStore_->clear();
        participantIdentity_.clear();
        networkStatsTimer_.stop();
        resetNetworkMetrics();
        emit localConnectionQualityChanged(static_cast<int>(localNetworkQuality_));
        emit localNetworkStatsUpdated(localNetworkStats_);
        emit disconnected();
    }

    emit connectionStateChanged(connState);
}

void ConferenceManager::markConnected(const char* source, bool emitStateSignal)
{
    const bool wasConnected = connected_;
    connected_ = true;

    const auto roomInfo = roomController_->roomInfo();
    roomName_ = QString::fromStdString(roomInfo.name);

    auto localParticipant = roomController_->localParticipant();
    if (localParticipant) {
        participantName_ = QString::fromStdString(localParticipant->name());
        participantIdentity_ = QString::fromStdString(localParticipant->identity());
    }

    reconcileParticipantsInternal(source);
    if (!networkStatsTimer_.isActive()) {
        networkStatsTimer_.start();
    }
    pollLocalNetworkStats();

    if (!wasConnected) {
        emit connected();
        if (emitStateSignal) {
            emit connectionStateChanged(livekit::ConnectionState::Connected);
        }
    }
}

void ConferenceManager::onRoomDisconnectedQueued(int reason)
{
    lastDisconnectReason_ = static_cast<livekit::DisconnectReason>(reason);
    Logger::instance().info(QString("Room disconnected reason received: %1").arg(reason));
    emit roomDisconnected(reason);
}

void ConferenceManager::onDataReceivedQueued(QByteArray data, QString participantIdentity, QString topic)
{
    Q_UNUSED(topic);

    try {
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (!doc.isObject()) {
            return;
        }

        QJsonObject json = doc.object();
        QString type = json["type"].toString();

        if (type == "chat") {
            ChatMessage msg;
            msg.sender = json["sender"].toString();
            msg.senderIdentity = participantIdentity;
            msg.message = json["message"].toString();
            msg.timestamp = json["timestamp"].toVariant().toLongLong();
            msg.isLocal = false;

            Logger::instance().debug("Chat message received from " + msg.sender);
            emit chatMessageReceived(msg);
        }

    } catch (const std::exception& e) {
        Logger::instance().error(QString("Failed to parse data: %1").arg(e.what()));
    }
}

void ConferenceManager::updateParticipantInfo(const QString& identity)
{
    if (!participantStore_->contains(identity)) {
        return;
    }

    if (!roomController_->room()) {
        emit participantUpdated(participantStore_->participantInfo(identity));
        return;
    }

    auto participant = roomController_->room()->remoteParticipant(identity.toStdString());
    if (!participant) {
        emit participantUpdated(participantStore_->participantInfo(identity));
        return;
    }

    ParticipantInfo updated = participantStore_->refreshParticipantInfo(identity);
    emit participantUpdated(updated);
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
    QMap<QString, ParticipantInfo> remoteSnapshot;
    for (const auto& participant : remoteParticipants) {
        if (!participant) {
            continue;
        }

        const QString identity = QString::fromStdString(participant->identity());
        if (identity.isEmpty()) {
            continue;
        }

        ParticipantInfo info;
        info.identity = identity;
        info.sid = QString::fromStdString(participant->sid());
        info.name = QString::fromStdString(participant->name());
        info.isMicrophoneEnabled = false;
        info.isCameraEnabled = false;
        info.isScreenSharing = false;
        info.isHost = links::conference::parseIsHostFromParticipantMetadata(participant->metadata());
        remoteSnapshot.insert(identity, info);
    }

    const QList<ParticipantInfo> storedParticipants = participantStore_->participants();
    QSet<QString> storedIds;
    storedIds.reserve(storedParticipants.size());

    QStringList removedIds;
    QStringList addedIds;

    for (const ParticipantInfo& info : storedParticipants) {
        if (info.identity.isEmpty()) {
            continue;
        }

        storedIds.insert(info.identity);
        if (!remoteSnapshot.contains(info.identity)) {
            participantStore_->removeParticipant(info.identity);
            emit participantLeft(info.identity);
            removedIds.append(info.identity);
        }
    }

    for (auto it = remoteSnapshot.cbegin(); it != remoteSnapshot.cend(); ++it) {
        const ParticipantInfo& snapshotInfo = it.value();
        if (storedIds.contains(it.key())) {
            const ParticipantInfo currentInfo = participantStore_->participantInfo(it.key());
            if (currentInfo.sid != snapshotInfo.sid
                || currentInfo.name != snapshotInfo.name
                || currentInfo.isHost != snapshotInfo.isHost) {
                ParticipantInfo updated = participantStore_->addParticipant(
                    it.key(), snapshotInfo.sid, snapshotInfo.name, snapshotInfo.isHost);
                emit participantUpdated(updated);
            }
            continue;
        }

        ParticipantInfo added = participantStore_->addParticipant(
            it.key(), snapshotInfo.sid, snapshotInfo.name, snapshotInfo.isHost);
        emit participantJoined(added);
        addedIds.append(it.key());
    }

    if (!removedIds.isEmpty() || !addedIds.isEmpty()) {
        QString message = QString("Participant reconciliation (%1): before=%2, after=%3")
            .arg(source ? source : "unknown")
            .arg(storedParticipants.size())
            .arg(participantStore_->size());
        if (!addedIds.isEmpty()) {
            message += QString(", added=[%1]").arg(addedIds.join(","));
        }
        if (!removedIds.isEmpty()) {
            message += QString(", removed=[%1]").arg(removedIds.join(","));
        }
        Logger::instance().info(message);
    }
}

void ConferenceManager::pollLocalNetworkStats()
{
    if (!connected_ || !roomController_ || !roomController_->room()) {
        return;
    }

    const auto tracks = collectTrackStatsSources();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    QSet<QString> currentTrackSids;
    currentTrackSids.reserve(static_cast<int>(tracks.size()));
    for (const auto& track : tracks) {
        if (!track) {
            continue;
        }

        const QString sid = QString::fromStdString(track->sid());
        if (!sid.isEmpty()) {
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
                emit localNetworkStatsUpdated(localNetworkStats_);
            }
        }
        return;
    }

    if (networkStatsPollInFlight_) {
        return;
    }

    const NetworkByteCounters baselineCounters = previousNetworkByteCounters_;
    const quint64 pollSeq = ++networkStatsPollSeq_;
    networkStatsPollInFlight_ = true;

    auto* watcher = new QFutureWatcher<AsyncNetworkPollResult>(this);
    QObject::connect(watcher, &QFutureWatcher<AsyncNetworkPollResult>::finished,
                     this, [this, watcher, pollSeq]() {
        AsyncNetworkPollResult asyncResult;
        try {
            asyncResult = watcher->result();
        } catch (const std::exception& e) {
            Logger::instance().warning(QString("Asynchronous network stats polling failed: %1")
                                       .arg(e.what()));
        } catch (...) {
            Logger::instance().warning("Asynchronous network stats polling failed with unknown error");
        }
        watcher->deleteLater();

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
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const bool withinGracePeriod = usingEstimatedNetworkStats_
            && (now - localNetworkStats_.sampledAtMs) < 5000;
        if (!hasNetworkStatsData(aggregated.snapshot) && withinGracePeriod) {
            return;
        }
        usingEstimatedNetworkStats_ = false;

        if (!networkStatsEquivalent(localNetworkStats_, aggregated.snapshot)) {
            localNetworkStats_ = aggregated.snapshot;
            Logger::instance().debug(
                QString("LiveKit network stats updated: rtt=%1ms, jitter=%2ms, loss=%3%, up=%4kbps, down=%5kbps, "
                        "bw=%6kbps, proto=%7, video=%8x%9@%10fps, acodec=%11, vcodec=%12")
                    .arg(localNetworkStats_.rttMs)
                    .arg(localNetworkStats_.jitterMs)
                    .arg(localNetworkStats_.packetLossPercent, 0, 'f', 1)
                    .arg(localNetworkStats_.uplinkKbps)
                    .arg(localNetworkStats_.downlinkKbps)
                    .arg(localNetworkStats_.availableSendBandwidthKbps)
                    .arg(localNetworkStats_.transportProtocol.isEmpty()
                        ? QStringLiteral("none") : localNetworkStats_.transportProtocol)
                    .arg(localNetworkStats_.videoWidth)
                    .arg(localNetworkStats_.videoHeight)
                    .arg(localNetworkStats_.videoFps, 0, 'f', 1)
                    .arg(localNetworkStats_.audioCodec.isEmpty()
                        ? QStringLiteral("none") : localNetworkStats_.audioCodec)
                    .arg(localNetworkStats_.videoCodec.isEmpty()
                        ? QStringLiteral("none") : localNetworkStats_.videoCodec));
            emit localNetworkStatsUpdated(localNetworkStats_);
        }
    });

    QFuture<AsyncNetworkPollResult> future = QtConcurrent::run(
        [tracks, baselineCounters, nowMs]() -> AsyncNetworkPollResult {
            AsyncNetworkPollResult result;
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

            if (aggregatedStats.empty()) {
                return result;
            }

            result.hasData = true;
            result.aggregation = aggregateNetworkStats(aggregatedStats, baselineCounters, nowMs);
            return result;
        });
    watcher->setFuture(future);
}

QString ConferenceManager::resolveLocalParticipantIdentity() const
{
    if (!participantIdentity_.isEmpty()) {
        return participantIdentity_;
    }

    if (!roomController_ || !roomController_->localParticipant()) {
        return {};
    }

    return QString::fromStdString(roomController_->localParticipant()->identity());
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

    if (auto* localParticipant = roomController_->localParticipant()) {
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
    qint64 nowMs) const
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
