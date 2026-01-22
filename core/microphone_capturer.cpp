#include "microphone_capturer.h"
#include "../utils/logger.h"
#include "livekit/audio_frame.h"
#include <QMediaDevices>

MicrophoneCapturer::MicrophoneCapturer(QObject* parent)
    : QObject(parent),
      audioInput_(nullptr),
      isActive_(false),
      samplesProcessed_(0)
{
    // Set up audio format (48kHz, 16-bit, mono)
    format_.setSampleRate(48000);
    format_.setChannelCount(1);
    format_.setSampleFormat(QAudioFormat::Int16);
    
    // Note: LiveKit AudioSource is created lazily in start() to ensure 
    // audio processing options are applied correctly
    
    // Get default or selected audio input device
    QAudioDevice deviceInfo;
    if (!selectedDevice_.isNull()) {
        // Check if selected device is still available
        const auto devices = QMediaDevices::audioInputs();
        bool found = false;
        for (const auto& dev : devices) {
            if (dev.id() == selectedDevice_.id()) {
                deviceInfo = dev;
                found = true;
                break;
            }
        }
        if (!found) {
            Logger::instance().warning(QString("Selected microphone '%1' not found, using default")
                                      .arg(selectedDevice_.description()));
            deviceInfo = QMediaDevices::defaultAudioInput();
        }
    } else {
        deviceInfo = QMediaDevices::defaultAudioInput();
    }
    if (deviceInfo.isNull()) {
        Logger::instance().warning("No microphone available");
        emit error("No microphone available");
        return;
    }
    
    Logger::instance().info(QString("Using microphone: %1").arg(deviceInfo.description()));
    
    // Check if format is supported
    if (!deviceInfo.isFormatSupported(format_)) {
        Logger::instance().warning("Audio format not supported, using nearest");
        format_ = deviceInfo.preferredFormat();
        
        // Try to adjust to our preferred settings
        format_.setSampleRate(48000);
        format_.setChannelCount(1);
        format_.setSampleFormat(QAudioFormat::Int16);
    }
    
    // Create audio source
    audioSource_ = std::make_unique<QAudioSource>(deviceInfo, format_);
    
    // Connect state changed signal
    connect(audioSource_.get(), &QAudioSource::stateChanged,
            this, &MicrophoneCapturer::onStateChanged);
}

MicrophoneCapturer::~MicrophoneCapturer()
{
    stop();
}

bool MicrophoneCapturer::start()
{
    if (isActive_) {
        return true;
    }
    
    if (!audioSource_) {
        Logger::instance().error("Audio source not initialized");
        return false;
    }
    
    // Create LiveKit audio source with current audio processing options
    // This is done here (not in constructor) to ensure options are applied correctly
    try {
        livekitAudioSource_ = std::make_shared<livekit::AudioSource>(48000, 1, 1000, audioOptions_);
        Logger::instance().info(QString("AudioSource created for microphone (echo_cancellation=%1, noise_suppression=%2, auto_gain_control=%3)")
                               .arg(audioOptions_.echo_cancellation)
                               .arg(audioOptions_.noise_suppression)
                               .arg(audioOptions_.auto_gain_control));
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Failed to create AudioSource: %1").arg(e.what()));
        emit error(QString("Failed to create audio source: %1").arg(e.what()));
        return false;
    }
    
    try {
        audioInput_ = audioSource_->start();
        if (!audioInput_) {
            Logger::instance().error("Failed to start audio input");
            livekitAudioSource_.reset();
            return false;
        }
        
        // Connect ready read signal
        connect(audioInput_, &QIODevice::readyRead,
                this, &MicrophoneCapturer::onReadyRead);
        
        isActive_ = true;
        samplesProcessed_ = 0;
        
        Logger::instance().info(QString("Microphone started (rate: %1, channels: %2)")
                               .arg(format_.sampleRate())
                               .arg(format_.channelCount()));
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Failed to start microphone: %1").arg(e.what()));
        emit error(QString("Failed to start microphone: %1").arg(e.what()));
        livekitAudioSource_.reset();
        return false;
    }
}

void MicrophoneCapturer::stop()
{
    if (!isActive_) {
        return;
    }
    
    if (audioSource_) {
        audioSource_->stop();
    }
    
    audioInput_ = nullptr;
    isActive_ = false;
    
    // Reset LiveKit audio source so it can be recreated with potentially new options
    livekitAudioSource_.reset();
    
    Logger::instance().info(QString("Microphone stopped (processed %1 samples)")
                           .arg(samplesProcessed_));
}

