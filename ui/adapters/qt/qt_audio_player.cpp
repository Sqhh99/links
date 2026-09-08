#include "qt_audio_player.h"

#include <QMediaDevices>

namespace links {
namespace qt_adapter {
namespace {

QAudioFormat::SampleFormat toQt(core::SampleFormat format)
{
    switch (format) {
    case core::SampleFormat::UInt8:   return QAudioFormat::UInt8;
    case core::SampleFormat::Int16:   return QAudioFormat::Int16;
    case core::SampleFormat::Int32:   return QAudioFormat::Int32;
    case core::SampleFormat::Float:   return QAudioFormat::Float;
    case core::SampleFormat::Unknown: return QAudioFormat::Unknown;
    }
    return QAudioFormat::Unknown;
}

core::SampleFormat fromQt(QAudioFormat::SampleFormat format)
{
    switch (format) {
    case QAudioFormat::UInt8: return core::SampleFormat::UInt8;
    case QAudioFormat::Int16: return core::SampleFormat::Int16;
    case QAudioFormat::Int32: return core::SampleFormat::Int32;
    case QAudioFormat::Float: return core::SampleFormat::Float;
    default:                  return core::SampleFormat::Unknown;
    }
}

QAudioFormat toQtFormat(const core::AudioFormat& format)
{
    QAudioFormat out;
    out.setSampleRate(format.sampleRate);
    out.setChannelCount(format.channels);
    out.setSampleFormat(toQt(format.sampleFormat));
    return out;
}

core::AudioFormat fromQtFormat(const QAudioFormat& format)
{
    core::AudioFormat out;
    out.sampleRate = format.sampleRate();
    out.channels = format.channelCount();
    out.sampleFormat = fromQt(format.sampleFormat());
    return out;
}

}  // namespace

core::AudioFormat QtAudioPlayer::negotiate(int sampleRate, int channels)
{
    const QAudioDevice device = QMediaDevices::defaultAudioOutput();

    QAudioFormat requested;
    requested.setSampleRate(sampleRate);
    requested.setChannelCount(channels);
    requested.setSampleFormat(QAudioFormat::Int16);
    if (device.isFormatSupported(requested)) {
        return fromQtFormat(requested);
    }

    QAudioFormat fallback = device.preferredFormat();
    if (!device.isFormatSupported(fallback)) {
        fallback = requested;
    }
    return fromQtFormat(fallback);
}

bool QtAudioPlayer::open(const core::AudioFormat& format)
{
    close();

    outputDevice_ = QMediaDevices::defaultAudioOutput();
    format_ = toQtFormat(format);
    sink_ = std::make_unique<QAudioSink>(outputDevice_, format_);
    device_ = sink_ ? sink_->start() : nullptr;
    return device_ != nullptr;
}

bool QtAudioPlayer::isOpen() const
{
    return device_ != nullptr;
}

void QtAudioPlayer::write(const std::uint8_t* data, std::size_t bytes)
{
    if (!device_ || !data || bytes == 0) {
        return;
    }
    device_->write(reinterpret_cast<const char*>(data), static_cast<qint64>(bytes));
}

void QtAudioPlayer::close()
{
    if (sink_) {
        sink_->stop();
    }
    sink_.reset();
    device_ = nullptr;
}

std::string QtAudioPlayer::currentDeviceId() const
{
    return outputDevice_.id().toStdString();
}

std::unique_ptr<core::AudioPlayer> QtAudioPlayerFactory::createPlayer()
{
    return std::make_unique<QtAudioPlayer>();
}

std::string QtAudioPlayerFactory::defaultDeviceId() const
{
    return QMediaDevices::defaultAudioOutput().id().toStdString();
}

}  // namespace qt_adapter
}  // namespace links
