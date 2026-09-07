#include "device_controller.h"

#include "../base/log.h"
#include "../base/strings.h"
#include "livekit/local_audio_track.h"
#include "livekit/local_participant.h"
#include "livekit/local_track_publication.h"
#include "livekit/local_video_track.h"

namespace core = links::core;

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

std::string resolvePublishedTrackSid(const std::shared_ptr<livekit::LocalParticipant>& localParticipant,
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
    return core::str::containsIgnoreCase(e.what() ? e.what() : "", "track not found");
}

enum class UnpublishOutcome {
    Unpublished,
    PublicationUnavailable,
    AlreadyGone,
    NoTrack,
    NoParticipant,
};

UnpublishOutcome unpublishLocalTrack(const std::shared_ptr<livekit::LocalParticipant>& localParticipant,
                                     const std::shared_ptr<livekit::Track>& track,
                                     std::string* cachedPublicationSid,
                                     const std::string& label,
                                     bool suppressTrackNotFound)
{
    if (!track) {
        return UnpublishOutcome::NoTrack;
    }

    if (!localParticipant) {
        return UnpublishOutcome::NoParticipant;
    }

    const std::string publicationSid = (cachedPublicationSid && !cachedPublicationSid->empty())
        ? *cachedPublicationSid
        : resolvePublishedTrackSid(localParticipant, track);
    if (publicationSid.empty()) {
        return UnpublishOutcome::PublicationUnavailable;
    }

    try {
        core::logInfo(core::str::cat("Unpublishing ", label, " (publication SID: %2)"));
        localParticipant->unpublishTrack(publicationSid);
        track->setPublication(nullptr);
        if (cachedPublicationSid) {
            cachedPublicationSid->clear();
        }
        return UnpublishOutcome::Unpublished;
    } catch (const std::exception& e) {
        if (suppressTrackNotFound && isTrackNotFoundError(e)) {
            core::logWarning(core::str::cat("Suppressing missing-publication error while unpublishing ", label, ": %2"));
            track->setPublication(nullptr);
            if (cachedPublicationSid) {
                cachedPublicationSid->clear();
            }
            return UnpublishOutcome::AlreadyGone;
        }
        throw;
    }
}

} // namespace

DeviceController::DeviceController(livekit::Room* room,
                                   const core::PlatformServices& services,
                                   const core::DeviceSelection& devices,
                                   const core::AudioProcessingConfig& audio)
    : services_(services),
      room_(room),
      cameraCapturer_(std::make_unique<CameraCapturer>(*services.camera)),
      microphoneCapturer_(std::make_unique<MicrophoneCapturer>(*services.microphone)),
      screenCapturer_(std::make_unique<ScreenCapturer>(*services.timers))
{
    pendingUnpublishRetryTimer_ =
        services_.timers->createTimer([this]() { processPendingUnpublish(); });

    if (!devices.cameraId.empty()) {
        if (!cameraCapturer_->setCameraById(devices.cameraId)) {
            // The persisted id is stale. Fall back to the first camera and tell
            // the caller to persist the correction (core no longer writes
            // settings itself).
            std::string fallbackId;
            const auto cameras = services_.devices->cameras();
            if (!cameras.empty()) {
                fallbackId = cameras.front().id;
                cameraCapturer_->setCameraById(fallbackId);
            }

            preferredCameraChanged.notify(fallbackId);
            core::logInfo(core::str::cat("Recovered stale camera id, fallback camera id: ",
                                         fallbackId.empty() ? "<none>" : fallbackId.c_str()));
        }
    }

    if (!devices.microphoneId.empty()) {
        microphoneCapturer_->setDeviceById(devices.microphoneId);
    }

    applyAudioSettings(audio);

    capturerConnections_ += cameraCapturer_->error.connect([](const std::string& msg) {
        core::logError(core::str::cat("Camera error: ", msg));
    });
    capturerConnections_ += cameraCapturer_->frameCaptured.connect(
        [this](const core::VideoFrame& frame) {
            localVideoFrameReady.notify(frame);
        });

    capturerConnections_ += microphoneCapturer_->error.connect([](const std::string& msg) {
        core::logError(core::str::cat("Microphone error: ", msg));
    });

    capturerConnections_ += screenCapturer_->error.connect([this](const std::string& msg) {
        core::logError(core::str::cat("Screen capture error: ", msg));
        if (screenShareEnabled_) {
            screenCapturer_->stop();
            auto localParticipant = this->localParticipant();
            if (localParticipant && localScreenTrack_) {
                try {
                    const UnpublishOutcome outcome =
                        unpublishLocalTrack(localParticipant, localScreenTrack_, &screenTrackSid_,
                                            "screen share track", true);
                    if (outcome == UnpublishOutcome::Unpublished
                        || outcome == UnpublishOutcome::AlreadyGone
                        || outcome == UnpublishOutcome::NoParticipant
                        || outcome == UnpublishOutcome::NoTrack) {
                        finalizeScreenShareDisabled();
                        return;
                    }
                } catch (const std::exception& e) {
                    core::logWarning(core::str::cat(
                        "Failed to unpublish screen share track after capture error: ", e.what()));
                }
            }
            pendingDisableScreenShare_ = true;
            pendingDisableScreenShareLogged_ = true;
            screenShareEnabled_ = false;
            localScreenShareChanged.notify(false);
            schedulePendingUnpublishRetry();
            core::logWarning("Deferring screen share shutdown until publication SID becomes available");
        }
    });
}

