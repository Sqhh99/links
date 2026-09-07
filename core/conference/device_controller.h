#ifndef CORE_CONFERENCE_DEVICE_CONTROLLER_H
#define CORE_CONFERENCE_DEVICE_CONTROLLER_H

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "../audio_processing_module.h"
#include "../base/signal.h"
#include "../base/time.h"
#include "../base/timer.h"
#include "../camera_capturer.h"
#include "../media/video_frame.h"
#include "../microphone_capturer.h"
#include "../platform_services.h"
#include "../screen_capturer.h"
#include "device_config.h"
#include "livekit/livekit.h"

class DeviceController {
public:
    DeviceController(livekit::Room* room,
                     const links::core::PlatformServices& services,
                     const links::core::DeviceSelection& devices,
                     const links::core::AudioProcessingConfig& audio);
    ~DeviceController();

    void setRoom(livekit::Room* room);
    void stopCapturers();
    void unpublishLocalTracks();
    void resetLocalState();
    void handleLocalTrackPublished(livekit::TrackSource source, const std::string& publicationSid);

    void toggleMicrophone();
    void toggleCamera();
    void toggleScreenShare();
    void setScreenShareMode(ScreenCapturer::Mode mode,
                            links::core::MonitorId monitorId,
                            links::core::WindowId windowId);
    void switchCamera(const std::string& deviceId);
    void switchMicrophone(const std::string& deviceId);

    bool isMicrophoneEnabled() const { return microphoneEnabled_; }
    bool isCameraEnabled() const { return cameraEnabled_; }
    bool isScreenSharing() const { return screenShareEnabled_; }

    // =========================================================================
    // Audio processing settings (runtime-applicable)
    // =========================================================================

    /// Re-apply the whole audio-processing configuration. The caller (ui/) owns
    /// persistence and hands the values in.
    void applyAudioSettings(const links::core::AudioProcessingConfig& config);

    // Basic toggles
    void setEchoCancellationEnabled(bool enabled);
    void setNoiseSuppressionEnabled(bool enabled);
    void setAutoGainControlEnabled(bool enabled);
    void setHighPassFilterEnabled(bool enabled);

    // Advanced parameters
    void setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel level);
    void setGainControlMode(AudioProcessingModule::GainControlMode mode);
    void setFixedDigitalGainDb(float gainDb);
    void setAdaptiveDigitalMaxGainDb(float maxGainDb);
    void setEchoEnhancedFilterEnabled(bool enabled);

    /**
     * Feed far-end (speaker) audio into the APM for AEC reference.
     * Called by MediaPipeline whenever remote audio is received.
     */
    void feedReverseAudio(const int16_t* data, int samples, int sampleRate, int channels);

    links::core::Signal<bool> localMicrophoneChanged;
    links::core::Signal<bool> localCameraChanged;
    links::core::Signal<bool> localScreenShareChanged;
    links::core::Signal<const links::core::VideoFrame&> localVideoFrameReady;
    links::core::Signal<const links::core::VideoFrame&> localScreenFrameReady;

    /// Emitted when a device switch should be persisted. Replaces the direct
    /// Settings::instance() writes core used to make.
    links::core::Signal<const std::string&> preferredCameraChanged;
    links::core::Signal<const std::string&> preferredMicrophoneChanged;

private:
    void connectScreenSignals();
    void schedulePendingUnpublishRetry();
    void processPendingUnpublish();
    void finalizeMicrophoneDisabled();
    void finalizeCameraDisabled();
    void finalizeScreenShareDisabled();
    livekit::Room* room() const { return room_; }

    /**
     * Promote the room's weak local-participant handle to a strong one.
     * Empty before connect and once the room has been torn down.
     */
    std::shared_ptr<livekit::LocalParticipant> localParticipant() const;

    const links::core::PlatformServices& services_;
    livekit::Room* room_{nullptr};

    std::unique_ptr<CameraCapturer> cameraCapturer_;
    std::unique_ptr<MicrophoneCapturer> microphoneCapturer_;
    std::unique_ptr<ScreenCapturer> screenCapturer_;

    std::shared_ptr<livekit::Track> localVideoTrack_;
    std::shared_ptr<livekit::Track> localAudioTrack_;
    std::shared_ptr<livekit::Track> localScreenTrack_;
    std::string audioTrackSid_;
    std::string screenTrackSid_;
    std::string cameraTrackSid_;

    bool cameraEnabled_{false};
    bool microphoneEnabled_{false};
    bool screenShareEnabled_{false};
    bool pendingDisableCamera_{false};
    bool pendingDisableMicrophone_{false};
    bool pendingDisableScreenShare_{false};
    bool pendingDisableCameraLogged_{false};
    bool pendingDisableMicrophoneLogged_{false};
    bool pendingDisableScreenShareLogged_{false};
    std::unique_ptr<links::core::Timer> pendingUnpublishRetryTimer_;

    // Monotonic, like QElapsedTimer. Unset until the first screen-share toggle.
    std::optional<links::core::SteadyClock::time_point> screenShareDebounceAt_;
    static constexpr int kScreenShareDebounceMs = 500;

    // Subscriptions to the capturers; cleared first in the destructor.
    links::core::ConnectionBag capturerConnections_;

    // Re-established whenever screen sharing starts; replacing it drops the
    // previous subscription.
    links::core::Connection screenFrameConnection_;
};

#endif // CORE_CONFERENCE_DEVICE_CONTROLLER_H
