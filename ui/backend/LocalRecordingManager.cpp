#include "LocalRecordingManager.h"

#include "../utils/logger.h"

#include <QAudioInput>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QMutexLocker>
#include <QPainter>
#include <QScreenCapture>
#include <QStandardPaths>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoFrameInput>
#include <algorithm>

namespace {

constexpr int kRecentRecordingLimit = 50;

QString sanitizePathPart(const QString& raw)
{
    QString value = raw.trimmed();
    if (value.isEmpty()) {
        return QStringLiteral("meeting");
    }

    static const QString blockedChars = QStringLiteral("\\/:*?\"<>|");
    for (QChar ch : blockedChars) {
        value.replace(ch, QChar('_'));
    }
    return value;
}

} // namespace

LocalRecordingManager& LocalRecordingManager::instance()
{
    static LocalRecordingManager manager;
    return manager;
}

LocalRecordingManager::LocalRecordingManager(QObject* parent)
    : QObject(parent)
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (baseDir.isEmpty()) {
        baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }

    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath();
    }

    outputDirectory_ = QDir(baseDir).filePath(QStringLiteral("SQLink/Recordings"));

    durationTimer_.setInterval(1000);
    durationTimer_.setSingleShot(false);
    connect(&durationTimer_, &QTimer::timeout, this, &LocalRecordingManager::updateDurationTick);

    compositeTimer_.setSingleShot(false);
    connect(&compositeTimer_, &QTimer::timeout, this, &LocalRecordingManager::compositeAndPushFrame);

    frameLogTimer_.setInterval(5000);
    frameLogTimer_.setSingleShot(false);
    connect(&frameLogTimer_, &QTimer::timeout, this, [this]() {
        if (isRecording_) {
            Logger::instance().debug(QString("Recording compositor: %1 frames pushed, "
                                             "localCam=%2, remotes=%3, screenShare=%4")
                .arg(compositeFrameCount_)
                .arg(!lastLocalCameraFrame_.isNull())
                .arg(lastRemoteCameraFrames_.size())
                .arg(!lastScreenShareFrame_.isNull()));
        }
    });

    refreshRecentRecordings();
}

int LocalRecordingManager::recordingDurationSeconds() const
{
    if (isRecording_ && recordingElapsed_.isValid()) {
        return std::max(0, static_cast<int>(recordingElapsed_.elapsed() / 1000));
    }
    return std::max(0, lastDurationSeconds_);
}

QString LocalRecordingManager::recordingDurationText() const
{
    return formatDuration(recordingDurationSeconds());
}

bool LocalRecordingManager::toggleRecording(const QString& meetingNo,
                                            const QString& userName)
{
    if (isRecording_) {
        stopRecording();
        return true;
    }

    return startConferenceRecording(meetingNo, userName);
}

