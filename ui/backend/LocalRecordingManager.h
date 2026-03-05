#ifndef LOCAL_RECORDING_MANAGER_H
#define LOCAL_RECORDING_MANAGER_H

#include <QElapsedTimer>
#include <QImage>
#include <QMap>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <memory>

class QAudioInput;
class QObject;
class QPainter;
class QVideoFrameInput;

class LocalRecordingManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingStateChanged)
    Q_PROPERTY(int recordingDurationSeconds READ recordingDurationSeconds NOTIFY recordingDurationChanged)
    Q_PROPERTY(QString recordingDurationText READ recordingDurationText NOTIFY recordingDurationChanged)
    Q_PROPERTY(QString currentOutputPath READ currentOutputPath NOTIFY currentOutputPathChanged)
    Q_PROPERTY(QString outputDirectory READ outputDirectory CONSTANT)
    Q_PROPERTY(QVariantList recentRecordings READ recentRecordings NOTIFY recentRecordingsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    static LocalRecordingManager& instance();
    ~LocalRecordingManager() override;

    bool isRecording() const { return isRecording_; }
    bool isAvailable() const;
    int recordingDurationSeconds() const;
    QString recordingDurationText() const;
    QString currentOutputPath() const { return currentOutputPath_; }
    QString outputDirectory() const { return outputDirectory_; }
    QVariantList recentRecordings() const { return recentRecordings_; }
    QString lastError() const { return lastError_; }

    // --- Recording lifecycle ---------------------------------------------------
    Q_INVOKABLE bool startConferenceRecording(const QString& meetingNo,
                                              const QString& userName);
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE bool toggleRecording(const QString& meetingNo,
                                     const QString& userName);
    Q_INVOKABLE void openRecordingFolder();
    Q_INVOKABLE void refreshRecentRecordings();

    // --- Frame feed API (called by ConferenceBackend) -------------------------
    void feedLocalCameraFrame(const QImage& frame);
    void feedRemoteCameraFrame(const QString& identity, const QImage& frame);
    void feedScreenShareFrame(const QString& identity, const QImage& frame);
    void clearScreenShareFrame();
    void removeRemoteParticipant(const QString& identity);
    void setParticipantNames(const QMap<QString, QString>& names);

signals:
    void recordingStateChanged();
    void recordingDurationChanged();
    void currentOutputPathChanged();
    void recentRecordingsChanged();
    void lastErrorChanged();

private slots:
    void onRecorderStateChanged(QMediaRecorder::RecorderState state);
    void onRecorderErrorChanged();
    void updateDurationTick();

private:
    explicit LocalRecordingManager(QObject* parent = nullptr);
    bool ensureOutputDirectoryReady();
    QString buildOutputPath(const QString& meetingNo, const QString& userName) const;
    void compositeAndPushFrame();
    void drawParticipantCell(QPainter& painter, const QRect& cellRect,
                             const QImage& frame, const QString& name);
    void drawPlaceholderCell(QPainter& painter, const QRect& cellRect,
                             const QString& name);
    void setLastError(const QString& message);
    void clearLastError();
    void releaseSessionResources();
    QVariantList scanRecentRecordings() const;
    static QString formatDuration(int totalSeconds);
    static QString formatFileSize(qint64 bytes);

    // Recording infrastructure
    QMediaCaptureSession captureSession_;
    std::unique_ptr<QAudioInput> audioInput_;
    std::unique_ptr<QVideoFrameInput> videoFrameInput_;
    std::unique_ptr<QMediaRecorder> recorder_;

    bool isRecording_{false};
    int lastDurationSeconds_{0};
    QElapsedTimer recordingElapsed_;
    QTimer durationTimer_;
    QTimer compositeTimer_;
    QTimer frameLogTimer_;
    int compositeFrameCount_{0};
    qint64 compositeAttemptCount_{0};
    qint64 lastFrameStartUs_{-1};
    bool frameInputReady_{false};
    bool inCompositePush_{false};
    int frameInputNotReadyCount_{0};
    int consecutiveSendFailures_{0};
    bool saveDiagnosticFrames_{false};

    // Composite frame state (protected by mutex for thread safety)
    mutable QMutex frameMutex_;
    QImage lastLocalCameraFrame_;
    QMap<QString, QImage> lastRemoteCameraFrames_;
    QImage lastScreenShareFrame_;
    QString screenShareIdentity_;
    QMap<QString, QString> participantNames_;

    // Output
    QString outputDirectory_;
    QString currentOutputPath_;
    QVariantList recentRecordings_;
    QString lastError_;

    // Canvas
    static constexpr int kCanvasWidth = 1920;
    static constexpr int kCanvasHeight = 1080;
};

#endif // LOCAL_RECORDING_MANAGER_H
