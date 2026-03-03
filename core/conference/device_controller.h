#ifndef CORE_CONFERENCE_DEVICE_CONTROLLER_H
#define CORE_CONFERENCE_DEVICE_CONTROLLER_H

#include <QElapsedTimer>
#include <QObject>
#include <QImage>
#include <QString>
#include <memory>
#include <string>
#include "livekit/livekit.h"
#include "../camera_capturer.h"
#include "../microphone_capturer.h"
#include "../screen_capturer.h"
#include "../audio_processing_module.h"

class DeviceController : public QObject {
    Q_OBJECT

public:
    explicit DeviceController(livekit::Room* room, QObject* parent = nullptr);

    void setRoom(livekit::Room* room);
    void stopCapturers();
    void unpublishLocalTracks();
    void resetLocalState();

    void toggleMicrophone();
    void toggleCamera();
    void toggleScreenShare();
    void setScreenShareMode(ScreenCapturer::Mode mode, QScreen* screen, WId windowId);
    void switchCamera(const QString& deviceId);
    void switchMicrophone(const QString& deviceId);

    bool isMicrophoneEnabled() const { return microphoneEnabled_; }
    bool isCameraEnabled() const { return cameraEnabled_; }
    bool isScreenSharing() const { return screenShareEnabled_; }

    // =========================================================================
    // Audio processing settings (runtime-applicable)
    // =========================================================================
    void applyAudioSettings();  // Re-read from Settings and apply

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

signals:
    void localMicrophoneChanged(bool enabled);
    void localCameraChanged(bool enabled);
    void localScreenShareChanged(bool enabled);
    void localVideoFrameReady(const QImage& frame);
    void localScreenFrameReady(const QImage& frame);

private:
    void connectScreenSignals();
    livekit::Room* room() const { return room_; }

    livekit::Room* room_{nullptr};
    CameraCapturer* cameraCapturer_{nullptr};
    MicrophoneCapturer* microphoneCapturer_{nullptr};
    ScreenCapturer* screenCapturer_{nullptr};
    std::shared_ptr<livekit::Track> localVideoTrack_;
    std::shared_ptr<livekit::Track> localAudioTrack_;
    std::shared_ptr<livekit::Track> localScreenTrack_;
    std::string screenTrackSid_;
    std::string cameraTrackSid_;

    bool cameraEnabled_{false};
    bool microphoneEnabled_{false};
    bool screenShareEnabled_{false};

    QElapsedTimer screenShareDebounceTimer_;
    static constexpr int kScreenShareDebounceMs = 500;
};

#endif // CORE_CONFERENCE_DEVICE_CONTROLLER_H