bool LocalRecordingManager::startConferenceRecording(const QString& meetingNo,
                                                     const QString& userName)
{
    if (isRecording_) {
        return true;
    }

    clearLastError();

    if (!ensureOutputDirectoryReady()) {
        return false;
    }

    Logger::instance().info(QString("Recording canvas: %1x%2")
        .arg(kCanvasWidth).arg(kCanvasHeight));

    // Create objects
    audioInput_ = std::make_unique<QAudioInput>();
    recorder_ = std::make_unique<QMediaRecorder>();

    // Use default constructor for widest backend compatibility.
    videoFrameInput_ = std::make_unique<QVideoFrameInput>();

    // Connect recorder state/error handlers
    connect(recorder_.get(), &QMediaRecorder::recorderStateChanged,
            this, &LocalRecordingManager::onRecorderStateChanged);
    connect(recorder_.get(), &QMediaRecorder::errorChanged,
            this, &LocalRecordingManager::onRecorderErrorChanged);
    connect(videoFrameInput_.get(), &QVideoFrameInput::readyToSendVideoFrame,
            this, [this]() {
                if (isRecording_ && compositeTimer_.isActive()) {
                    compositeAndPushFrame();
                }
            });

    // Wire into capture session
    captureSession_.setAudioInput(audioInput_.get());
    captureSession_.setVideoFrameInput(videoFrameInput_.get());
    captureSession_.setRecorder(recorder_.get());

    // Configure media format explicitly
    QMediaFormat format;
    format.setFileFormat(QMediaFormat::MPEG4);
    format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    recorder_->setMediaFormat(format);
    recorder_->setQuality(QMediaRecorder::HighQuality);
    recorder_->setVideoFrameRate(30);
    recorder_->setVideoResolution(QSize(kCanvasWidth, kCanvasHeight));

    currentOutputPath_ = buildOutputPath(meetingNo, userName);
    emit currentOutputPathChanged();
    recorder_->setOutputLocation(QUrl::fromLocalFile(currentOutputPath_));

    // Start the recorder first; compositor starts after recorder enters RecordingState.
    recorder_->record();
    compositeFrameCount_ = 0;
    frameInputReadyLogPrinted_ = false;
    lastFrameStartUs_ = -1;
    consecutiveSendFailures_ = 0;
    usingScreenCaptureFallback_ = false;

    Logger::instance().info(QString("Local recording started: %1").arg(currentOutputPath_));
    return true;
}

void LocalRecordingManager::stopRecording()
{
    compositeTimer_.stop();
    frameLogTimer_.stop();

    // Clear frame state
    {
        QMutexLocker lock(&frameMutex_);
        lastLocalCameraFrame_ = QImage();
        lastRemoteCameraFrames_.clear();
        lastScreenShareFrame_ = QImage();
        screenShareIdentity_.clear();
    }
    lastFrameStartUs_ = -1;
    consecutiveSendFailures_ = 0;
    usingScreenCaptureFallback_ = false;

    if (!recorder_) {
        return;
    }

    if (recorder_->recorderState() != QMediaRecorder::StoppedState) {
        recorder_->stop();
        return;
    }

    if (isRecording_) {
        onRecorderStateChanged(QMediaRecorder::StoppedState);
    }
}

// =============================================================================
// Frame feed API
// =============================================================================

void LocalRecordingManager::feedLocalCameraFrame(const QImage& frame)
{
    if (!isRecording_ || frame.isNull()) return;
    QMutexLocker lock(&frameMutex_);
    lastLocalCameraFrame_ = frame;
}

void LocalRecordingManager::feedRemoteCameraFrame(const QString& identity,
                                                   const QImage& frame)
{
    if (!isRecording_ || frame.isNull()) return;
    QMutexLocker lock(&frameMutex_);
    lastRemoteCameraFrames_[identity] = frame;
}

void LocalRecordingManager::feedScreenShareFrame(const QString& identity,
                                                  const QImage& frame)
{
    if (!isRecording_ || frame.isNull()) return;
    QMutexLocker lock(&frameMutex_);
    lastScreenShareFrame_ = frame;
    screenShareIdentity_ = identity;
}

void LocalRecordingManager::clearScreenShareFrame()
{
    QMutexLocker lock(&frameMutex_);
    lastScreenShareFrame_ = QImage();
    screenShareIdentity_.clear();
}

void LocalRecordingManager::removeRemoteParticipant(const QString& identity)
{
    QMutexLocker lock(&frameMutex_);
    lastRemoteCameraFrames_.remove(identity);
}

void LocalRecordingManager::setParticipantNames(const QMap<QString, QString>& names)
{
    QMutexLocker lock(&frameMutex_);
    participantNames_ = names;
}

void LocalRecordingManager::openRecordingFolder()
{
    if (!ensureOutputDirectoryReady()) {
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(outputDirectory_))) {
        setLastError(QStringLiteral("无法打开录制文件夹"));
    }
}

void LocalRecordingManager::refreshRecentRecordings()
{
    QVariantList next = scanRecentRecordings();
    if (next == recentRecordings_) {
        return;
    }

    recentRecordings_ = next;
    emit recentRecordingsChanged();
}

