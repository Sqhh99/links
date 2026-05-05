#include "device_controller.h"
#include "../../utils/logger.h"
#include "../../utils/settings.h"
#include "livekit/local_audio_track.h"
#include "livekit/local_participant.h"
#include "livekit/local_track_publication.h"
#include "livekit/local_video_track.h"

namespace {

std::shared_ptr<livekit::LocalTrackPublication>
resolveTrackPublication(const std::shared_ptr<livekit::Track>& track)
{
    if (!track) {
        return nullptr;
    }

    if (auto localVideoTrack = std::dynamic_pointer_cast<livekit::LocalVideoTrack>(track)) {
        return localVideoTrack->publication();
    }

    if (auto localAudioTrack = std::dynamic_pointer_cast<livekit::LocalAudioTrack>(track)) {
        return localAudioTrack->publication();
    }

    return nullptr;
}

std::string resolvePublishedTrackSid(livekit::LocalParticipant* localParticipant,
                                     const std::shared_ptr<livekit::Track>& track)
{
    if (!track) {
        return {};
    }

    if (const auto publication = resolveTrackPublication(track)) {
        if (!publication->sid().empty()) {
            return publication->sid();
        }
    }

    if (!localParticipant) {
        return {};
    }

    const auto publications = localParticipant->trackPublications();
    for (const auto& entry : publications) {
        const auto& publication = entry.second;
        if (!publication) {
            continue;
        }

        if (publication->track() == track) {
            return publication->sid();
        }
    }

    return {};
}

bool isTrackNotFoundError(const std::exception& e)
{
    return QString::fromUtf8(e.what()).contains("track not found", Qt::CaseInsensitive);
}

bool unpublishLocalTrack(livekit::LocalParticipant* localParticipant,
                         const std::shared_ptr<livekit::Track>& track,
                         std::string* cachedPublicationSid,
                         const QString& label,
                         bool suppressTrackNotFound)
{
    if (!localParticipant || !track) {
        return false;
    }

    const std::string publicationSid = (cachedPublicationSid && !cachedPublicationSid->empty())
        ? *cachedPublicationSid
        : resolvePublishedTrackSid(localParticipant, track);
    if (publicationSid.empty()) {
        Logger::instance().warning(QString("Skipping unpublish for %1: publication SID is unavailable")
                                   .arg(label));
        track->setPublication(nullptr);
        if (cachedPublicationSid) {
            cachedPublicationSid->clear();
        }
        return false;
    }

    try {
        Logger::instance().info(QString("Unpublishing %1 (publication SID: %2)")
                                .arg(label, QString::fromStdString(publicationSid)));
        localParticipant->unpublishTrack(publicationSid);
        track->setPublication(nullptr);
        if (cachedPublicationSid) {
            cachedPublicationSid->clear();
        }
        return true;
    } catch (const std::exception& e) {
        if (suppressTrackNotFound && isTrackNotFoundError(e)) {
            Logger::instance().warning(QString("Suppressing missing-publication error while unpublishing %1: %2")
                                       .arg(label, QString::fromUtf8(e.what())));
            track->setPublication(nullptr);
            if (cachedPublicationSid) {
                cachedPublicationSid->clear();
            }
            return false;
        }
        throw;
    }
}

} // namespace

