#include "conference_manager.h"
#include "room_event_delegate.h"
#include "../utils/logger.h"
#include "../utils/settings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QMediaDevices>
#include <QAudioSink>
#include <QMetaType>
#include <QAudioDevice>
#include "livekit/local_video_track.h"
#include "livekit/local_audio_track.h"
#include "livekit/video_stream.h"
#include "livekit/audio_stream.h"
#include "livekit/remote_participant.h"

ConferenceManager::ConferenceManager(QObject* parent)
    : QObject(parent),
      room_(std::make_unique<livekit::Room>()),
      roomDelegate_(std::make_unique<RoomEventDelegate>(this)),
      cameraCapturer_(nullptr),
      microphoneCapturer_(nullptr),
      screenCapturer_(nullptr),
      connected_(false),
      microphoneEnabled_(false),
      cameraEnabled_(false),
      screenShareEnabled_(false)
{
    Logger::instance().info("ConferenceManager created");
    qRegisterMetaType<livekit::TrackSource>("livekit::TrackSource");
    qRegisterMetaType<livekit::TrackKind>("livekit::TrackKind");
    
    // Set up room delegate
    room_->setDelegate(roomDelegate_.get());
    
    // Create media capturers
    cameraCapturer_ = new CameraCapturer(this);
    microphoneCapturer_ = new MicrophoneCapturer(this);
    
    // Apply device settings from configuration
    auto& settings = Settings::instance();
    
    // Set camera device if configured
    QString cameraId = settings.getSelectedCameraId();
    if (!cameraId.isEmpty()) {
        cameraCapturer_->setCameraById(cameraId.toUtf8());
    }
    
    // Set microphone device if configured
    QString micId = settings.getSelectedMicrophoneId();
    if (!micId.isEmpty()) {
        microphoneCapturer_->setDeviceById(micId.toUtf8());
    }
    screenCapturer_ = new ScreenCapturer(this);
    
    // Connect error signals
    QObject::connect(cameraCapturer_, &CameraCapturer::error, this, [this](const QString& msg) {
        Logger::instance().error(QString("Camera error: %1").arg(msg));
    });
    QObject::connect(cameraCapturer_, &CameraCapturer::frameCaptured, this, [this](const QImage& frame) {
        emit localVideoFrameReady(frame);
    });
    
    QObject::connect(microphoneCapturer_, &MicrophoneCapturer::error, this, [this](const QString& msg) {
        Logger::instance().error(QString("Microphone error: %1").arg(msg));
    });

    QObject::connect(screenCapturer_, &ScreenCapturer::error, this, [this](const QString& msg) {
        Logger::instance().error(QString("Screen capture error: %1").arg(msg));
        if (screenShareEnabled_) {
            screenCapturer_->stop();
            auto localParticipant = room_->localParticipant();
            if (localParticipant && localScreenTrack_) {
                localParticipant->unpublishTrack(localScreenTrack_->sid());
                localScreenTrack_ = nullptr;
            }
            screenShareEnabled_ = false;
            emit localScreenShareChanged(false);
        }
    });
    
    // Connect RoomEventDelegate signals to ConferenceManager slots
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
        // Events are now handled via RoomEventDelegate set up in constructor
        
        // Configure room options
        livekit::RoomOptions options;
        options.auto_subscribe = true;  // Critical: enables automatic track subscription
        options.dynacast = false;
        
        // Connect to room with options
        bool success = room_->Connect(url.toStdString(), token.toStdString(), options);
        
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
        // Stop capturers first
        if (cameraCapturer_) cameraCapturer_->stop();
        if (microphoneCapturer_) microphoneCapturer_->stop();
        if (screenCapturer_) screenCapturer_->stop();

        // CRITICAL: Stop all stream reader threads FIRST
        // This must happen before clearing streams to avoid race conditions
        for (auto it = streamStopFlags_.begin(); it != streamStopFlags_.end(); ++it) {
            if (it.value()) {
                it.value()->store(true);
            }
        }
        
        // Join all video reader threads
        for (auto& [trackSid, threadPtr] : videoStreamThreads_) {
            if (threadPtr && threadPtr->joinable()) {
                threadPtr->join();
            }
        }
        videoStreamThreads_.clear();
        
        // Join all audio reader threads
        for (auto& [trackSid, threadPtr] : audioStreamThreads_) {
            if (threadPtr && threadPtr->joinable()) {
                threadPtr->join();
            }
        }
        audioStreamThreads_.clear();
        
        // Clean up stop flags
        for (auto it = streamStopFlags_.begin(); it != streamStopFlags_.end(); ++it) {
            delete it.value();
        }
        streamStopFlags_.clear();
        
        // CRITICAL: Clear streams BEFORE room destruction
        // VideoStream and AudioStream destructors call FfiClient::RemoveListener
        // which must happen while FfiClient is still valid
        Logger::instance().info("Cleaning up video streams");
        videoStreams_.clear();
        
        Logger::instance().info("Cleaning up audio streams");
        audioStreams_.clear();
        
        // Stop audio players
        for (auto& player : audioPlayers_) {
            if (player.sink) {
                player.sink->stop();
            }
        }
        audioPlayers_.clear();

        if (room_) {
            // Step 1: Remove delegate to prevent callbacks during cleanup (prevents use-after-free)
            room_->setDelegate(nullptr);
            
            // Step 2: Unpublish all tracks before destroying the room (only if connected)
            if (connected_) {
                auto localParticipant = room_->localParticipant();
                if (localParticipant) {
                    // Unpublish audio track
                    if (localAudioTrack_ && !localAudioTrack_->sid().empty()) {
                        Logger::instance().info("Unpublishing audio track");
                        localParticipant->unpublishTrack(localAudioTrack_->sid());
                    }
                    
                    // Unpublish camera video track
                    if (!cameraTrackSid_.empty()) {
                        Logger::instance().info("Unpublishing camera track");
                        localParticipant->unpublishTrack(cameraTrackSid_);
                    }
                    
                    // Unpublish screen share track
                    if (!screenTrackSid_.empty()) {
                        Logger::instance().info("Unpublishing screen share track");
                        localParticipant->unpublishTrack(screenTrackSid_);
                    }
                }
            }
            
            // Step 3: Reset the room (this triggers disconnect in FFI layer)
            Logger::instance().info("Resetting room");
            room_.reset();
            
            Logger::instance().info("Room disconnected successfully");
        }
        
        // Clear local state
        localVideoTrack_ = nullptr;
        localAudioTrack_ = nullptr;
        localScreenTrack_ = nullptr;
        cameraTrackSid_.clear();
        screenTrackSid_.clear();
        cameraEnabled_ = false;
        microphoneEnabled_ = false;
        screenShareEnabled_ = false;
        connected_ = false;
        
        // Clear participant and tracking data
        participants_.clear();
        trackSources_.clear();
        trackKinds_.clear();
        screenShareActive_.clear();
        
        emit disconnected();
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Disconnect error: %1").arg(e.what()));
    }
}