QList<QAudioDevice> MicrophoneCapturer::availableDevices()
{
    return QMediaDevices::audioInputs();
}

void MicrophoneCapturer::onReadyRead()
{
    if (!audioInput_ || !livekitAudioSource_) {
        return;
    }
    
    // Read available audio data
    QByteArray data = audioInput_->readAll();
    if (data.isEmpty()) {
        return;
    }
    
    processAudioData(data);
}

void MicrophoneCapturer::onStateChanged(QAudio::State state)
{
    switch (state) {
        case QAudio::ActiveState:
            Logger::instance().debug("Audio state: Active");
            break;
        case QAudio::SuspendedState:
            Logger::instance().debug("Audio state: Suspended");
            break;
        case QAudio::StoppedState:
            Logger::instance().debug("Audio state: Stopped");
            break;
        case QAudio::IdleState:
            Logger::instance().debug("Audio state: Idle");
            break;
    }
}

void MicrophoneCapturer::processAudioData(const QByteArray& data)
{
    // Convert QByteArray to int16_t array
    const int16_t* samples = reinterpret_cast<const int16_t*>(data.constData());
    uint32_t sampleCount = data.size() / sizeof(int16_t);
    
    if (sampleCount == 0) {
        return;
    }
    
    // Calculate samples per channel
    uint32_t samplesPerChannel = sampleCount / format_.channelCount();
    
    try {
        // Create AudioFrame for LiveKit SDK
        std::vector<int16_t> sampleVec(samples, samples + sampleCount);
        livekit::AudioFrame frame(std::move(sampleVec), 
                                  format_.sampleRate(), 
                                  format_.channelCount(), 
                                  samplesPerChannel);
        
        // Send to LiveKit (blocking call with default timeout)
        livekitAudioSource_->captureFrame(frame);
        
        samplesProcessed_ += samplesPerChannel;
        
        // Log every second (48000 samples at 48kHz)
        if (samplesProcessed_ % 48000 < samplesPerChannel) {
            Logger::instance().debug(QString("Captured %1 audio samples")
                                    .arg(samplesProcessed_));
        }
    } catch (const std::exception& e) {
        Logger::instance().error(QString("Failed to capture audio: %1").arg(e.what()));
    }
}

void MicrophoneCapturer::setDevice(const QAudioDevice& device)
{
    if (isActive_) {
        Logger::instance().warning("Cannot change microphone while active");
        return;
    }
    selectedDevice_ = device;
    audioSource_.reset(); // Reset audio source so it will be recreated with new device
    
    // Recreate audio source with new device
    if (!device.isNull()) {
        if (!device.isFormatSupported(format_)) {
            Logger::instance().warning("Audio format not supported, using nearest");
            format_ = device.preferredFormat();
            format_.setSampleRate(48000);
            format_.setChannelCount(1);
            format_.setSampleFormat(QAudioFormat::Int16);
        }
        audioSource_ = std::make_unique<QAudioSource>(device, format_);
        connect(audioSource_.get(), &QAudioSource::stateChanged,
                this, &MicrophoneCapturer::onStateChanged);
        Logger::instance().info(QString("Microphone device set to: %1").arg(device.description()));
    }
}

void MicrophoneCapturer::setDeviceById(const QByteArray& deviceId)
{
    if (deviceId.isEmpty()) {
        selectedDevice_ = QAudioDevice();
        return;
    }
    
    const auto devices = QMediaDevices::audioInputs();
    for (const auto& device : devices) {
        if (device.id() == deviceId) {
            setDevice(device);
            return;
        }
    }
    Logger::instance().warning(QString("Microphone with ID '%1' not found")
                              .arg(QString(deviceId)));
}

void MicrophoneCapturer::setAudioProcessingOptions(bool echoCancellation, bool noiseSuppression, bool autoGainControl)
{
    if (isActive_) {
        Logger::instance().warning("Cannot change audio processing options while microphone is active");
        return;
    }
    
    audioOptions_.echo_cancellation = echoCancellation;
    audioOptions_.noise_suppression = noiseSuppression;
    audioOptions_.auto_gain_control = autoGainControl;
    
    Logger::instance().info(QString("Audio processing options set: echo_cancellation=%1, noise_suppression=%2, auto_gain_control=%3")
                           .arg(echoCancellation)
                           .arg(noiseSuppression)
                           .arg(autoGainControl));
}