void LocalRecordingManager::onRecorderStateChanged(QMediaRecorder::RecorderState state)
{
    if (state == QMediaRecorder::RecordingState) {
        if (!isRecording_) {
            isRecording_ = true;
            lastDurationSeconds_ = 0;
            recordingElapsed_.start();
            durationTimer_.start();
            compositeTimer_.setInterval(33);  // ~30 fps
            compositeTimer_.start();
            frameLogTimer_.start();
            compositeAndPushFrame();  // push first frame as soon as recorder is ready
            emit recordingStateChanged();
            emit recordingDurationChanged();
        }
        return;
    }

    if (state == QMediaRecorder::StoppedState) {
        if (isRecording_) {
            lastDurationSeconds_ = recordingDurationSeconds();
            isRecording_ = false;
            recordingElapsed_.invalidate();
            durationTimer_.stop();
            emit recordingStateChanged();
            emit recordingDurationChanged();
        }

        releaseSessionResources();
        refreshRecentRecordings();

        Logger::instance().info(QString("Local recording stopped: %1").arg(currentOutputPath_));
    }
}

void LocalRecordingManager::onRecorderErrorChanged()
{
    if (!recorder_) {
        return;
    }

    if (recorder_->error() == QMediaRecorder::NoError) {
        return;
    }

    setLastError(recorder_->errorString().isEmpty()
                     ? QStringLiteral("本地录制发生错误")
                     : recorder_->errorString());
    Logger::instance().error(QString("Local recording error: %1").arg(lastError_));
}

void LocalRecordingManager::updateDurationTick()
{
    emit recordingDurationChanged();
}

bool LocalRecordingManager::ensureOutputDirectoryReady()
{
    if (outputDirectory_.isEmpty()) {
        setLastError(QStringLiteral("录制目录不可用"));
        return false;
    }

    QDir dir(outputDirectory_);
    if (dir.exists()) {
        return true;
    }

    if (!dir.mkpath(QStringLiteral("."))) {
        setLastError(QStringLiteral("无法创建录制目录"));
        return false;
    }

    return true;
}

QString LocalRecordingManager::buildOutputPath(const QString& meetingNo,
                                               const QString& userName) const
{
    const QString safeMeetingNo = sanitizePathPart(meetingNo);
    const QString safeUserName = sanitizePathPart(userName);
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    const QString fileName = QStringLiteral("meeting_%1_%2_%3.mp4")
                                 .arg(safeMeetingNo, safeUserName, timestamp);
    return QDir(outputDirectory_).filePath(fileName);
}

// =============================================================================
// Composite rendering
// =============================================================================