DeviceController::~DeviceController()
{
    // First, so no capturer callback can run against half-destroyed members.
    capturerConnections_.clear();
}

std::shared_ptr<livekit::LocalParticipant> DeviceController::localParticipant() const
{
    return room_ ? room_->localParticipant().lock() : nullptr;
}

void DeviceController::setRoom(livekit::Room* room)
{
    room_ = room;
    if (!room_) {
        pendingUnpublishRetryTimer_->stop();
    }
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

    auto localParticipant = this->localParticipant();
    if (!localParticipant) {
        return;
    }

    if (localAudioTrack_) {
        try {
            unpublishLocalTrack(localParticipant, localAudioTrack_, &audioTrackSid_,
                                "audio track", true);
        } catch (const std::exception& e) {
            core::logWarning(core::str::cat("Failed to unpublish audio track during disconnect: ", e.what()));
        }
    }

    if (localVideoTrack_) {
        try {
            unpublishLocalTrack(localParticipant, localVideoTrack_, &cameraTrackSid_,
                                "camera track", true);
        } catch (const std::exception& e) {
            core::logWarning(core::str::cat("Failed to unpublish camera track during disconnect: ", e.what()));
        }
    }

    if (localScreenTrack_) {
        try {
            unpublishLocalTrack(localParticipant, localScreenTrack_, &screenTrackSid_,
                                "screen share track", true);
        } catch (const std::exception& e) {
            core::logWarning(core::str::cat("Failed to unpublish screen share track during disconnect: ", e.what()));
        }
    }
}

void DeviceController::resetLocalState()
{
    pendingUnpublishRetryTimer_->stop();
    localVideoTrack_ = nullptr;
    localAudioTrack_ = nullptr;
    localScreenTrack_ = nullptr;
    audioTrackSid_.clear();
    cameraTrackSid_.clear();
    screenTrackSid_.clear();
    cameraEnabled_ = false;
    microphoneEnabled_ = false;
    screenShareEnabled_ = false;
    pendingDisableCamera_ = false;
    pendingDisableMicrophone_ = false;
    pendingDisableScreenShare_ = false;
    pendingDisableCameraLogged_ = false;
    pendingDisableMicrophoneLogged_ = false;
    pendingDisableScreenShareLogged_ = false;
}

