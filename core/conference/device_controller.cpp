#include "device_controller.h"
#include "../../utils/logger.h"
#include "../../utils/settings.h"
#include "livekit/local_audio_track.h"
#include "livekit/local_video_track.h"

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
        cameraCapturer_->setCameraById(cameraId.toUtf8());
    }

    const QString micId = settings.getSelectedMicrophoneId();
    if (!micId.isEmpty()) {
        microphoneCapturer_->setDeviceById(micId.toUtf8());
    }
    
    // Apply audio processing options from settings
    microphoneCapturer_->setEchoCancellationEnabled(settings.isEchoCancellationEnabled());
    microphoneCapturer_->setNoiseSuppressionEnabled(settings.isNoiseSuppressionEnabled());
    microphoneCapturer_->setAutoGainControlEnabled(settings.isAutoGainControlEnabled());

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
            auto localParticipant = room_->localParticipant();
            if (localParticipant && localScreenTrack_) {
                localParticipant->unpublishTrack(localScreenTrack_->sid());
                localScreenTrack_ = nullptr;
            }
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

    if (localAudioTrack_ && !localAudioTrack_->sid().empty()) {
        Logger::instance().info("Unpublishing audio track");
        localParticipant->unpublishTrack(localAudioTrack_->sid());
    }

    if (!cameraTrackSid_.empty()) {
        Logger::instance().info("Unpublishing camera track");
        localParticipant->unpublishTrack(cameraTrackSid_);
    }

    if (!screenTrackSid_.empty()) {
        Logger::instance().info("Unpublishing screen share track");
        localParticipant->unpublishTrack(screenTrackSid_);
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
                localParticipant->unpublishTrack(localVideoTrack_->sid());
                localVideoTrack_ = nullptr;
            }
        }

        cameraCapturer_->setCameraById(deviceId.toUtf8());

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

        Settings::instance().setSelectedCameraId(deviceId);
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
                localParticipant->unpublishTrack(localAudioTrack_->sid());
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