void LocalRecordingManager::compositeAndPushFrame()
{
    if (!videoFrameInput_ || !recorder_ || usingScreenCaptureFallback_) {
        return;
    }

    if (recorder_->recorderState() != QMediaRecorder::RecordingState) {
        return;
    }

    // Snapshot the current frame state under lock
    QImage localCam;
    QMap<QString, QImage> remoteCams;
    QImage screenShare;
    QString screenShareId;
    QMap<QString, QString> names;

    {
        QMutexLocker lock(&frameMutex_);
        localCam = lastLocalCameraFrame_;
        remoteCams = lastRemoteCameraFrames_;
        screenShare = lastScreenShareFrame_;
        screenShareId = screenShareIdentity_;
        names = participantNames_;
    }

    // Create canvas using ARGB32 — QPainter's native format on Windows.
    QImage canvas(kCanvasWidth, kCanvasHeight, QImage::Format_ARGB32);
    canvas.fill(QColor(30, 30, 35));  // dark background

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!screenShare.isNull()) {
        // --- Screen-share-dominant layout ---
        // Screen share fills the canvas
        drawParticipantCell(painter, QRect(0, 0, kCanvasWidth, kCanvasHeight),
                            screenShare, QString());

        // Draw camera thumbnails as overlays in the bottom-right
        QList<QPair<QString, QImage>> thumbnails;
        if (!localCam.isNull()) {
            thumbnails.append({names.value(QStringLiteral("local"), QStringLiteral("我")), localCam});
        }
        for (auto it = remoteCams.constBegin(); it != remoteCams.constEnd(); ++it) {
            if (!it.value().isNull()) {
                thumbnails.append({names.value(it.key(), it.key()), it.value()});
            }
        }

        if (!thumbnails.isEmpty()) {
            const int thumbW = 240;
            const int thumbH = 135;
            const int margin = 12;
            const int maxThumbs = 4;
            const int count = qMin(thumbnails.size(), maxThumbs);

            int startX = kCanvasWidth - margin - thumbW;
            int startY = kCanvasHeight - margin - count * (thumbH + margin) + margin;

            for (int i = 0; i < count; ++i) {
                const QRect thumbRect(startX, startY + i * (thumbH + margin), thumbW, thumbH);
                // Draw a border/shadow
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 0, 0, 160));
                painter.drawRoundedRect(thumbRect.adjusted(-2, -2, 2, 2), 6, 6);
                drawParticipantCell(painter, thumbRect,
                                    thumbnails[i].second, thumbnails[i].first);
            }
        }
    } else {
        // --- Grid layout (no screen share) ---
        // Collect all camera participants
        QList<QPair<QString, QImage>> participants;
        if (!localCam.isNull()) {
            participants.append({names.value(QStringLiteral("local"), QStringLiteral("我")), localCam});
        } else {
            // Even without a frame, show local as placeholder
            participants.append({names.value(QStringLiteral("local"), QStringLiteral("我")), QImage()});
        }
        for (auto it = remoteCams.constBegin(); it != remoteCams.constEnd(); ++it) {
            participants.append({names.value(it.key(), it.key()), it.value()});
        }

        const int count = participants.size();
        int cols = 1, rows = 1;
        if (count == 2) {
            cols = 2; rows = 1;
        } else if (count <= 4) {
            cols = 2; rows = 2;
        } else if (count <= 6) {
            cols = 3; rows = 2;
        } else if (count <= 9) {
            cols = 3; rows = 3;
        } else {
            cols = 4; rows = (count + 3) / 4;
        }

        const int cellW = kCanvasWidth / cols;
        const int cellH = kCanvasHeight / rows;

        for (int i = 0; i < count && i < cols * rows; ++i) {
            const int col = i % cols;
            const int row = i / cols;
            const QRect cellRect(col * cellW, row * cellH, cellW, cellH);

            if (!participants[i].second.isNull()) {
                drawParticipantCell(painter, cellRect,
                                    participants[i].second, participants[i].first);
            } else {
                drawPlaceholderCell(painter, cellRect, participants[i].first);
            }
        }
    }

    painter.end();

    // Diagnostic: save the first composited frame as PNG so we can verify
    // that QPainter is actually producing visible content.
    if (compositeFrameCount_ < 3) {
        const QString diagPath = QDir(outputDirectory_).filePath(
            QStringLiteral("_diag_frame_%1.png").arg(compositeFrameCount_));
        canvas.save(diagPath);
        Logger::instance().debug(QString("Diagnostic frame saved: %1 (%2x%3, format=%4)")
            .arg(diagPath)
            .arg(canvas.width()).arg(canvas.height())
            .arg(canvas.format()));
    }

    // Keep an encoder-friendly RGBX buffer.
    QImage rgbx = canvas.convertToFormat(QImage::Format_RGBX8888);

    if (compositeFrameCount_ < 5) {
        Logger::instance().debug(QString(
            "composite frame prepared: src=%1x%2 stride=%3")
            .arg(rgbx.width()).arg(rgbx.height()).arg(rgbx.bytesPerLine()));
    }

    qint64 startUs = static_cast<qint64>(compositeFrameCount_) * 33333;
    if (recordingElapsed_.isValid()) {
        startUs = static_cast<qint64>(recordingElapsed_.elapsed()) * 1000;
    }
    if (lastFrameStartUs_ >= 0 && startUs <= lastFrameStartUs_) {
        startUs = lastFrameStartUs_ + 33333;
    }
    lastFrameStartUs_ = startUs;
    QVideoFrame frame(rgbx);
    frame.setStartTime(startUs);
    frame.setEndTime(startUs + 33333);

    const bool sent = videoFrameInput_->sendVideoFrame(frame);
    if (!sent) {
        ++consecutiveSendFailures_;
        if (!frameInputReadyLogPrinted_ || compositeFrameCount_ % 30 == 0) {
            frameInputReadyLogPrinted_ = true;
            const auto fmt = videoFrameInput_->format();
            Logger::instance().warning(QString("compositeAndPushFrame: sendVideoFrame returned false "
                                               "(failures=%1, recorderState=%2, inputFmt=%3x%4 pix=%5)")
                .arg(consecutiveSendFailures_)
                .arg(static_cast<int>(recorder_->recorderState()))
                .arg(fmt.frameWidth())
                .arg(fmt.frameHeight())
                .arg(static_cast<int>(fmt.pixelFormat())));
        }

        if (!usingScreenCaptureFallback_ && consecutiveSendFailures_ >= 30) {
            Logger::instance().warning("QVideoFrameInput failed continuously, switching to QScreenCapture fallback");

            usingScreenCaptureFallback_ = true;
            compositeTimer_.stop();
            frameLogTimer_.stop();

            captureSession_.setVideoFrameInput(nullptr);
            videoFrameInput_.reset();

            screenCapture_ = std::make_unique<QScreenCapture>();
            captureSession_.setScreenCapture(screenCapture_.get());

            if (QScreen* primary = QGuiApplication::primaryScreen()) {
                screenCapture_->setScreen(primary);
            }

            screenCapture_->setActive(true);
            if (screenCapture_->isActive()) {
                Logger::instance().info("QScreenCapture fallback is active");
            } else {
                Logger::instance().error("QScreenCapture fallback failed to activate");
            }
        }
    } else {
        consecutiveSendFailures_ = 0;
        frameInputReadyLogPrinted_ = false;
    }
    ++compositeFrameCount_;
}

