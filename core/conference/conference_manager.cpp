#include "conference_manager.h"
#include "../room_event_delegate.h"
#include "../../utils/logger.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <vector>
#include "livekit/audio_stream.h"
#include "livekit/remote_participant.h"
#include "livekit/video_stream.h"

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
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::connectionStateChangedQueued,
                     this, &ConferenceManager::onConnectionStateChangedQueued);
    QObject::connect(roomDelegate_.get(), &RoomEventDelegate::dataReceivedQueued,
                     this, &ConferenceManager::onDataReceivedQueued);

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

    try {
        livekit::RoomOptions options;
        options.auto_subscribe = true;
        options.dynacast = false;

        bool success = roomController_->connectToRoom(url, token, options);

        if (success) {
            Logger::instance().info("Connection initiated successfully");
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

void ConferenceManager::disconnect()
{
    Logger::instance().info("Disconnecting from room");

    try {
        deviceController_->stopCapturers();
        // Clear streams before room reset so FFI listeners are removed safely.
        mediaPipeline_->stopAll();

        if (roomController_->room()) {
            roomController_->clearDelegate();

            if (connected_) {
                deviceController_->unpublishLocalTracks();
            }

            Logger::instance().info("Resetting room");
            roomController_->reset();
            Logger::instance().info("Room disconnected successfully");
        }

        deviceController_->resetLocalState();
        deviceController_->setRoom(roomController_->room());

        connected_ = false;
        participantStore_->clear();
        participantIdentity_.clear();

        emit disconnected();
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Disconnect error: %1").arg(e.what()));
    }
}

void ConferenceManager::toggleMicrophone()
{
    deviceController_->toggleMicrophone();
}

void ConferenceManager::toggleCamera()
{
    deviceController_->toggleCamera();
}

void ConferenceManager::toggleScreenShare()
{
    deviceController_->toggleScreenShare();
}

void ConferenceManager::setScreenShareMode(ScreenCapturer::Mode mode, QScreen* screen, WId windowId)
{
    deviceController_->setScreenShareMode(mode, screen, windowId);
}

void ConferenceManager::switchCamera(const QString& deviceId)
{
    deviceController_->switchCamera(deviceId);
}

void ConferenceManager::switchMicrophone(const QString& deviceId)
{
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

void ConferenceManager::onParticipantConnectedQueued(QString identity, QString sid, QString name)
{
    if (identity.trimmed().isEmpty()) {
        Logger::instance().warning("Participant connected event has empty identity, triggering reconciliation");
        reconcileParticipantsInternal("participant_connected_empty_identity");
        return;
    }

    if (participantStore_->contains(identity)) {
        Logger::instance().debug(QString("Duplicate participant connected ignored: %1").arg(identity));
        reconcileParticipantsInternal("participant_connected_duplicate");
        return;
    }

    ParticipantInfo info = participantStore_->addParticipant(identity, sid, name);

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

void ConferenceManager::onConnectionStateChangedQueued(int state)
{
    livekit::ConnectionState connState = static_cast<livekit::ConnectionState>(state);
    Logger::instance().info(QString("Connection state changed: %1").arg(state));

    if (connState == livekit::ConnectionState::Connected) {
        connected_ = true;

        const auto roomInfo = roomController_->roomInfo();
        roomName_ = QString::fromStdString(roomInfo.name);

        auto localParticipant = roomController_->localParticipant();
        if (localParticipant) {
            participantName_ = QString::fromStdString(localParticipant->name());
            participantIdentity_ = QString::fromStdString(localParticipant->identity());
        }

        reconcileParticipantsInternal("connection_connected_state");

        emit connected();
    } else if (connState == livekit::ConnectionState::Disconnected) {
        connected_ = false;
        participantStore_->clear();
        participantIdentity_.clear();
        emit disconnected();
    }

    emit connectionStateChanged(connState);
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
        if (storedIds.contains(it.key())) {
            continue;
        }

        const ParticipantInfo& snapshotInfo = it.value();
        ParticipantInfo added = participantStore_->addParticipant(it.key(), snapshotInfo.sid, snapshotInfo.name);
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
