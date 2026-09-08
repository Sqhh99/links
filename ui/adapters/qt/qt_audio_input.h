#ifndef QT_AUDIO_INPUT_H
#define QT_AUDIO_INPUT_H

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QObject>

#include <memory>

#include "core/media/audio_input.h"

namespace links {
namespace qt_adapter {

/**
 * AudioInput backed by QAudioSource.
 *
 * The QAudioSource half of the old core/microphone_capturer.cpp, moved rather
 * than rewritten: device selection, format fallback and the readyRead pull
 * loop behave exactly as before. The APM and 10 ms framing stayed in core.
 */
class QtAudioInput : public QObject, public core::AudioInput {
    Q_OBJECT
public:
    explicit QtAudioInput(QObject* parent = nullptr);
    ~QtAudioInput() override;

    bool start(const core::AudioFormat& format) override;
    void stop() override;
    bool isActive() const override;
    void setDeviceId(const std::string& deviceId) override;
    void setDataCallback(std::function<void(const std::int16_t*, std::size_t)> callback) override;
    void setErrorCallback(std::function<void(const std::string&)> callback) override;

private:
    void onReadyRead();
    void onStateChanged(QAudio::State state);
    void ensureSource(const core::AudioFormat& format);

    std::unique_ptr<QAudioSource> audioSource_;
    QIODevice* audioInput_{nullptr};
    QAudioFormat format_;
    QAudioDevice selectedDevice_;
    bool active_{false};

    std::function<void(const std::int16_t*, std::size_t)> dataCallback_;
    std::function<void(const std::string&)> errorCallback_;
};

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_AUDIO_INPUT_H