void DeviceController::handleLocalTrackPublished(livekit::TrackSource source, const std::string& publicationSid)
{
    const std::string sid = publicationSid;

    switch (source) {
    case livekit::TrackSource::SOURCE_MICROPHONE:
        audioTrackSid_ = sid;
        break;
    case livekit::TrackSource::SOURCE_CAMERA:
        cameraTrackSid_ = sid;
        break;
    case livekit::TrackSource::SOURCE_SCREENSHARE:
        screenTrackSid_ = sid;
        break;
    default:
        return;
    }

    processPendingUnpublish();
}

void DeviceController::schedulePendingUnpublishRetry()
{
    if (pendingDisableMicrophone_ || pendingDisableCamera_ || pendingDisableScreenShare_) {
        if (!pendingUnpublishRetryTimer_->isActive()) {
            // 500 ms repeating, same cadence as the QTimer it replaces.
            pendingUnpublishRetryTimer_->start(std::chrono::milliseconds(500), /*repeat=*/true);
        }
    } else {
        pendingUnpublishRetryTimer_->stop();
    }
}

void DeviceController::processPendingUnpublish()
{
    auto localParticipant = this->localParticipant();
    if (!localParticipant) {
        schedulePendingUnpublishRetry();
        return;
    }

    if (pendingDisableMicrophone_ && localAudioTrack_) {
        try {
            const UnpublishOutcome outcome =
                unpublishLocalTrack(localParticipant, localAudioTrack_, &audioTrackSid_,
                                    "audio track", true);
            if (outcome == UnpublishOutcome::Unpublished
                || outcome == UnpublishOutcome::AlreadyGone
                || outcome == UnpublishOutcome::NoParticipant
                || outcome == UnpublishOutcome::NoTrack) {
                finalizeMicrophoneDisabled();
            }
        } catch (const std::exception& e) {
            core::logWarning(core::str::cat("Deferred microphone unpublish failed, will retry: ", e.what()));
        }
    }

    if (pendingDisableCamera_ && localVideoTrack_) {
        try {
            const UnpublishOutcome outcome =
                unpublishLocalTrack(localParticipant, localVideoTrack_, &cameraTrackSid_,
                                    "camera track", true);
            if (outcome == UnpublishOutcome::Unpublished
                || outcome == UnpublishOutcome::AlreadyGone
                || outcome == UnpublishOutcome::NoParticipant
                || outcome == UnpublishOutcome::NoTrack) {
                finalizeCameraDisabled();
            }
        } catch (const std::exception& e) {
            core::logWarning(core::str::cat("Deferred camera unpublish failed, will retry: ", e.what()));
        }
    }

    if (pendingDisableScreenShare_ && localScreenTrack_) {
        try {
            const UnpublishOutcome outcome =
                unpublishLocalTrack(localParticipant, localScreenTrack_, &screenTrackSid_,
                                    "screen share track", true);
            if (outcome == UnpublishOutcome::Unpublished
                || outcome == UnpublishOutcome::AlreadyGone
                || outcome == UnpublishOutcome::NoParticipant
                || outcome == UnpublishOutcome::NoTrack) {
                finalizeScreenShareDisabled();
            }
        } catch (const std::exception& e) {
            core::logWarning(core::str::cat("Deferred screen-share unpublish failed, will retry: ", e.what()));
        }
    }

    schedulePendingUnpublishRetry();
}

void DeviceController::finalizeMicrophoneDisabled()
{
    localAudioTrack_ = nullptr;
    audioTrackSid_.clear();
    pendingDisableMicrophone_ = false;
    pendingDisableMicrophoneLogged_ = false;
    schedulePendingUnpublishRetry();
}

void DeviceController::finalizeCameraDisabled()
{
    localVideoTrack_ = nullptr;
    cameraTrackSid_.clear();
    pendingDisableCamera_ = false;
    pendingDisableCameraLogged_ = false;
    schedulePendingUnpublishRetry();
}

