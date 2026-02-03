#ifndef MICROPHONE_CAPTURER_H
#define MICROPHONE_CAPTURER_H

#include <QObject>
#include <QAudioSource>
#include <QAudioFormat>
#include <QIODevice>
#include <memory>
#include <vector>
#include "livekit/audio_source.h"
#include "audio_processing_module.h"

class MicrophoneCapturer : public QObject
{
    Q_OBJECT
    
public:
    explicit MicrophoneCapturer(QObject* parent = nullptr);
    ~MicrophoneCapturer() override;
    
    // Start/stop capture
    bool start();
    void stop();
    bool isActive() const { return isActive_; }
    
    // Get the LiveKit audio source
    std::shared_ptr<livekit::AudioSource> getAudioSource() const { return livekitAudioSource_; }
    
    // Get available audio devices
    static QList<QAudioDevice> availableDevices();
    
    // Set audio device (must be called before start())
    void setDevice(const QAudioDevice& device);
    void setDeviceById(const QByteArray& deviceId);
    
    // Audio processing options (AEC, NS, AGC) - delegates to AudioProcessingModule
    void setEchoCancellationEnabled(bool enabled);
    void setNoiseSuppressionEnabled(bool enabled);
    void setAutoGainControlEnabled(bool enabled);
    
    // Get the audio processing module for advanced configuration
    AudioProcessingModule* audioProcessingModule() { return &apm_; }
    
signals:
    void error(const QString& message);
    
private slots:
    void onReadyRead();
    void onStateChanged(QAudio::State state);
    
private:
    void processAudioData(const QByteArray& data);
    void sendBufferedFrames();
    
    std::unique_ptr<QAudioSource> audioSource_;
    std::shared_ptr<livekit::AudioSource> livekitAudioSource_;
    QIODevice* audioInput_;
    QAudioFormat format_;
    
    bool isActive_;
    int64_t samplesProcessed_;
    
    // Selected audio device
    QAudioDevice selectedDevice_;
    
    // Audio Processing Module
    AudioProcessingModule apm_;
    
    // Audio buffer for accumulating samples to send in 10ms frames
    // At 48kHz mono, 10ms = 480 samples
    static constexpr int kFrameSizeSamples = 480; // 10ms at 48kHz
    std::vector<int16_t> audioBuffer_;
};

#endif // MICROPHONE_CAPTURER_H
