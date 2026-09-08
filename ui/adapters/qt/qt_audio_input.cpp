#include "qt_audio_input.h"

#include <QByteArray>
#include <QMediaDevices>

#include <utility>

#include "core/base/log.h"

namespace links {
namespace qt_adapter {
namespace {

QAudioFormat toQtFormat(const core::AudioFormat& format)
{
    QAudioFormat out;
    out.setSampleRate(format.sampleRate);
    out.setChannelCount(format.channels);
    out.setSampleFormat(QAudioFormat::Int16);
    return out;
}

}  // namespace

QtAudioInput::QtAudioInput(QObject* parent)
    : QObject(parent)
{
}

QtAudioInput::~QtAudioInput()
{
    stop();
}

void QtAudioInput::setDataCallback(std::function<void(const std::int16_t*, std::size_t)> callback)
{
    dataCallback_ = std::move(callback);
}

void QtAudioInput::setErrorCallback(std::function<void(const std::string&)> callback)
{
    errorCallback_ = std::move(callback);
}

void QtAudioInput::setDeviceId(const std::string& deviceId)
{
    if (deviceId.empty()) {
        selectedDevice_ = QAudioDevice();
        audioSource_.reset();
        return;
    }

    // Device ids are opaque bytes; never re-encode them.
    const QByteArray wanted = QByteArray::fromStdString(deviceId);
    const auto devices = QMediaDevices::audioInputs();
    for (const auto& device : devices) {
        if (device.id() == wanted) {
            selectedDevice_ = device;
            audioSource_.reset();  // recreated with the new device on next start()
            return;
        }
    }
    core::logWarning("Microphone with the requested ID was not found");
}

void QtAudioInput::ensureSource(const core::AudioFormat& format)
{
    if (audioSource_) {
        return;
    }

    format_ = toQtFormat(format);

    QAudioDevice device = selectedDevice_.isNull()
        ? QMediaDevices::defaultAudioInput()
        : selectedDevice_;

    if (!device.isNull() && !device.isFormatSupported(format_)) {
        core::logWarning("Audio format not supported, using nearest");
        format_ = device.preferredFormat();
        format_.setSampleRate(format.sampleRate);
        format_.setChannelCount(format.channels);
        format_.setSampleFormat(QAudioFormat::Int16);
    }

    audioSource_ = std::make_unique<QAudioSource>(device, format_);
    connect(audioSource_.get(), &QAudioSource::stateChanged,
            this, &QtAudioInput::onStateChanged);
}

bool QtAudioInput::start(const core::AudioFormat& format)
{
    if (active_) {
        return true;
    }

    ensureSource(format);
    if (!audioSource_) {
        if (errorCallback_) {
            errorCallback_("Failed to create audio source");
        }
        return false;
    }

    audioInput_ = audioSource_->start();
    if (!audioInput_) {
        if (errorCallback_) {
            errorCallback_("Failed to start audio input");
        }
        return false;
    }

    connect(audioInput_, &QIODevice::readyRead, this, &QtAudioInput::onReadyRead);
    active_ = true;
    return true;
}

void QtAudioInput::stop()
{
    if (!active_) {
        return;
    }
    if (audioSource_) {
        audioSource_->stop();
    }
    audioInput_ = nullptr;
    active_ = false;
}

bool QtAudioInput::isActive() const
{
    return active_;
}

void QtAudioInput::onReadyRead()
{
    if (!audioInput_ || !active_ || !dataCallback_) {
        return;
    }

    const QByteArray data = audioInput_->readAll();
    if (data.isEmpty()) {
        return;
    }

    const std::size_t sampleCount = static_cast<std::size_t>(data.size()) / sizeof(std::int16_t);
    if (sampleCount == 0) {
        return;
    }
    dataCallback_(reinterpret_cast<const std::int16_t*>(data.constData()), sampleCount);
}

void QtAudioInput::onStateChanged(QAudio::State state)
{
    switch (state) {
    case QAudio::ActiveState:    core::logDebug("Audio state: Active"); break;
    case QAudio::SuspendedState: core::logDebug("Audio state: Suspended"); break;
    case QAudio::StoppedState:   core::logDebug("Audio state: Stopped"); break;
    case QAudio::IdleState:      core::logDebug("Audio state: Idle"); break;
    }
}

}  // namespace qt_adapter
}  // namespace links