void DeviceController::finalizeScreenShareDisabled()
{
    core::logInfo("Screen track unpublished, releasing reference");
    localScreenTrack_.reset();
    screenTrackSid_.clear();
    pendingDisableScreenShare_ = false;
    pendingDisableScreenShareLogged_ = false;
    core::logInfo("Screen track reference released");
    schedulePendingUnpublishRetry();
}

void DeviceController::toggleMicrophone()
{
    if (pendingDisableMicrophone_) {
        core::logWarning("Ignoring microphone toggle while disable is pending");
        return;
    }

    microphoneEnabled_ = !microphoneEnabled_;
    core::logInfo(core::str::cat("Microphone toggled: ", microphoneEnabled_ ? "ON" : "OFF"));

    try {
        if (microphoneEnabled_) {
            core::logInfo("Starting microphone capturer...");
            if (microphoneCapturer_->start()) {
                core::logInfo("Microphone capturer started successfully");
                auto source = microphoneCapturer_->getAudioSource();
                core::logInfo(core::str::cat("Got audio source: ", source ? "valid" : "null"));

                if (source) {
                    // Always create a new audio track since AudioSource is recreated each time
                    core::logInfo("Creating audio track...");
                    localAudioTrack_ = livekit::LocalAudioTrack::createLocalAudioTrack("mic", source);
                    core::logInfo(core::str::cat("Audio track created: ", localAudioTrack_ ? "valid" : "null"));

                    auto localParticipant = this->localParticipant();
                    core::logInfo(core::str::cat("Got local participant: ", localParticipant ? "valid" : "null"));

                    if (localParticipant && localAudioTrack_) {
                        core::logInfo("Publishing audio track...");
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_MICROPHONE;
                        audioTrackSid_.clear();
                        localParticipant->publishTrack(localAudioTrack_, options);
                        core::logInfo("Audio track published successfully");
                    }
                }
            } else {
                core::logError("Failed to start microphone");
                microphoneEnabled_ = false;
            }
        } else {
            microphoneCapturer_->stop();

            auto localParticipant = this->localParticipant();
            if (localParticipant && localAudioTrack_) {
                const UnpublishOutcome outcome =
                    unpublishLocalTrack(localParticipant, localAudioTrack_, &audioTrackSid_,
                                        "audio track", true);
                if (outcome == UnpublishOutcome::PublicationUnavailable) {
                    pendingDisableMicrophone_ = true;
                    if (!pendingDisableMicrophoneLogged_) {
                        core::logWarning("Deferring microphone shutdown until publication SID becomes available");
                        pendingDisableMicrophoneLogged_ = true;
                    }
                    schedulePendingUnpublishRetry();
                } else {
                    finalizeMicrophoneDisabled();
                }
            } else {
                finalizeMicrophoneDisabled();
            }
            microphoneEnabled_ = false;
            localMicrophoneChanged.notify(false);
            return;
        }
    } catch (const std::exception& e) {
        core::logError(core::str::cat("Exception in toggleMicrophone: ", e.what()));
        microphoneEnabled_ = false;
    }

    localMicrophoneChanged.notify(microphoneEnabled_);
}

