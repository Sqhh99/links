#ifndef LINKS_UI_ADAPTERS_QT_AUDIO_PREVIEW_SOURCE_H_
#define LINKS_UI_ADAPTERS_QT_AUDIO_PREVIEW_SOURCE_H_

#include <QObject>
#include <QString>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class QAudioSource;
class QIODevice;

class QtAudioPreviewSource : public QObject
{
    Q_OBJECT

public:
    using FrameCallback = std::function<void(const std::vector<int16_t>&, int, int)>;

    explicit QtAudioPreviewSource(QObject* parent = nullptr);
    ~QtAudioPreviewSource() override;

    void setDeviceId(const QString& deviceId);
    void setFrameCallback(FrameCallback callback);
    bool start();
    void stop();
    bool isActive() const { return isActive_; }

signals:
    void errorOccurred(const QString& errorMessage);

private slots:
    void onReadyRead();

private:
    bool createAudioSource();

    QString deviceId_;
    int sampleRate_{48000};
    int channelCount_{1};
    std::unique_ptr<QAudioSource> audioSource_;
    QIODevice* audioInput_{nullptr};
    FrameCallback frameCallback_;
    bool isActive_{false};
};

#endif  // LINKS_UI_ADAPTERS_QT_AUDIO_PREVIEW_SOURCE_H_