void ConferenceManager::toggleMicrophone()
{
    microphoneEnabled_ = !microphoneEnabled_;
    Logger::instance().info(QString("Microphone toggled: %1").arg(microphoneEnabled_ ? "ON" : "OFF"));
    
    try {
        if (microphoneEnabled_) {
            Logger::instance().info("Starting microphone capturer...");
            if (microphoneCapturer_->start()) {
                Logger::instance().info("Microphone capturer started successfully");
                auto source = microphoneCapturer_->getAudioSource();
                Logger::instance().info(QString("Got audio source: %1").arg(source ? "valid" : "null"));
                
                if (source) {
                    if (!localAudioTrack_) {
                        Logger::instance().info("Creating audio track...");
                        localAudioTrack_ = livekit::LocalAudioTrack::createLocalAudioTrack("mic", source);
                        Logger::instance().info(QString("Audio track created: %1").arg(localAudioTrack_ ? "valid" : "null"));
                    }
                    
                    auto localParticipant = room_->localParticipant();
                    Logger::instance().info(QString("Got local participant: %1").arg(localParticipant ? "valid" : "null"));
                    
                    if (localParticipant && localAudioTrack_) {
                        Logger::instance().info("Publishing audio track...");
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_MICROPHONE;
                        localParticipant->publishTrack(localAudioTrack_, options);
                        Logger::instance().info("Audio track published successfully");
                    }
                }
            } else {
                Logger::instance().error("Failed to start microphone");
                microphoneEnabled_ = false;
            }
        } else {
            microphoneCapturer_->stop();
            
            auto localParticipant = room_->localParticipant();
            if (localParticipant && localAudioTrack_) {
                localParticipant->unpublishTrack(localAudioTrack_->sid());
                localAudioTrack_ = nullptr;
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in toggleMicrophone: %1").arg(e.what()));
        microphoneEnabled_ = false;
    }
    
    emit localMicrophoneChanged(microphoneEnabled_);
}

void ConferenceManager::toggleCamera()
{
    cameraEnabled_ = !cameraEnabled_;
    Logger::instance().info(QString("Camera toggled: %1").arg(cameraEnabled_ ? "ON" : "OFF"));
    
    try {
        if (cameraEnabled_) {
            Logger::instance().info("Starting camera capturer...");
            if (cameraCapturer_->start()) {
                Logger::instance().info("Camera capturer started successfully");
                auto source = cameraCapturer_->getVideoSource();
                Logger::instance().info(QString("Got video source: %1").arg(source ? "valid" : "null"));
                
                if (source) {
                    if (!localVideoTrack_) {
                        Logger::instance().info("Creating video track...");
                        localVideoTrack_ = livekit::LocalVideoTrack::createLocalVideoTrack("camera", source);
                        Logger::instance().info(QString("Video track created: %1").arg(localVideoTrack_ ? "valid" : "null"));
                    }
                    
                    auto localParticipant = room_->localParticipant();
                    Logger::instance().info(QString("Got local participant: %1").arg(localParticipant ? "valid" : "null"));
                    
                    if (localParticipant && localVideoTrack_) {
                        Logger::instance().info("Publishing video track...");
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_CAMERA;
                        auto publication = localParticipant->publishTrack(localVideoTrack_, options);
                        if (publication) {
                            cameraTrackSid_ = publication->sid();
                            Logger::instance().info(QString("Video track published with SID: %1")
                                .arg(QString::fromStdString(cameraTrackSid_)));
                        }
                    }
                }
            } else {
                Logger::instance().error("Failed to start camera");
                cameraEnabled_ = false;
            }
        } else {
            cameraCapturer_->stop();
            
            auto localParticipant = room_->localParticipant();
            if (localParticipant && !cameraTrackSid_.empty()) {
                Logger::instance().info(QString("Unpublishing camera track: %1")
                    .arg(QString::fromStdString(cameraTrackSid_)));
                localParticipant->unpublishTrack(cameraTrackSid_);
                cameraTrackSid_.clear();
                localVideoTrack_ = nullptr;
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in toggleCamera: %1").arg(e.what()));
        cameraEnabled_ = false;
    }
    
    emit localCameraChanged(cameraEnabled_);
}

void ConferenceManager::toggleScreenShare()
{
    // Debounce rapid toggling to prevent resource conflicts
    if (screenShareDebounceTimer_.isValid() && 
        screenShareDebounceTimer_.elapsed() < kScreenShareDebounceMs) {
        Logger::instance().warning("Screen share toggle debounced, ignoring rapid toggle");
        return;
    }
    screenShareDebounceTimer_.start();
    
    screenShareEnabled_ = !screenShareEnabled_;
    Logger::instance().info(QString("Screen sharing toggled: %1").arg(screenShareEnabled_ ? "ON" : "OFF"));
    
    try {
        if (screenShareEnabled_) {
            Logger::instance().info("Starting screen capturer...");
            if (screenCapturer_->start()) {
                connectScreenSignals();
                auto source = screenCapturer_->getVideoSource();
                if (source) {
                    localScreenTrack_ = livekit::LocalVideoTrack::createLocalVideoTrack("screen", source);
                }

                auto localParticipant = room_->localParticipant();
                if (localParticipant && localScreenTrack_) {
                    livekit::TrackPublishOptions options;
                    options.source = livekit::TrackSource::SOURCE_SCREENSHARE;
                    auto publication = localParticipant->publishTrack(localScreenTrack_, options);
                    if (publication) {
                        screenTrackSid_ = publication->sid();
                        Logger::instance().info(QString("Screen share track published with SID: %1")
                            .arg(QString::fromStdString(screenTrackSid_)));
                    }
                }
            } else {
                Logger::instance().error("Failed to start screen sharing");
                screenShareEnabled_ = false;
            }
        } else {
            // Stop screen capturer first to ensure no frames are being captured
            screenCapturer_->stop();
            
            auto localParticipant = room_->localParticipant();
            if (localParticipant && !screenTrackSid_.empty()) {
                Logger::instance().info(QString("Unpublishing screen track: %1")
                    .arg(QString::fromStdString(screenTrackSid_)));
                localParticipant->unpublishTrack(screenTrackSid_);
                Logger::instance().info("Screen track unpublished, releasing reference");
                screenTrackSid_.clear();
                localScreenTrack_.reset();
                Logger::instance().info("Screen track reference released");
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in toggleScreenShare: %1").arg(e.what()));
        screenShareEnabled_ = false;
    }

    emit localScreenShareChanged(screenShareEnabled_);
}

void ConferenceManager::setScreenShareMode(ScreenCapturer::Mode mode, QScreen* screen, WId windowId)
{
    if (!screenCapturer_) return;
    screenCapturer_->setMode(mode);
    if (mode == ScreenCapturer::Mode::Screen && screen) {
        screenCapturer_->setScreen(screen);
    } else if (mode == ScreenCapturer::Mode::Window) {
        screenCapturer_->setWindow(windowId);
    }
}

void ConferenceManager::switchCamera(const QString& deviceId)
{
    Logger::instance().info(QString("Switching camera to device: %1").arg(deviceId));
    
    try {
        bool wasEnabled = cameraEnabled_;
        
        // Stop current camera if enabled
        if (cameraEnabled_) {
            cameraCapturer_->stop();
            
            // Unpublish current track
            auto localParticipant = room_->localParticipant();
            if (localParticipant && localVideoTrack_) {
                localParticipant->unpublishTrack(localVideoTrack_->sid());
                localVideoTrack_ = nullptr;
            }
        }
        
        // Set new camera device
        cameraCapturer_->setCameraById(deviceId.toUtf8());
        
        // Restart if was enabled
        if (wasEnabled) {
            if (cameraCapturer_->start()) {
                auto source = cameraCapturer_->getVideoSource();
                if (source) {
                    localVideoTrack_ = livekit::LocalVideoTrack::createLocalVideoTrack("camera", source);
                    
                    auto localParticipant = room_->localParticipant();
                    if (localParticipant && localVideoTrack_) {
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_CAMERA;
                        localParticipant->publishTrack(localVideoTrack_, options);
                        Logger::instance().info("Camera switched and republished successfully");
                    }
                }
            } else {
                Logger::instance().error("Failed to restart camera with new device");
                cameraEnabled_ = false;
                emit localCameraChanged(cameraEnabled_);
            }
        }
        
        // Save to settings
        Settings::instance().setSelectedCameraId(deviceId);
        Settings::instance().sync();
        
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in switchCamera: %1").arg(e.what()));
    }
}

void ConferenceManager::switchMicrophone(const QString& deviceId)
{
    Logger::instance().info(QString("Switching microphone to device: %1").arg(deviceId));
    
    try {
        bool wasEnabled = microphoneEnabled_;
        
        // Stop current microphone if enabled
        if (microphoneEnabled_) {
            microphoneCapturer_->stop();
            
            // Unpublish current track
            auto localParticipant = room_->localParticipant();
            if (localParticipant && localAudioTrack_) {
                localParticipant->unpublishTrack(localAudioTrack_->sid());
                localAudioTrack_ = nullptr;
            }
        }
        
        // Set new microphone device
        microphoneCapturer_->setDeviceById(deviceId.toUtf8());
        
        // Restart if was enabled
        if (wasEnabled) {
            if (microphoneCapturer_->start()) {
                auto source = microphoneCapturer_->getAudioSource();
                if (source) {
                    localAudioTrack_ = livekit::LocalAudioTrack::createLocalAudioTrack("mic", source);
                    
                    auto localParticipant = room_->localParticipant();
                    if (localParticipant && localAudioTrack_) {
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_MICROPHONE;
                        localParticipant->publishTrack(localAudioTrack_, options);
                        Logger::instance().info("Microphone switched and republished successfully");
                    }
                }
            } else {
                Logger::instance().error("Failed to restart microphone with new device");
                microphoneEnabled_ = false;
                emit localMicrophoneChanged(microphoneEnabled_);
            }
        }
        
        // Save to settings
        Settings::instance().setSelectedMicrophoneId(deviceId);
        Settings::instance().sync();
        
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in switchMicrophone: %1").arg(e.what()));
    }
}

void ConferenceManager::sendChatMessage(const QString& message)
{
    if (!connected_ || message.trimmed().isEmpty()) {
        return;
    }
    
    try {
        auto localParticipant = room_->localParticipant();
        if (!localParticipant) {
            Logger::instance().warning("No local participant");
            return;
        }
        
        // Create JSON message
        QJsonObject json;
        json["type"] = "chat";
        json["message"] = message;
        json["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        json["sender"] = QString::fromStdString(localParticipant->name());
        
        QJsonDocument doc(json);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        std::vector<uint8_t> data(jsonData.begin(), jsonData.end());
        localParticipant->publishData(data, true, {}, "chat");
        
        // Emit local message
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
    return participants_.values();
}

int ConferenceManager::getParticipantCount() const
{
    return participants_.size() + 1; // +1 for local participant
}

// Private methods - Queued slot implementations

void ConferenceManager::onParticipantConnectedQueued(QString identity, QString sid, QString name)
{
    ParticipantInfo info;
    info.identity = identity;
    info.sid = sid;
    info.name = name;
    info.isMicrophoneEnabled = false;
    info.isCameraEnabled = false;
    info.isScreenSharing = false;
    
    participants_[identity] = info;
    
    Logger::instance().info("Participant joined: " + name);
    emit participantJoined(info);
}

void ConferenceManager::onParticipantDisconnectedQueued(QString identity, int reason)
{
    Q_UNUSED(reason);
    participants_.remove(identity);
    
    Logger::instance().info("Participant left: " + identity);
    emit participantLeft(identity);
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
    
    trackSources_[trackSid] = info.source;
    trackKinds_[trackSid] = info.kind;
    
    // Fallback: detect screenshare by name if source unknown
    if (info.kind == livekit::TrackKind::KIND_VIDEO && 
        (info.source == livekit::TrackSource::SOURCE_UNKNOWN || info.source == livekit::TrackSource::SOURCE_CAMERA)) {
        QString trackName = track ? QString::fromStdString(track->name()).toLower() : "";
        if (trackName.contains("screen") || trackName.contains("share")) {
            trackSources_[trackSid] = livekit::TrackSource::SOURCE_SCREENSHARE;
            info.source = livekit::TrackSource::SOURCE_SCREENSHARE;
        }
    }
    
    bool isScreenShare = (trackSources_[trackSid] == livekit::TrackSource::SOURCE_SCREENSHARE ||
                          trackSources_[trackSid] == livekit::TrackSource::SOURCE_SCREENSHARE_AUDIO);
    
    QString kindStr = (info.kind == livekit::TrackKind::KIND_AUDIO) ? "audio" : "video";
    Logger::instance().info(QString("Track subscribed: %1 from %2")
                           .arg(kindStr, participantIdentity));

    // Start media stream for remote tracks using pull-based model
    if (info.kind == livekit::TrackKind::KIND_VIDEO && track) {
        try {
            if (isScreenShare) {
                screenShareActive_[participantIdentity] = true;
            }
            
            // Create VideoStream using new SDK API
            livekit::VideoStream::Options videoOptions;
            auto videoStream = livekit::VideoStream::fromTrack(track, videoOptions);
            videoStreams_[trackSid] = videoStream;
            
            // Start reader thread for this stream
            startVideoStreamReader(trackSid, participantIdentity, videoStream);
            
            // Emit initial muted state
            QTimer::singleShot(100, this, [this, trackSid, identity = participantIdentity, kind = info.kind, muted]() {
                emit trackMutedStateChanged(trackSid, identity, kind, muted);
            });
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Failed to create video stream: %1").arg(e.what()));
        }
    } else if (info.kind == livekit::TrackKind::KIND_AUDIO && track) {
        try {
            // Create AudioStream using new SDK API
            livekit::AudioStream::Options audioOptions;
            auto audioStream = livekit::AudioStream::fromTrack(track, audioOptions);
            audioStreams_[trackSid] = audioStream;
            
            // Start reader thread for this stream
            startAudioStreamReader(trackSid, participantIdentity, audioStream);

            // Emit initial muted state
            QTimer::singleShot(100, this, [this, trackSid, identity = participantIdentity, kind = info.kind, muted]() {
                emit trackMutedStateChanged(trackSid, identity, kind, muted);
            });
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Failed to create audio stream: %1").arg(e.what()));
        }
    }

    emit trackSubscribed(info);
    updateParticipantInfo(participantIdentity);
}

void ConferenceManager::onTrackUnsubscribedQueued(QString trackSid, QString participantIdentity)
{
    Logger::instance().info(QString("Track unsubscribed: %1 from %2")
                           .arg(trackSid, participantIdentity));
    
    // Stop stream reader threads
    stopStreamReaders(trackSid);
    
    emit trackUnsubscribed(trackSid, participantIdentity);
    
    // Consider the participant muted when track is removed
    livekit::TrackKind kind = trackKinds_.value(trackSid, livekit::TrackKind::KIND_AUDIO);
    emit trackMutedStateChanged(trackSid, participantIdentity, kind, true);
    
    trackSources_.remove(trackSid);
    trackKinds_.remove(trackSid);
    
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
    if (trackSources_.contains(trackSid)) {
        if (trackSources_[trackSid] == livekit::TrackSource::SOURCE_SCREENSHARE) {
            screenShareActive_[participantIdentity] = false;
        }
        trackSources_.remove(trackSid);
    }
    
    updateParticipantInfo(participantIdentity);
}

void ConferenceManager::onTrackMutedQueued(QString trackSid, QString participantIdentity, int kind)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    QString kindStr = (trackKind == livekit::TrackKind::KIND_AUDIO) ? "AUDIO" : "VIDEO";
    Logger::instance().info(QString("Track muted: sid=%1, identity=%2, kind=%3")
        .arg(trackSid).arg(participantIdentity).arg(kindStr));
    
    trackKinds_[trackSid] = trackKind;
    emit trackMutedStateChanged(trackSid, participantIdentity, trackKind, true);
}

void ConferenceManager::onTrackUnmutedQueued(QString trackSid, QString participantIdentity, int kind)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    QString kindStr = (trackKind == livekit::TrackKind::KIND_AUDIO) ? "AUDIO" : "VIDEO";
    Logger::instance().info(QString("Track unmuted: sid=%1, identity=%2, kind=%3")
        .arg(trackSid).arg(participantIdentity).arg(kindStr));
    
    trackKinds_[trackSid] = trackKind;
    emit trackMutedStateChanged(trackSid, participantIdentity, trackKind, false);
}

void ConferenceManager::onTrackUnpublishedQueued(QString trackSid, QString participantIdentity, int kind, int source)
{
    livekit::TrackKind trackKind = static_cast<livekit::TrackKind>(kind);
    livekit::TrackSource trackSource = static_cast<livekit::TrackSource>(source);
    
    Logger::instance().info(QString("Track unpublished: sid=%1, identity=%2, kind=%3, source=%4")
        .arg(trackSid, participantIdentity)
        .arg(kind).arg(source));
    
    // Emit signal to ConferenceBackend for proper state clearing
    emit trackUnpublished(trackSid, participantIdentity, trackKind, trackSource);
    
    // Clean up track sources
    if (trackSources_.contains(trackSid)) {
        trackSources_.remove(trackSid);
    }
    if (trackKinds_.contains(trackSid)) {
        trackKinds_.remove(trackSid);
    }
}

void ConferenceManager::onConnectionStateChangedQueued(int state)
{
    livekit::ConnectionState connState = static_cast<livekit::ConnectionState>(state);
    Logger::instance().info(QString("Connection state changed: %1").arg(state));
    
    if (connState == livekit::ConnectionState::Connected) {
        connected_ = true;
        
        // Get room info
        const auto roomInfo = room_->room_info();
        roomName_ = QString::fromStdString(roomInfo.name);
        
        // Get local participant
        auto localParticipant = room_->localParticipant();
        if (localParticipant) {
            participantName_ = QString::fromStdString(localParticipant->name());
        }
        
        // Get existing remote participants
        auto remoteParticipants = room_->remoteParticipants();
        for (const auto& participant : remoteParticipants) {
            ParticipantInfo info;
            info.identity = QString::fromStdString(participant->identity());
            info.sid = QString::fromStdString(participant->sid());
            info.name = QString::fromStdString(participant->name());
            info.isMicrophoneEnabled = false;
            info.isCameraEnabled = false;
            info.isScreenSharing = false;
            
            participants_[info.identity] = info;
            emit participantJoined(info);
        }
        
        emit connected();
    } else if (connState == livekit::ConnectionState::Disconnected) {
        connected_ = false;
        participants_.clear();
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

void ConferenceManager::connectScreenSignals()
{
    // Avoid duplicate connections
    static QMetaObject::Connection screenConn;
    if (screenConn) {
        QObject::disconnect(screenConn);
    }
    screenConn = QObject::connect(screenCapturer_, &ScreenCapturer::frameCaptured,
                                  this, [this](const QImage& frame) {
                                      emit localScreenFrameReady(frame);
                                  });
}

void ConferenceManager::handleVideoFrame(const livekit::VideoFrameEvent& event,
                                         const QString& trackSid,
                                         const QString& participantIdentity)
{
    // VideoFrameEvent contains frame directly (not a pointer)
    const auto& frame = event.frame;
    if (frame.width() == 0 || frame.height() == 0) {
        return;
    }

    livekit::TrackSource source = livekit::TrackSource::SOURCE_UNKNOWN;
    if (trackSources_.contains(trackSid)) {
        source = trackSources_[trackSid];
    }
    if (source == livekit::TrackSource::SOURCE_SCREENSHARE) {
        screenShareActive_[participantIdentity] = true;
    }

    // Create QImage from frame data
    // The frame data is in RGBA format by default (set in VideoStream::Options)
    QImage image(
        frame.data(),
        frame.width(),
        frame.height(),
        QImage::Format_RGBA8888
    );
    
    // Make a deep copy since the frame data may be reused
    QImage imageCopy = image.copy();

    emit videoFrameReceived(participantIdentity, trackSid, imageCopy, source);
}

void ConferenceManager::handleAudioFrame(const livekit::AudioFrameEvent& event,
                                         const QString& trackSid,
                                         const QString& participantIdentity)
{
    // AudioFrameEvent contains frame directly (not a pointer)
    const auto& frame = event.frame;
    
    // Notify UI of audio activity
    emit audioActivity(participantIdentity, true);

    auto playbackIt = audioPlayers_.find(trackSid);
    if (playbackIt == audioPlayers_.end()) {
        playbackIt = audioPlayers_.insert(trackSid, AudioPlayback{});
    }

    AudioPlayback& playback = playbackIt.value();

    // New SDK uses sample_rate() and num_channels() instead of sampleRate() and channelCount()
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

    // Get audio data from the frame
    const auto& samples = frame.data();
    QByteArray data(reinterpret_cast<const char*>(samples.data()),
                    static_cast<int>(samples.size() * sizeof(int16_t)));
    playback.device->write(data);
}

void ConferenceManager::startVideoStreamReader(const QString& trackSid, 
                                               const QString& participantIdentity,
                                               std::shared_ptr<livekit::VideoStream> stream)
{
    // Create stop flag for this stream
    auto* stopFlag = new std::atomic<bool>(false);
    streamStopFlags_[trackSid] = stopFlag;
    
    // Create reader thread
    std::thread readerThread([this, trackSid, participantIdentity, stream, stopFlag]() {
        livekit::VideoFrameEvent event;
        while (!stopFlag->load()) {
            // read() returns false when stream ends or is closed
            if (!stream->read(event)) {
                break;
            }
            
            // VideoFrameEvent contains frame directly, queue to main thread for processing
            QMetaObject::invokeMethod(this, [this, event = std::move(event), trackSid, participantIdentity]() mutable {
                handleVideoFrame(event, trackSid, participantIdentity);
            }, Qt::QueuedConnection);
        }
    });
    
    videoStreamThreads_[trackSid] = std::make_unique<std::thread>(std::move(readerThread));
}

void ConferenceManager::startAudioStreamReader(const QString& trackSid, 
                                               const QString& participantIdentity,
                                               std::shared_ptr<livekit::AudioStream> stream)
{
    // Create stop flag for this stream
    auto* stopFlag = new std::atomic<bool>(false);
    streamStopFlags_[trackSid] = stopFlag;
    
    // Create reader thread
    std::thread readerThread([this, trackSid, participantIdentity, stream, stopFlag]() {
        livekit::AudioFrameEvent event;
        while (!stopFlag->load()) {
            // read() returns false when stream ends or is closed
            if (!stream->read(event)) {
                break;
            }
            
            // AudioFrameEvent contains frame directly, queue to main thread for processing
            QMetaObject::invokeMethod(this, [this, event = std::move(event), trackSid, participantIdentity]() mutable {
                handleAudioFrame(event, trackSid, participantIdentity);
            }, Qt::QueuedConnection);
        }
    });
    
    audioStreamThreads_[trackSid] = std::make_unique<std::thread>(std::move(readerThread));
}

void ConferenceManager::stopStreamReaders(const QString& trackSid)
{
    // Signal stop
    if (streamStopFlags_.contains(trackSid)) {
        streamStopFlags_[trackSid]->store(true);
    }
    
    // Join video reader thread
    if (videoStreamThreads_.count(trackSid) > 0) {
        auto& threadPtr = videoStreamThreads_[trackSid];
        if (threadPtr && threadPtr->joinable()) {
            threadPtr->join();
        }
        videoStreamThreads_.erase(trackSid);
    }
    
    // Join audio reader thread
    if (audioStreamThreads_.count(trackSid) > 0) {
        auto& threadPtr = audioStreamThreads_[trackSid];
        if (threadPtr && threadPtr->joinable()) {
            threadPtr->join();
        }
        audioStreamThreads_.erase(trackSid);
    }
    
    // Delete stop flag
    if (streamStopFlags_.contains(trackSid)) {
        delete streamStopFlags_[trackSid];
        streamStopFlags_.remove(trackSid);
    }
}

void ConferenceManager::updateParticipantInfo(const QString& identity)
{
    if (!participants_.contains(identity)) {
        return;
    }
    
    auto participant = room_->remoteParticipant(identity.toStdString());
    if (!participant) {
        emit participantUpdated(participants_[identity]);
        return;
    }

    ParticipantInfo updated = participants_[identity];
    updated.isMicrophoneEnabled = false;
    updated.isCameraEnabled = false;
    updated.isScreenSharing = false;

    // Note: The new SDK may have different track publication APIs
    // We'll update based on the track sources we've tracked
    for (auto it = trackSources_.begin(); it != trackSources_.end(); ++it) {
        QString trackSid = it.key();
        livekit::TrackSource source = it.value();
        
        // Check if this track belongs to this participant
        // (we'd need to track this mapping, for now just update based on tracked data)
        if (trackKinds_.contains(trackSid)) {
            livekit::TrackKind kind = trackKinds_[trackSid];
            if (kind == livekit::TrackKind::KIND_AUDIO && source == livekit::TrackSource::SOURCE_MICROPHONE) {
                updated.isMicrophoneEnabled = true;
            }
            if (kind == livekit::TrackKind::KIND_VIDEO && source == livekit::TrackSource::SOURCE_CAMERA) {
                updated.isCameraEnabled = true;
            }
            if (kind == livekit::TrackKind::KIND_VIDEO &&
                (source == livekit::TrackSource::SOURCE_SCREENSHARE || source == livekit::TrackSource::SOURCE_SCREENSHARE_AUDIO)) {
                updated.isScreenSharing = true;
            }
        }
    }

    participants_[identity] = updated;
    emit participantUpdated(updated);
}