void DeviceController::toggleCamera()
{
    if (pendingDisableCamera_) {
        core::logWarning("Ignoring camera toggle while disable is pending");
        return;
    }

    cameraEnabled_ = !cameraEnabled_;
    core::logInfo(core::str::cat("Camera toggled: ", cameraEnabled_ ? "ON" : "OFF"));

    try {
        if (cameraEnabled_) {
            core::logInfo("Starting camera capturer...");
            if (cameraCapturer_->start()) {
                core::logInfo("Camera capturer started successfully");
                auto source = cameraCapturer_->getVideoSource();
                core::logInfo(core::str::cat("Got video source: ", source ? "valid" : "null"));

                if (source) {
                    if (!localVideoTrack_) {
                        core::logInfo("Creating video track...");
                        localVideoTrack_ = livekit::LocalVideoTrack::createLocalVideoTrack("camera", source);
                        core::logInfo(core::str::cat("Video track created: ", localVideoTrack_ ? "valid" : "null"));
                    }

                    auto localParticipant = this->localParticipant();
                    core::logInfo(core::str::cat("Got local participant: ", localParticipant ? "valid" : "null"));

                    if (localParticipant && localVideoTrack_) {
                        core::logInfo("Publishing video track...");
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_CAMERA;
                        cameraTrackSid_.clear();
                        localParticipant->publishTrack(localVideoTrack_, options);
                        cameraTrackSid_ = resolvePublishedTrackSid(localParticipant, localVideoTrack_);
                        if (!cameraTrackSid_.empty()) {
                            core::logInfo(core::str::cat("Video track published with SID: ", cameraTrackSid_));
                        } else {
                            core::logWarning("Video track published but SID is not available yet");
                        }
                    }
                }
            } else {
                core::logError("Failed to start camera");
                cameraEnabled_ = false;
            }
        } else {
            cameraCapturer_->stop();

            auto localParticipant = this->localParticipant();
            if (localParticipant && localVideoTrack_) {
                const UnpublishOutcome outcome =
                    unpublishLocalTrack(localParticipant, localVideoTrack_, &cameraTrackSid_,
                                        "camera track", true);
                if (outcome == UnpublishOutcome::PublicationUnavailable) {
                    pendingDisableCamera_ = true;
                    if (!pendingDisableCameraLogged_) {
                        core::logWarning("Deferring camera shutdown until publication SID becomes available");
                        pendingDisableCameraLogged_ = true;
                    }
                    schedulePendingUnpublishRetry();
                } else {
                    finalizeCameraDisabled();
                }
            } else {
                finalizeCameraDisabled();
            }
            cameraEnabled_ = false;
            localCameraChanged.notify(false);
            return;
        }
    } catch (const std::exception& e) {
        core::logError(core::str::cat("Exception in toggleCamera: ", e.what()));
        cameraEnabled_ = false;
    }

    localCameraChanged.notify(cameraEnabled_);
}

void DeviceController::toggleScreenShare()
{
    if (pendingDisableScreenShare_) {
        core::logWarning("Ignoring screen share toggle while disable is pending");
        return;
    }

    if (screenShareDebounceAt_.has_value()
        && core::elapsedMs(*screenShareDebounceAt_) < kScreenShareDebounceMs) {
        core::logWarning("Screen share toggle debounced, ignoring rapid toggle");
        return;
    }
    screenShareDebounceAt_ = core::SteadyClock::now();

    screenShareEnabled_ = !screenShareEnabled_;
    core::logInfo(core::str::cat("Screen sharing toggled: ", screenShareEnabled_ ? "ON" : "OFF"));

    try {
        if (screenShareEnabled_) {
            core::logInfo("Starting screen capturer...");
            if (screenCapturer_->start()) {
                connectScreenSignals();
                auto source = screenCapturer_->getVideoSource();
                if (source) {
                    localScreenTrack_ = livekit::LocalVideoTrack::createLocalVideoTrack("screen", source);
                }

                auto localParticipant = this->localParticipant();
                if (localParticipant && localScreenTrack_) {
                    livekit::TrackPublishOptions options;
                    options.source = livekit::TrackSource::SOURCE_SCREENSHARE;
                    screenTrackSid_.clear();
                    localParticipant->publishTrack(localScreenTrack_, options);
                    screenTrackSid_ = resolvePublishedTrackSid(localParticipant, localScreenTrack_);
                    if (!screenTrackSid_.empty()) {
                        core::logInfo(core::str::cat("Screen share track published with SID: ", screenTrackSid_));
                    } else {
                        core::logWarning("Screen share track published but SID is not available yet");
                    }
                }
            } else {
                core::logError("Failed to start screen sharing");
                screenShareEnabled_ = false;
            }
        } else {
            screenCapturer_->stop();

            auto localParticipant = this->localParticipant();
            if (localParticipant && localScreenTrack_) {
                const UnpublishOutcome outcome =
                    unpublishLocalTrack(localParticipant, localScreenTrack_, &screenTrackSid_,
                                        "screen share track", true);
                if (outcome == UnpublishOutcome::PublicationUnavailable) {
                    pendingDisableScreenShare_ = true;
                    if (!pendingDisableScreenShareLogged_) {
                        core::logWarning("Deferring screen share shutdown until publication SID becomes available");
                        pendingDisableScreenShareLogged_ = true;
                    }
                    schedulePendingUnpublishRetry();
                } else {
                    finalizeScreenShareDisabled();
                }
            } else {
                finalizeScreenShareDisabled();
            }
            screenShareEnabled_ = false;
            localScreenShareChanged.notify(false);
            return;
        }
    } catch (const std::exception& e) {
        core::logError(core::str::cat("Exception in toggleScreenShare: ", e.what()));
        screenShareEnabled_ = false;
    }

    localScreenShareChanged.notify(screenShareEnabled_);
}