DeviceController::DeviceController(livekit::Room* room, QObject* parent)
    : QObject(parent),
      room_(room),
      cameraCapturer_(new CameraCapturer(this)),
      microphoneCapturer_(new MicrophoneCapturer(this)),
      screenCapturer_(new ScreenCapturer(this))
{
    auto& settings = Settings::instance();

    const QString cameraId = settings.getSelectedCameraId();
    if (!cameraId.isEmpty()) {
        if (!cameraCapturer_->setCameraById(cameraId.toUtf8())) {
            const QList<QCameraDevice> cameras = CameraCapturer::availableCameras();
            QString fallbackId;
            if (!cameras.isEmpty()) {
                cameraCapturer_->setCamera(cameras.first());
                fallbackId = QString::fromUtf8(cameras.first().id());
            }

            settings.setSelectedCameraId(fallbackId);
            settings.sync();
            Logger::instance().info(QString("Recovered stale camera id, fallback camera id: %1")
                                        .arg(fallbackId.isEmpty() ? QStringLiteral("<none>") : fallbackId));
        }
    }

    const QString micId = settings.getSelectedMicrophoneId();
    if (!micId.isEmpty()) {
        microphoneCapturer_->setDeviceById(micId.toUtf8());
    }
    
    // Apply audio processing options from settings
    microphoneCapturer_->setEchoCancellationEnabled(settings.isEchoCancellationEnabled());
    microphoneCapturer_->setNoiseSuppressionEnabled(settings.isNoiseSuppressionEnabled());
    microphoneCapturer_->setAutoGainControlEnabled(settings.isAutoGainControlEnabled());
    microphoneCapturer_->setHighPassFilterEnabled(settings.isHighPassFilterEnabled());
    
    // Apply advanced audio processing settings
    microphoneCapturer_->setNoiseSuppressionLevel(
        static_cast<AudioProcessingModule::NoiseSuppressionLevel>(settings.noiseSuppressionLevel()));
    microphoneCapturer_->setGainControlMode(
        static_cast<AudioProcessingModule::GainControlMode>(settings.gainControlMode()));
    microphoneCapturer_->setFixedDigitalGainDb(settings.fixedDigitalGainDb());
    microphoneCapturer_->setAdaptiveDigitalMaxGainDb(settings.adaptiveDigitalMaxGainDb());
    microphoneCapturer_->setEchoEnhancedFilterEnabled(settings.isEchoEnhancedFilterEnabled());

    QObject::connect(cameraCapturer_, &CameraCapturer::error, this, [](const QString& msg) {
        Logger::instance().error(QString("Camera error: %1").arg(msg));
    });
    QObject::connect(cameraCapturer_, &CameraCapturer::frameCaptured, this,
                     [this](const QImage& frame) {
                         emit localVideoFrameReady(frame);
                     });

    QObject::connect(microphoneCapturer_, &MicrophoneCapturer::error, this, [](const QString& msg) {
        Logger::instance().error(QString("Microphone error: %1").arg(msg));
    });

    QObject::connect(screenCapturer_, &ScreenCapturer::error, this, [this](const QString& msg) {
        Logger::instance().error(QString("Screen capture error: %1").arg(msg));
        if (screenShareEnabled_) {
            screenCapturer_->stop();
            auto localParticipant = room_ ? room_->localParticipant() : nullptr;
            if (localParticipant && localScreenTrack_) {
                try {
                    unpublishLocalTrack(localParticipant, localScreenTrack_, &screenTrackSid_,
                                        QStringLiteral("screen share track"), true);
                } catch (const std::exception& e) {
                    Logger::instance().warning(QString("Failed to unpublish screen share track after capture error: %1")
                                               .arg(e.what()));
                }
            }
            localScreenTrack_ = nullptr;
            screenTrackSid_.clear();
            screenShareEnabled_ = false;
            emit localScreenShareChanged(false);
        }
    });
}

void DeviceController::setRoom(livekit::Room* room)
{
    room_ = room;
}

void DeviceController::stopCapturers()
{
    if (cameraCapturer_) {
        cameraCapturer_->stop();
    }
    if (microphoneCapturer_) {
        microphoneCapturer_->stop();
    }
    if (screenCapturer_) {
        screenCapturer_->stop();
    }
}

