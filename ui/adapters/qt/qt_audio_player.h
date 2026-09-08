#ifndef QT_AUDIO_PLAYER_H
#define QT_AUDIO_PLAYER_H

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>

#include <memory>

#include "core/media/audio_player.h"

namespace links {
namespace qt_adapter {

/**
 * AudioPlayer backed by QAudioSink.
 *
 * This is the QAudioSink half of the old MediaPipeline, moved rather than
 * rewritten: the format negotiation, the recreate condition and the raw
 * QIODevice write are the same code. Buffer size is deliberately left at
 * Qt's default, because that is what production latency and underrun
 * behaviour are tuned to today.
 */
class QtAudioPlayer : public core::AudioPlayer {
public:
    core::AudioFormat negotiate(int sampleRate, int channels) override;
    bool open(const core::AudioFormat& format) override;
    bool isOpen() const override;
    void write(const std::uint8_t* data, std::size_t bytes) override;
    void close() override;
    std::string currentDeviceId() const override;

private:
    std::unique_ptr<QAudioSink> sink_;
    QIODevice* device_{nullptr};
    QAudioDevice outputDevice_;
    QAudioFormat format_;
};

class QtAudioPlayerFactory : public core::AudioPlayerFactory {
public:
    std::unique_ptr<core::AudioPlayer> createPlayer() override;
    std::string defaultDeviceId() const override;
};

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_AUDIO_PLAYER_H