void DeviceController::setScreenShareMode(ScreenCapturer::Mode mode,
                                          core::MonitorId monitorId,
                                          core::WindowId windowId)
{
    if (!screenCapturer_) {
        return;
    }
    screenCapturer_->setMode(mode);
    if (mode == ScreenCapturer::Mode::Screen) {
        screenCapturer_->setScreen(monitorId);
    } else if (mode == ScreenCapturer::Mode::Window) {
        screenCapturer_->setWindow(windowId);
    }
}

void DeviceController::switchCamera(const std::string& deviceId)
{
    if (pendingDisableCamera_) {
        core::logWarning("Ignoring camera switch while camera shutdown is pending");
        return;
    }

    core::logInfo(core::str::cat("Switching camera to device: ", deviceId));

    try {
        const bool wasEnabled = cameraEnabled_;
        auto localParticipant = this->localParticipant();

        if (cameraEnabled_ && localVideoTrack_) {
            const UnpublishOutcome outcome =
                unpublishLocalTrack(localParticipant, localVideoTrack_, &cameraTrackSid_,
                                    "camera track", true);
            if (outcome == UnpublishOutcome::PublicationUnavailable) {
                core::logWarning("Deferring camera switch until current publication SID becomes available");
                return;
            }
            cameraCapturer_->stop();
            localVideoTrack_ = nullptr;
        }

        const bool switched = cameraCapturer_->setCameraById(deviceId);
        if (!switched) {
            core::logWarning(core::str::cat("Requested camera device not found: ", deviceId));
        }

        if (wasEnabled) {
            if (cameraCapturer_->start()) {
                auto source = cameraCapturer_->getVideoSource();
                if (source) {
                    localVideoTrack_ = livekit::LocalVideoTrack::createLocalVideoTrack("camera", source);

                    if (localParticipant && localVideoTrack_) {
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_CAMERA;
                        localParticipant->publishTrack(localVideoTrack_, options);
                        cameraTrackSid_ = resolvePublishedTrackSid(localParticipant, localVideoTrack_);
                        core::logInfo(core::str::cat("Camera switched and republished successfully", cameraTrackSid_.empty() ? std::string() : ": " + std::string(cameraTrackSid_)));
                    }
                }
            } else {
                core::logError("Failed to restart camera with new device");
                cameraEnabled_ = false;
                localCameraChanged.notify(cameraEnabled_);
            }
        }

        if (switched) {
            preferredCameraChanged.notify(deviceId);
        }

    } catch (const std::exception& e) {
        core::logError(core::str::cat("Exception in switchCamera: ", e.what()));
    }
}