void DeviceController::unpublishLocalTracks()
{
    if (!room_) {
        return;
    }

    auto localParticipant = room_->localParticipant();
    if (!localParticipant) {
        return;
    }

    if (localAudioTrack_) {
        try {
            unpublishLocalTrack(localParticipant, localAudioTrack_, nullptr,
                                QStringLiteral("audio track"), true);
        } catch (const std::exception& e) {
            Logger::instance().warning(QString("Failed to unpublish audio track during disconnect: %1")
                                       .arg(e.what()));
        }
    }

    if (localVideoTrack_) {
        try {
            unpublishLocalTrack(localParticipant, localVideoTrack_, &cameraTrackSid_,
                                QStringLiteral("camera track"), true);
        } catch (const std::exception& e) {
            Logger::instance().warning(QString("Failed to unpublish camera track during disconnect: %1")
                                       .arg(e.what()));
        }
    }

    if (localScreenTrack_) {
        try {
            unpublishLocalTrack(localParticipant, localScreenTrack_, &screenTrackSid_,
                                QStringLiteral("screen share track"), true);
        } catch (const std::exception& e) {
            Logger::instance().warning(QString("Failed to unpublish screen share track during disconnect: %1")
                                       .arg(e.what()));
        }
    }
}

void DeviceController::resetLocalState()
{
    localVideoTrack_ = nullptr;
    localAudioTrack_ = nullptr;
    localScreenTrack_ = nullptr;
    cameraTrackSid_.clear();
    screenTrackSid_.clear();
    cameraEnabled_ = false;
    microphoneEnabled_ = false;
    screenShareEnabled_ = false;
}

void DeviceController::toggleMicrophone()
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
                    // Always create a new audio track since AudioSource is recreated each time
                    Logger::instance().info("Creating audio track...");
                    localAudioTrack_ = livekit::LocalAudioTrack::createLocalAudioTrack("mic", source);
                    Logger::instance().info(QString("Audio track created: %1")
                        .arg(localAudioTrack_ ? "valid" : "null"));

                    auto localParticipant = room_->localParticipant();
                    Logger::instance().info(QString("Got local participant: %1")
                        .arg(localParticipant ? "valid" : "null"));

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
                unpublishLocalTrack(localParticipant, localAudioTrack_, nullptr,
                                    QStringLiteral("audio track"), true);
                localAudioTrack_ = nullptr;
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in toggleMicrophone: %1").arg(e.what()));
        microphoneEnabled_ = false;
    }

    emit localMicrophoneChanged(microphoneEnabled_);
}

