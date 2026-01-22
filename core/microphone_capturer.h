#ifndef MICROPHONE_CAPTURER_H
#define MICROPHONE_CAPTURER_H

#include <QObject>
#include <QAudioSource>
#include <QAudioFormat>
#include <QIODevice>
#include <memory>
#include "livekit/audio_source.h"

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
    
    // Audio processing options (must be called before start())
    void setAudioProcessingOptions(bool echoCancellation, bool noiseSuppression, bool autoGainControl);
    
signals:
    void error(const QString& message);
    
private slots:
    void onReadyRead();
    void onStateChanged(QAudio::State state);
    
private:
    void processAudioData(const QByteArray& data);
    
    std::unique_ptr<QAudioSource> audioSource_;
    std::shared_ptr<livekit::AudioSource> livekitAudioSource_;
    QIODevice* audioInput_;
    QAudioFormat format_;
    
    bool isActive_;
    int64_t samplesProcessed_;
    
    // Selected audio device
    QAudioDevice selectedDevice_;
    
    // Audio processing options for LiveKit AudioSource
    livekit::AudioSourceOptions audioOptions_;
};

#endif // MICROPHONE_CAPTURER_H
