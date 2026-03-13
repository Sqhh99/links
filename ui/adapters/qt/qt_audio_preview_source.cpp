#include "qt_audio_preview_source.h"

#include "utils/logger.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>

#include <cstring>
#include <utility>

QtAudioPreviewSource::QtAudioPreviewSource(QObject* parent)
    : QObject(parent)
{
}

QtAudioPreviewSource::~QtAudioPreviewSource()
{
    stop();
}

void QtAudioPreviewSource::setDeviceId(const QString& deviceId)
{
    if (deviceId_ == deviceId) {
        return;
    }

    const bool restart = isActive_;
    if (restart) {
        stop();
    }
    deviceId_ = deviceId;
    if (restart) {
        start();
    }
}

void QtAudioPreviewSource::setFrameCallback(FrameCallback callback)
{
    frameCallback_ = std::move(callback);
}

bool QtAudioPreviewSource::start()
{
    if (isActive_) {
        return true;
    }
    if (!createAudioSource()) {
        return false;
    }

    audioInput_ = audioSource_->start();
    if (!audioInput_) {
        emit errorOccurred(QStringLiteral("无法启动麦克风预览采集"));
        audioSource_.reset();
        return false;
    }

    connect(audioInput_, &QIODevice::readyRead, this, &QtAudioPreviewSource::onReadyRead, Qt::UniqueConnection);
    isActive_ = true;
    Logger::instance().info("Speech preview audio capture started");
    return true;
}

void QtAudioPreviewSource::stop()
{
    if (audioSource_) {
        audioSource_->stop();
    }
    audioInput_ = nullptr;
    audioSource_.reset();
    isActive_ = false;
}

void QtAudioPreviewSource::onReadyRead()
{
    if (!audioInput_ || !frameCallback_) {
        return;
    }

    const QByteArray data = audioInput_->readAll();
    if (data.isEmpty()) {
        return;
    }

    const qsizetype sampleByteCount = data.size() - (data.size() % static_cast<qsizetype>(sizeof(int16_t)));
    if (sampleByteCount <= 0) {
        return;
    }
    if (sampleByteCount != data.size()) {
        Logger::instance().warning(QString("Dropping %1 trailing byte(s) from preview audio buffer")
                                   .arg(data.size() - sampleByteCount));
    }

    std::vector<int16_t> samples(static_cast<std::size_t>(sampleByteCount / static_cast<qsizetype>(sizeof(int16_t))), 0);
    std::memcpy(samples.data(), data.constData(), static_cast<std::size_t>(sampleByteCount));
    frameCallback_(samples, sampleRate_, channelCount_);
}

bool QtAudioPreviewSource::createAudioSource()
{
    QAudioDevice selectedDevice = QMediaDevices::defaultAudioInput();
    if (!deviceId_.isEmpty()) {
        const auto devices = QMediaDevices::audioInputs();
        for (const auto& device : devices) {
            if (QString::fromUtf8(device.id()) == deviceId_) {
                selectedDevice = device;
                break;
            }
        }
    }

    if (selectedDevice.isNull()) {
        emit errorOccurred(QStringLiteral("未找到可用麦克风设备"));
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!selectedDevice.isFormatSupported(format)) {
        QAudioFormat preferredFormat = selectedDevice.preferredFormat();
        if (preferredFormat.sampleFormat() != QAudioFormat::Int16) {
            emit errorOccurred(QStringLiteral("当前麦克风不支持 Int16 采样格式，无法进行语音转写预览"));
            return false;
        }
        format = preferredFormat;
    }

    sampleRate_ = format.sampleRate();
    channelCount_ = format.channelCount();
    audioSource_ = std::make_unique<QAudioSource>(selectedDevice, format, this);
    return true;
}