void LocalRecordingManager::drawParticipantCell(QPainter& painter,
                                                 const QRect& cellRect,
                                                 const QImage& frame,
                                                 const QString& name)
{
    // Scale the frame to fill the cell, preserving aspect ratio
    QImage scaled = frame.scaled(cellRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = cellRect.x() + (cellRect.width() - scaled.width()) / 2;
    int y = cellRect.y() + (cellRect.height() - scaled.height()) / 2;

    // Fill background (for letterboxing)
    painter.fillRect(cellRect, QColor(30, 30, 35));
    painter.drawImage(x, y, scaled);

    // Draw name label
    if (!name.isEmpty()) {
        QFont font(QStringLiteral("Microsoft YaHei"), 11);
        font.setWeight(QFont::Medium);
        painter.setFont(font);

        QFontMetrics fm(font);
        const int textWidth = fm.horizontalAdvance(name);
        const int padding = 8;
        const int labelH = fm.height() + padding;
        const int labelW = textWidth + padding * 2;
        const int labelX = cellRect.x() + 8;
        const int labelY = cellRect.bottom() - labelH - 8;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 140));
        painter.drawRoundedRect(labelX, labelY, labelW, labelH, 4, 4);

        painter.setPen(Qt::white);
        painter.drawText(labelX + padding, labelY + fm.ascent() + padding / 2, name);
    }
}