void DeviceController::toggleCamera()
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
                        Logger::instance().info(QString("Video track created: %1")
                            .arg(localVideoTrack_ ? "valid" : "null"));
                    }

                    auto localParticipant = room_->localParticipant();
                    Logger::instance().info(QString("Got local participant: %1")
                        .arg(localParticipant ? "valid" : "null"));

                    if (localParticipant && localVideoTrack_) {
                        Logger::instance().info("Publishing video track...");
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_CAMERA;
                        localParticipant->publishTrack(localVideoTrack_, options);
                        cameraTrackSid_ = resolvePublishedTrackSid(localParticipant, localVideoTrack_);
                        if (!cameraTrackSid_.empty()) {
                            Logger::instance().info(QString("Video track published with SID: %1")
                                .arg(QString::fromStdString(cameraTrackSid_)));
                        } else {
                            Logger::instance().warning("Video track published but SID is not available yet");
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
            if (localParticipant && localVideoTrack_) {
                unpublishLocalTrack(localParticipant, localVideoTrack_, &cameraTrackSid_,
                                    QStringLiteral("camera track"), true);
                localVideoTrack_ = nullptr;
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in toggleCamera: %1").arg(e.what()));
        cameraEnabled_ = false;
    }

    emit localCameraChanged(cameraEnabled_);
}

void DeviceController::toggleScreenShare()
{
    if (screenShareDebounceTimer_.isValid()
        && screenShareDebounceTimer_.elapsed() < kScreenShareDebounceMs) {
        Logger::instance().warning("Screen share toggle debounced, ignoring rapid toggle");
        return;
    }
    screenShareDebounceTimer_.start();

    screenShareEnabled_ = !screenShareEnabled_;
    Logger::instance().info(QString("Screen sharing toggled: %1")
        .arg(screenShareEnabled_ ? "ON" : "OFF"));

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
                    localParticipant->publishTrack(localScreenTrack_, options);
                    screenTrackSid_ = resolvePublishedTrackSid(localParticipant, localScreenTrack_);
                    if (!screenTrackSid_.empty()) {
                        Logger::instance().info(QString("Screen share track published with SID: %1")
                            .arg(QString::fromStdString(screenTrackSid_)));
                    } else {
                        Logger::instance().warning("Screen share track published but SID is not available yet");
                    }
                }
            } else {
                Logger::instance().error("Failed to start screen sharing");
                screenShareEnabled_ = false;
            }
        } else {
            screenCapturer_->stop();

            auto localParticipant = room_->localParticipant();
            if (localParticipant && localScreenTrack_) {
                unpublishLocalTrack(localParticipant, localScreenTrack_, &screenTrackSid_,
                                    QStringLiteral("screen share track"), true);
                Logger::instance().info("Screen track unpublished, releasing reference");
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

void DeviceController::setScreenShareMode(ScreenCapturer::Mode mode, QScreen* screen, WId windowId)
{
    if (!screenCapturer_) {
        return;
    }
    screenCapturer_->setMode(mode);
    if (mode == ScreenCapturer::Mode::Screen && screen) {
        screenCapturer_->setScreen(screen);
    } else if (mode == ScreenCapturer::Mode::Window) {
        screenCapturer_->setWindow(windowId);
    }
}

void DeviceController::switchCamera(const QString& deviceId)
{
    Logger::instance().info(QString("Switching camera to device: %1").arg(deviceId));

    try {
        const bool wasEnabled = cameraEnabled_;

        if (cameraEnabled_) {
            cameraCapturer_->stop();

            auto localParticipant = room_->localParticipant();
            if (localParticipant && localVideoTrack_) {
                unpublishLocalTrack(localParticipant, localVideoTrack_, &cameraTrackSid_,
                                    QStringLiteral("camera track"), true);
                localVideoTrack_ = nullptr;
            }
        }

        const bool switched = cameraCapturer_->setCameraById(deviceId.toUtf8());
        if (!switched) {
            Logger::instance().warning(QString("Requested camera device not found: %1").arg(deviceId));
        }

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
                        cameraTrackSid_ = resolvePublishedTrackSid(localParticipant, localVideoTrack_);
                        Logger::instance().info(QString("Camera switched and republished successfully%1")
                            .arg(cameraTrackSid_.empty()
                                ? QString()
                                : QStringLiteral(": ") + QString::fromStdString(cameraTrackSid_)));
                    }
                }
            } else {
                Logger::instance().error("Failed to restart camera with new device");
                cameraEnabled_ = false;
                emit localCameraChanged(cameraEnabled_);
            }
        }

        if (switched) {
            Settings::instance().setSelectedCameraId(deviceId);
        }
        Settings::instance().sync();

    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in switchCamera: %1").arg(e.what()));
    }
}

void DeviceController::switchMicrophone(const QString& deviceId)
{
    Logger::instance().info(QString("Switching microphone to device: %1").arg(deviceId));

    try {
        const bool wasEnabled = microphoneEnabled_;

        if (microphoneEnabled_) {
            microphoneCapturer_->stop();

            auto localParticipant = room_->localParticipant();
            if (localParticipant && localAudioTrack_) {
                unpublishLocalTrack(localParticipant, localAudioTrack_, nullptr,
                                    QStringLiteral("audio track"), true);
                localAudioTrack_ = nullptr;
            }
        }

        microphoneCapturer_->setDeviceById(deviceId.toUtf8());

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

        Settings::instance().setSelectedMicrophoneId(deviceId);
        Settings::instance().sync();

    } catch (const std::exception& e) {
        Logger::instance().error(QString("Exception in switchMicrophone: %1").arg(e.what()));
    }
}

void DeviceController::connectScreenSignals()
{
    static QMetaObject::Connection screenConn;
    if (screenConn) {
        QObject::disconnect(screenConn);
    }
    screenConn = QObject::connect(screenCapturer_, &ScreenCapturer::frameCaptured,
                                  this, [this](const QImage& frame) {
                                      emit localScreenFrameReady(frame);
                                  });
}

