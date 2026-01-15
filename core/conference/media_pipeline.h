#ifndef CORE_CONFERENCE_MEDIA_PIPELINE_H
#define CORE_CONFERENCE_MEDIA_PIPELINE_H

#include <QAudioFormat>
#include <QAudioSink>
#include <QImage>
#include <QMap>
#include <QIODevice>
#include <QSharedPointer>
#include <QString>
#include <QObject>
#include <QMediaDevices>
#include <QAudioDevice>
#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include "livekit/livekit.h"

class ParticipantStore;

class MediaPipeline : public QObject {
    Q_OBJECT

public:
    explicit MediaPipeline(ParticipantStore* participantStore, QObject* parent = nullptr);
    ~MediaPipeline() override;

    void startVideoStreamReader(const QString& trackSid,
                                const QString& participantIdentity,
                                std::shared_ptr<livekit::VideoStream> stream);
    void startAudioStreamReader(const QString& trackSid,
                                const QString& participantIdentity,
                                std::shared_ptr<livekit::AudioStream> stream);
    void stopTrack(const QString& trackSid);
    void stopAll();

    void setVideoStream(const QString& trackSid, std::shared_ptr<livekit::VideoStream> stream);
    void setAudioStream(const QString& trackSid, std::shared_ptr<livekit::AudioStream> stream);
    bool hasVideoStream(const QString& trackSid) const;
    bool hasAudioStream(const QString& trackSid) const;
    void removeVideoStream(const QString& trackSid);
    void removeAudioStream(const QString& trackSid);

signals:
    void videoFrameReady(const QString& participantIdentity,
                         const QString& trackSid,
                         const QImage& frame,
                         livekit::TrackSource source);
    void audioActivity(const QString& participantIdentity, bool hasAudio);

private:
    struct AudioPlayback {
        QSharedPointer<QAudioSink> sink;
        QIODevice* device{nullptr};
        QAudioFormat format;
    };

    void handleVideoFrame(const livekit::VideoFrameEvent& event,
                          const QString& trackSid,
                          const QString& participantIdentity);
    void handleAudioFrame(const livekit::AudioFrameEvent& event,
                          const QString& trackSid,
                          const QString& participantIdentity);
    void stopStreamReaders(const QString& trackSid);

    ParticipantStore* participantStore_;
    QMap<QString, std::shared_ptr<livekit::VideoStream>> videoStreams_;
    QMap<QString, std::shared_ptr<livekit::AudioStream>> audioStreams_;
    std::map<QString, std::unique_ptr<std::thread>> videoStreamThreads_;
    std::map<QString, std::unique_ptr<std::thread>> audioStreamThreads_;
    QMap<QString, std::atomic<bool>*> streamStopFlags_;
    QMap<QString, AudioPlayback> audioPlayers_;
};

#endif // CORE_CONFERENCE_MEDIA_PIPELINE_H