void LocalRecordingManager::drawPlaceholderCell(QPainter& painter,
                                                 const QRect& cellRect,
                                                 const QString& name)
{
    painter.fillRect(cellRect, QColor(45, 45, 50));

    // Draw a circle avatar placeholder
    const int avatarSize = qMin(cellRect.width(), cellRect.height()) / 4;
    const int cx = cellRect.x() + cellRect.width() / 2;
    const int cy = cellRect.y() + cellRect.height() / 2 - 10;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(80, 80, 90));
    painter.drawEllipse(cx - avatarSize / 2, cy - avatarSize / 2, avatarSize, avatarSize);

    // Draw initial letter
    if (!name.isEmpty()) {
        QFont avatarFont(QStringLiteral("Microsoft YaHei"), avatarSize / 2);
        avatarFont.setWeight(QFont::Bold);
        painter.setFont(avatarFont);
        painter.setPen(QColor(200, 200, 210));
        painter.drawText(QRect(cx - avatarSize / 2, cy - avatarSize / 2, avatarSize, avatarSize),
                         Qt::AlignCenter, name.left(1).toUpper());
    }

    // Draw name below avatar
    if (!name.isEmpty()) {
        QFont nameFont(QStringLiteral("Microsoft YaHei"), 13);
        nameFont.setWeight(QFont::Medium);
        painter.setFont(nameFont);
        painter.setPen(QColor(180, 180, 190));
        painter.drawText(QRect(cellRect.x(), cy + avatarSize / 2 + 8,
                               cellRect.width(), 30),
                         Qt::AlignHCenter | Qt::AlignTop, name);
    }
}

void LocalRecordingManager::setLastError(const QString& message)
{
    if (lastError_ == message) {
        return;
    }
    lastError_ = message;
    emit lastErrorChanged();
}

void LocalRecordingManager::clearLastError()
{
    if (lastError_.isEmpty()) {
        return;
    }
    lastError_.clear();
    emit lastErrorChanged();
}

void LocalRecordingManager::releaseSessionResources()
{
    compositeTimer_.stop();
    frameLogTimer_.stop();
    captureSession_.setRecorder(nullptr);
    captureSession_.setVideoFrameInput(nullptr);
    captureSession_.setScreenCapture(nullptr);
    captureSession_.setAudioInput(nullptr);
    recorder_.reset();
    videoFrameInput_.reset();
    screenCapture_.reset();
    audioInput_.reset();
}

QVariantList LocalRecordingManager::scanRecentRecordings() const
{
    QVariantList records;

    QDir dir(outputDirectory_);
    if (!dir.exists()) {
        return records;
    }

    const QFileInfoList files = dir.entryInfoList(
        QStringList() << QStringLiteral("*.mp4")
                      << QStringLiteral("*.mkv")
                      << QStringLiteral("*.mov")
                      << QStringLiteral("*.webm"),
        QDir::Files | QDir::Readable,
        QDir::Time);

    for (const QFileInfo& file : files) {

        QVariantMap item;
        item[QStringLiteral("fileName")] = file.fileName();
        item[QStringLiteral("path")] = file.absoluteFilePath();
        item[QStringLiteral("sizeBytes")] = file.size();
        item[QStringLiteral("sizeText")] = formatFileSize(file.size());
        item[QStringLiteral("createdAt")] = file.lastModified().toMSecsSinceEpoch();
        item[QStringLiteral("createdAtText")] = file.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        records.append(item);

        if (records.size() >= kRecentRecordingLimit) {
            break;
        }
    }

    return records;
}

QString LocalRecordingManager::formatDuration(int totalSeconds)
{
    const int clamped = std::max(0, totalSeconds);
    const int hours = clamped / 3600;
    const int minutes = (clamped % 3600) / 60;
    const int seconds = clamped % 60;

    return QStringLiteral("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

QString LocalRecordingManager::formatFileSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }

    const double kb = static_cast<double>(bytes) / 1024.0;
    if (kb < 1024.0) {
        return QStringLiteral("%1 KB").arg(QString::number(kb, 'f', 1));
    }

    const double mb = kb / 1024.0;
    if (mb < 1024.0) {
        return QStringLiteral("%1 MB").arg(QString::number(mb, 'f', 1));
    }

    const double gb = mb / 1024.0;
    return QStringLiteral("%1 GB").arg(QString::number(gb, 'f', 1));
}