void DeviceController::switchMicrophone(const std::string& deviceId)
{
    if (pendingDisableMicrophone_) {
        core::logWarning("Ignoring microphone switch while microphone shutdown is pending");
        return;
    }

    core::logInfo(core::str::cat("Switching microphone to device: ", deviceId));

    try {
        const bool wasEnabled = microphoneEnabled_;
        auto localParticipant = this->localParticipant();

        if (microphoneEnabled_ && localAudioTrack_) {
            const UnpublishOutcome outcome =
                unpublishLocalTrack(localParticipant, localAudioTrack_, &audioTrackSid_,
                                    "audio track", true);
            if (outcome == UnpublishOutcome::PublicationUnavailable) {
                core::logWarning("Deferring microphone switch until current publication SID becomes available");
                return;
            }
            microphoneCapturer_->stop();
            localAudioTrack_ = nullptr;
        }

        microphoneCapturer_->setDeviceById(deviceId);

        if (wasEnabled) {
            if (microphoneCapturer_->start()) {
                auto source = microphoneCapturer_->getAudioSource();
                if (source) {
                    localAudioTrack_ = livekit::LocalAudioTrack::createLocalAudioTrack("mic", source);

                    if (localParticipant && localAudioTrack_) {
                        livekit::TrackPublishOptions options;
                        options.source = livekit::TrackSource::SOURCE_MICROPHONE;
                        audioTrackSid_.clear();
                        localParticipant->publishTrack(localAudioTrack_, options);
                        core::logInfo("Microphone switched and republished successfully");
                    }
                }
            } else {
                core::logError("Failed to restart microphone with new device");
                microphoneEnabled_ = false;
                localMicrophoneChanged.notify(microphoneEnabled_);
            }
        }

        preferredMicrophoneChanged.notify(deviceId);

    } catch (const std::exception& e) {
        core::logError(core::str::cat("Exception in switchMicrophone: ", e.what()));
    }
}

void DeviceController::connectScreenSignals()
{
    // Previously a function-local `static QMetaObject::Connection`, which meant
    // a second DeviceController would disconnect the first one's handler. The
    // subscription is now per-instance and owned by this object.
    screenFrameConnection_ = screenCapturer_->frameCaptured.connect(
        [this](const core::VideoFrame& frame) {
            localScreenFrameReady.notify(frame);
        });
}

// =============================================================================
// Audio processing settings (runtime-applicable)
// =============================================================================

void DeviceController::applyAudioSettings(const core::AudioProcessingConfig& settings)
{
    if (!microphoneCapturer_) return;
    
    // Basic toggles
    microphoneCapturer_->setEchoCancellationEnabled(settings.echoCancellation);
    microphoneCapturer_->setNoiseSuppressionEnabled(settings.noiseSuppression);
    microphoneCapturer_->setAutoGainControlEnabled(settings.autoGainControl);
    microphoneCapturer_->setHighPassFilterEnabled(settings.highPassFilter);
    
    // Advanced parameters
    microphoneCapturer_->setNoiseSuppressionLevel(
        static_cast<AudioProcessingModule::NoiseSuppressionLevel>(settings.noiseSuppressionLevel));
    microphoneCapturer_->setGainControlMode(
        static_cast<AudioProcessingModule::GainControlMode>(settings.gainControlMode));
    microphoneCapturer_->setFixedDigitalGainDb(settings.fixedDigitalGainDb);
    microphoneCapturer_->setAdaptiveDigitalMaxGainDb(settings.adaptiveDigitalMaxGainDb);
    microphoneCapturer_->setEchoEnhancedFilterEnabled(settings.echoEnhancedFilter);

    core::logInfo(core::str::cat("Audio settings re-applied (AEC=", settings.echoCancellation, ", NS=", settings.noiseSuppression, "[lvl=", settings.noiseSuppressionLevel, "], AGC=", settings.autoGainControl, "[mode=", settings.gainControlMode, "], HPF=", settings.highPassFilter, ", AEC-enhanced=", settings.echoEnhancedFilter, ")"));
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
