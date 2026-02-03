#include "microphone_capturer.h"
#include "../utils/logger.h"
#include "livekit/audio_frame.h"
#include <QMediaDevices>
#include <vector>

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
    
    // Initialize Audio Processing Module
    if (!apm_.initialize()) {
        Logger::instance().warning("Failed to initialize Audio Processing Module");
    }
    
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
    
    // Create LiveKit audio source (using official API)
    try {
        livekitAudioSource_ = std::make_shared<livekit::AudioSource>(48000, 1, 0);
        Logger::instance().info("LiveKit AudioSource created for microphone");
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
    
    // Reset LiveKit audio source
    livekitAudioSource_.reset();
    
    // Clear audio buffer
    audioBuffer_.clear();
    
    Logger::instance().info(QString("Microphone stopped (processed %1 samples)")
                           .arg(samplesProcessed_));
}

QList<QAudioDevice> MicrophoneCapturer::availableDevices()
{
    return QMediaDevices::audioInputs();
}

void MicrophoneCapturer::onReadyRead()
{
    if (!audioInput_ || !isActive_) {
        return;
    }
    
    const QByteArray data = audioInput_->readAll();
    if (!data.isEmpty()) {
        processAudioData(data);
    }
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
    if (!livekitAudioSource_) {
        return;
    }
    
    // Calculate samples from incoming data
    const int bytesPerSample = format_.bytesPerSample();
    const int numChannels = format_.channelCount();
    const int incomingSamples = data.size() / bytesPerSample / numChannels;
    
    if (incomingSamples <= 0) {
        return;
    }
    
    // Get pointer to incoming audio data
    const int16_t* incomingData = reinterpret_cast<const int16_t*>(data.constData());
    
    // Add incoming samples to buffer
    audioBuffer_.insert(audioBuffer_.end(), incomingData, incomingData + incomingSamples * numChannels);
    
    // Send buffered frames (each 10ms = 480 samples at 48kHz)
    sendBufferedFrames();
}

void MicrophoneCapturer::sendBufferedFrames()
{
    if (!livekitAudioSource_) {
        return;
    }
    
    const int numChannels = format_.channelCount();
    const int frameSizeTotalSamples = kFrameSizeSamples * numChannels;
    
    // Process all complete 10ms frames in the buffer
    while (audioBuffer_.size() >= static_cast<size_t>(frameSizeTotalSamples)) {
        try {
            // Extract one frame worth of samples
            std::vector<int16_t> frameData(audioBuffer_.begin(), 
                                           audioBuffer_.begin() + frameSizeTotalSamples);
            
            // Process through Audio Processing Module (in-place)
            if (apm_.isInitialized()) {
                apm_.processFrame(frameData.data(), kFrameSizeSamples, 
                                 format_.sampleRate(), numChannels);
            }
            
            // Create audio frame and send to LiveKit
            livekit::AudioFrame frame(std::move(frameData),
                                      format_.sampleRate(), 
                                      numChannels, 
                                      kFrameSizeSamples);
            
            // Send to LiveKit
            livekitAudioSource_->captureFrame(frame);
            
            // Remove sent samples from buffer
            audioBuffer_.erase(audioBuffer_.begin(), 
                              audioBuffer_.begin() + frameSizeTotalSamples);
            
            samplesProcessed_ += kFrameSizeSamples;
            
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Failed to capture audio: %1").arg(e.what()));
            break;
        }
    }
    
    // Log every second (48000 samples at 48kHz)
    static int64_t lastLoggedSamples = 0;
    if (samplesProcessed_ - lastLoggedSamples >= 48000) {
        Logger::instance().debug(QString("Captured %1 audio samples (buffer: %2)")
                                .arg(samplesProcessed_)
                                .arg(audioBuffer_.size()));
        lastLoggedSamples = samplesProcessed_;
    }
}

void MicrophoneCapturer::setDevice(const QAudioDevice& device)
{
    if (isActive_) {
        Logger::instance().warning("Cannot change microphone while active");
        return;
    }
    selectedDevice_ = device;
    audioSource_.reset();
    
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

void MicrophoneCapturer::setEchoCancellationEnabled(bool enabled)
{
    apm_.setEchoCancellationEnabled(enabled);
}

void MicrophoneCapturer::setNoiseSuppressionEnabled(bool enabled)
{
    apm_.setNoiseSuppressionEnabled(enabled);
}

void MicrophoneCapturer::setAutoGainControlEnabled(bool enabled)
{
    apm_.setAutoGainControlEnabled(enabled);
}