// =============================================================================
// Audio processing settings (runtime-applicable)
// =============================================================================

void DeviceController::applyAudioSettings()
{
    if (!microphoneCapturer_) return;

    auto& settings = Settings::instance();
    
    // Basic toggles
    microphoneCapturer_->setEchoCancellationEnabled(settings.isEchoCancellationEnabled());
    microphoneCapturer_->setNoiseSuppressionEnabled(settings.isNoiseSuppressionEnabled());
    microphoneCapturer_->setAutoGainControlEnabled(settings.isAutoGainControlEnabled());
    microphoneCapturer_->setHighPassFilterEnabled(settings.isHighPassFilterEnabled());
    
    // Advanced parameters
    microphoneCapturer_->setNoiseSuppressionLevel(
        static_cast<AudioProcessingModule::NoiseSuppressionLevel>(settings.noiseSuppressionLevel()));
    microphoneCapturer_->setGainControlMode(
        static_cast<AudioProcessingModule::GainControlMode>(settings.gainControlMode()));
    microphoneCapturer_->setFixedDigitalGainDb(settings.fixedDigitalGainDb());
    microphoneCapturer_->setAdaptiveDigitalMaxGainDb(settings.adaptiveDigitalMaxGainDb());
    microphoneCapturer_->setEchoEnhancedFilterEnabled(settings.isEchoEnhancedFilterEnabled());

    Logger::instance().info(QString("Audio settings re-applied (AEC=%1, NS=%2[lvl=%3], AGC=%4[mode=%5], HPF=%6, AEC-enhanced=%7)")
                           .arg(settings.isEchoCancellationEnabled())
                           .arg(settings.isNoiseSuppressionEnabled())
                           .arg(settings.noiseSuppressionLevel())
                           .arg(settings.isAutoGainControlEnabled())
                           .arg(settings.gainControlMode())
                           .arg(settings.isHighPassFilterEnabled())
                           .arg(settings.isEchoEnhancedFilterEnabled()));
}

void DeviceController::setEchoCancellationEnabled(bool enabled)
{
    if (microphoneCapturer_) microphoneCapturer_->setEchoCancellationEnabled(enabled);
}

void DeviceController::setNoiseSuppressionEnabled(bool enabled)
{
    if (microphoneCapturer_) microphoneCapturer_->setNoiseSuppressionEnabled(enabled);
}

void DeviceController::setAutoGainControlEnabled(bool enabled)
{
    if (microphoneCapturer_) microphoneCapturer_->setAutoGainControlEnabled(enabled);
}

void DeviceController::setHighPassFilterEnabled(bool enabled)
{
    if (microphoneCapturer_) microphoneCapturer_->setHighPassFilterEnabled(enabled);
}

void DeviceController::setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel level)
{
    if (microphoneCapturer_) microphoneCapturer_->setNoiseSuppressionLevel(level);
}

void DeviceController::setGainControlMode(AudioProcessingModule::GainControlMode mode)
{
    if (microphoneCapturer_) microphoneCapturer_->setGainControlMode(mode);
}

void DeviceController::setFixedDigitalGainDb(float gainDb)
{
    if (microphoneCapturer_) microphoneCapturer_->setFixedDigitalGainDb(gainDb);
}

void DeviceController::setAdaptiveDigitalMaxGainDb(float maxGainDb)
{
    if (microphoneCapturer_) microphoneCapturer_->setAdaptiveDigitalMaxGainDb(maxGainDb);
}

void DeviceController::setEchoEnhancedFilterEnabled(bool enabled)
{
    if (microphoneCapturer_) microphoneCapturer_->setEchoEnhancedFilterEnabled(enabled);
}

void DeviceController::feedReverseAudio(const int16_t* data, int samples,
                                         int sampleRate, int channels)
{
    if (microphoneCapturer_ && microphoneCapturer_->isActive()) {
        microphoneCapturer_->feedReverseStream(data, samples, sampleRate, channels);
    }
}
