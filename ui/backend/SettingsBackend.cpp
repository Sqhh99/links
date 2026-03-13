#include "SettingsBackend.h"

#include "../adapters/qt/qt_audio_preview_source.h"
#include "../utils/logger.h"
#include "../utils/settings.h"
#include "../../core/speech/model_scanner.h"
#include "../../core/speech/realtime_transcription_session.h"

#include <QCameraDevice>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QMetaObject>
#include <QUrl>
#include <QVariantMap>

#include <utility>

SettingsBackend::SettingsBackend(QObject* parent)
    : QObject(parent),
      resolutions_({"1280x720", "1920x1080", "640x480"}),
      speechSession_(std::make_unique<links::speech::RealtimeTranscriptionSession>()),
      speechPreviewSource_(std::make_unique<QtAudioPreviewSource>())
{
    speechSession_->setResultCallback([this](const links::speech::TranscriptionResult& result) {
        QMetaObject::invokeMethod(this, [this, text = QString::fromStdString(result.text)]() {
            QString trimmed = text.trimmed();
            if (trimmed.isEmpty()) {
                return;
            }
            if (!speechPreviewText_.isEmpty()) {
                speechPreviewText_.append('\n');
            }
            speechPreviewText_.append(trimmed);
            emit speechPreviewTextChanged();
        }, Qt::QueuedConnection);
    });
    speechSession_->setStatusCallback([this](const std::string& status) {
        QMetaObject::invokeMethod(this, [this, message = QString::fromStdString(status)]() {
            updateSpeechPreviewStatus(message);
        }, Qt::QueuedConnection);
    });

    connect(speechPreviewSource_.get(), &QtAudioPreviewSource::errorOccurred,
            this, [this](const QString& errorMessage) {
        stopSpeechPreview();
        updateSpeechPreviewStatus(errorMessage);
    });
    speechPreviewSource_->setFrameCallback([this](const std::vector<int16_t>& samples, int sampleRate, int channels) {
        if (speechSession_) {
            speechSession_->pushPcm16(samples, sampleRate, channels);
        }
    });

    populateDevices();
    loadFromSettings();
}

SettingsBackend::~SettingsBackend()
{
    stopSpeechPreview();
}

void SettingsBackend::setSelectedMicId(const QString& id)
{
    if (selectedMicId_ != id) {
        selectedMicId_ = id;
        emit selectedMicIdChanged();
        restartSpeechPreviewIfRunning();
    }
}

void SettingsBackend::setSelectedSpeakerId(const QString& id)
{
    if (selectedSpeakerId_ != id) {
        selectedSpeakerId_ = id;
        emit selectedSpeakerIdChanged();
    }
}

void SettingsBackend::setSelectedCameraId(const QString& id)
{
    if (selectedCameraId_ != id) {
        selectedCameraId_ = id;
        emit selectedCameraIdChanged();
    }
}

void SettingsBackend::setSelectedResolutionIndex(int index)
{
    if (selectedResolutionIndex_ != index) {
        selectedResolutionIndex_ = index;
        emit selectedResolutionIndexChanged();
    }
}

void SettingsBackend::setEchoCancel(bool enabled)
{
    if (echoCancel_ != enabled) {
        echoCancel_ = enabled;
        emit echoCancelChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setNoiseSuppression(bool enabled)
{
    if (noiseSuppression_ != enabled) {
        noiseSuppression_ = enabled;
        emit noiseSuppressionChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setAutoGainControl(bool enabled)
{
    if (autoGainControl_ != enabled) {
        autoGainControl_ = enabled;
        emit autoGainControlChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setHardwareAccel(bool enabled)
{
    if (hardwareAccel_ != enabled) {
        hardwareAccel_ = enabled;
        emit hardwareAccelChanged();
    }
}

void SettingsBackend::setApiUrl(const QString& url)
{
    if (apiUrl_ != url) {
        apiUrl_ = url;
        emit apiUrlChanged();
    }
}

void SettingsBackend::setHighPassFilter(bool enabled)
{
    if (highPassFilter_ != enabled) {
        highPassFilter_ = enabled;
        emit highPassFilterChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setNsLevel(int level)
{
    level = qBound(0, level, 3);
    if (nsLevel_ != level) {
        nsLevel_ = level;
        emit nsLevelChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setAgcMode(int mode)
{
    mode = qBound(0, mode, 1);
    if (agcMode_ != mode) {
        agcMode_ = mode;
        emit agcModeChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setFixedDigitalGainDb(double gainDb)
{
    gainDb = qBound(0.0, gainDb, 50.0);
    if (!qFuzzyCompare(fixedDigitalGainDb_, gainDb)) {
        fixedDigitalGainDb_ = gainDb;
        emit fixedDigitalGainDbChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setAdaptiveDigitalMaxGainDb(double maxGainDb)
{
    maxGainDb = qBound(0.0, maxGainDb, 50.0);
    if (!qFuzzyCompare(adaptiveDigitalMaxGainDb_, maxGainDb)) {
        adaptiveDigitalMaxGainDb_ = maxGainDb;
        emit adaptiveDigitalMaxGainDbChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setEchoEnhancedFilter(bool enabled)
{
    if (echoEnhancedFilter_ != enabled) {
        echoEnhancedFilter_ = enabled;
        emit echoEnhancedFilterChanged();
        applySpeechAudioProcessingOptions();
    }
}

void SettingsBackend::setSpeechModelDirectory(const QString& directory)
{
    if (speechModelDirectory_ == directory) {
        return;
    }

    speechModelDirectory_ = directory;
    emit speechModelDirectoryChanged();
    stopSpeechPreview();
    refreshSpeechModels();
}

QUrl SettingsBackend::speechModelDirectoryUrl() const
{
    if (speechModelDirectory_.isEmpty()) {
        return QUrl();
    }
    return QUrl::fromLocalFile(speechModelDirectory_);
}

void SettingsBackend::setSelectedSpeechModelPath(const QString& modelPath)
{
    if (selectedSpeechModelPath_ == modelPath) {
        return;
    }

    selectedSpeechModelPath_ = modelPath;
    emit selectedSpeechModelPathChanged();
    stopSpeechPreview();
}

bool SettingsBackend::speechPreviewAvailable() const
{
    return speechSession_ && speechSession_->isAvailable();
}

void SettingsBackend::refreshDevices()
{
    populateDevices();
}

void SettingsBackend::refreshSpeechModels()
{
    speechModels_.clear();
    const auto models = links::speech::ModelScanner::scanDirectory(speechModelDirectory_.toStdString());
    bool selectedExists = false;
    for (const auto& model : models) {
        QVariantMap map;
        map["name"] = QString::fromStdString(model.displayName);
        map["path"] = QString::fromStdString(model.path);
        map["format"] = QString::fromStdString(model.format);
        if (map["path"].toString() == selectedSpeechModelPath_) {
            selectedExists = true;
        }
        speechModels_.append(map);
    }

    if (!selectedExists) {
        selectedSpeechModelPath_ = speechModels_.isEmpty()
            ? QString()
            : speechModels_.first().toMap().value("path").toString();
        emit selectedSpeechModelPathChanged();
    }

    emit speechModelsChanged();

    if (speechModels_.isEmpty()) {
        updateSpeechPreviewStatus(speechModelDirectory_.isEmpty()
            ? QStringLiteral("请选择模型文件夹")
            : QStringLiteral("所选目录中未发现可用 Whisper 模型"));
    } else {
        updateSpeechPreviewStatus(QStringLiteral("已发现 %1 个模型").arg(speechModels_.size()));
    }
}

void SettingsBackend::startSpeechPreview()
{
    if (!speechPreviewAvailable()) {
        updateSpeechPreviewStatus(QStringLiteral("当前平台未启用本地语音转文字预览"));
        return;
    }
    if (speechPreviewRunning_) {
        return;
    }
    if (selectedSpeechModelPath_.isEmpty()) {
        updateSpeechPreviewStatus(QStringLiteral("请先选择 Whisper 模型"));
        return;
    }
    if (selectedSpeechModelPath_.contains(QStringLiteral(".en."), Qt::CaseInsensitive)
        || selectedSpeechModelPath_.contains(QStringLiteral(".en.bin"), Qt::CaseInsensitive)) {
        updateSpeechPreviewStatus(QStringLiteral("当前选择的是英文专用模型，请改用多语言 Whisper 模型"));
        return;
    }

    applySpeechAudioProcessingOptions();
    if (!speechSession_->loadModel(selectedSpeechModelPath_.toStdString())) {
        updateSpeechPreviewStatus(QStringLiteral("模型加载失败，请检查模型文件是否可用"));
        return;
    }
    if (!speechSession_->start()) {
        return;
    }

    speechPreviewSource_->setDeviceId(selectedMicId_);
    if (!speechPreviewSource_->start()) {
        speechSession_->stop();
        updateSpeechPreviewStatus(QStringLiteral("麦克风预览采集启动失败"));
        return;
    }

    speechPreviewRunning_ = true;
    emit speechPreviewRunningChanged();
    updateSpeechPreviewStatus(QStringLiteral("预览中"));
}

void SettingsBackend::stopSpeechPreview()
{
    if (!speechPreviewRunning_ && (!speechPreviewSource_ || !speechPreviewSource_->isActive())) {
        if (speechSession_) {
            speechSession_->stop();
        }
        return;
    }

    if (speechPreviewSource_) {
        speechPreviewSource_->stop();
    }
    if (speechSession_) {
        speechSession_->stop();
    }

    if (speechPreviewRunning_) {
        speechPreviewRunning_ = false;
        emit speechPreviewRunningChanged();
    }
}

void SettingsBackend::clearSpeechPreviewText()
{
    if (speechPreviewText_.isEmpty()) {
        return;
    }
    speechPreviewText_.clear();
    emit speechPreviewTextChanged();
}

void SettingsBackend::save()
{
    stopSpeechPreview();
    saveToSettings();
    emit accepted();
}

void SettingsBackend::cancel()
{
    stopSpeechPreview();
    loadFromSettings();
    emit rejected();
}

void SettingsBackend::loadSettings()
{
    stopSpeechPreview();
    loadFromSettings();
}

int SettingsBackend::findDeviceIndex(const QVariantList& devices, const QString& id) const
{
    for (int i = 0; i < devices.size(); ++i) {
        const QVariantMap device = devices[i].toMap();
        if (device["id"].toString() == id) {
            return i;
        }
    }
    return -1;
}

int SettingsBackend::findValueIndex(const QVariantList& items, const QString& key, const QString& value) const
{
    for (int i = 0; i < items.size(); ++i) {
        const QVariantMap item = items[i].toMap();
        if (item.value(key).toString() == value) {
            return i;
        }
    }
    return -1;
}

QString SettingsBackend::localPathFromUrl(const QUrl& url) const
{
    return url.toLocalFile();
}

void SettingsBackend::populateDevices()
{
    microphones_.clear();
    speakers_.clear();
    cameras_.clear();

    const auto micList = QMediaDevices::audioInputs();
    for (const auto& device : micList) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(device.id());
        map["name"] = device.description();
        microphones_.append(map);
    }
    if (microphones_.isEmpty()) {
        QVariantMap map;
        map["id"] = "";
        map["name"] = "无可用麦克风";
        microphones_.append(map);
    }

    const auto speakerList = QMediaDevices::audioOutputs();
    for (const auto& device : speakerList) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(device.id());
        map["name"] = device.description();
        speakers_.append(map);
    }
    if (speakers_.isEmpty()) {
        QVariantMap map;
        map["id"] = "";
        map["name"] = "无可用扬声器";
        speakers_.append(map);
    }

    const auto cameraList = QMediaDevices::videoInputs();
    for (const auto& device : cameraList) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(device.id());
        map["name"] = device.description();
        cameras_.append(map);
    }
    if (cameras_.isEmpty()) {
        QVariantMap map;
        map["id"] = "";
        map["name"] = "无可用摄像头";
        cameras_.append(map);
    }

    emit devicesChanged();
}

void SettingsBackend::saveToSettings()
{
    auto& settings = Settings::instance();

    Logger::instance().info("Saving settings from SettingsBackend...");

    settings.setSignalingServerUrl(apiUrl_);
    settings.setSelectedCameraId(selectedCameraId_);
    settings.setSelectedMicrophoneId(selectedMicId_);
    settings.setSelectedSpeakerId(selectedSpeakerId_);

    settings.setEchoCancellationEnabled(echoCancel_);
    settings.setNoiseSuppressionEnabled(noiseSuppression_);
    settings.setAutoGainControlEnabled(autoGainControl_);

    settings.setHighPassFilterEnabled(highPassFilter_);
    settings.setNoiseSuppressionLevel(nsLevel_);
    settings.setGainControlMode(agcMode_);
    settings.setFixedDigitalGainDb(static_cast<float>(fixedDigitalGainDb_));
    settings.setAdaptiveDigitalMaxGainDb(static_cast<float>(adaptiveDigitalMaxGainDb_));
    settings.setEchoEnhancedFilterEnabled(echoEnhancedFilter_);

    settings.setSpeechModelDirectory(speechModelDirectory_);
    settings.setSelectedSpeechModelPath(selectedSpeechModelPath_);
    settings.setSpeechBackend(QStringLiteral("cuda"));

    settings.sync();
    Logger::instance().info("Settings saved successfully");
}

void SettingsBackend::loadFromSettings()
{
    auto& settings = Settings::instance();

    setApiUrl(settings.getSignalingServerUrl());
    setSelectedCameraId(settings.getSelectedCameraId());
    setSelectedMicId(settings.getSelectedMicrophoneId());
    setSelectedSpeakerId(settings.getSelectedSpeakerId());

    setEchoCancel(settings.isEchoCancellationEnabled());
    setNoiseSuppression(settings.isNoiseSuppressionEnabled());
    setAutoGainControl(settings.isAutoGainControlEnabled());

    setHighPassFilter(settings.isHighPassFilterEnabled());
    setNsLevel(settings.noiseSuppressionLevel());
    setAgcMode(settings.gainControlMode());
    setFixedDigitalGainDb(settings.fixedDigitalGainDb());
    setAdaptiveDigitalMaxGainDb(settings.adaptiveDigitalMaxGainDb());
    setEchoEnhancedFilter(settings.isEchoEnhancedFilterEnabled());

    speechModelDirectory_ = settings.getSpeechModelDirectory();
    emit speechModelDirectoryChanged();
    selectedSpeechModelPath_ = settings.getSelectedSpeechModelPath();
    emit selectedSpeechModelPathChanged();
    refreshSpeechModels();
}

void SettingsBackend::restartSpeechPreviewIfRunning()
{
    if (!speechPreviewRunning_) {
        applySpeechAudioProcessingOptions();
        return;
    }

    stopSpeechPreview();
    startSpeechPreview();
}

void SettingsBackend::updateSpeechPreviewStatus(const QString& status)
{
    if (speechPreviewStatus_ == status) {
        return;
    }
    speechPreviewStatus_ = status;
    emit speechPreviewStatusChanged();
}

void SettingsBackend::applySpeechAudioProcessingOptions()
{
    if (!speechSession_) {
        return;
    }

    links::speech::AudioProcessingOptions options;
    // Speech preview has no far-end render reference, so enabling AEC here can
    // distort the microphone signal and hurt recognition quality.
    options.echoCancellationEnabled = false;
    options.noiseSuppressionEnabled = noiseSuppression_;
    options.autoGainControlEnabled = autoGainControl_;
    options.highPassFilterEnabled = highPassFilter_;
    options.noiseSuppressionLevel = nsLevel_;
    options.gainControlMode = agcMode_;
    options.fixedDigitalGainDb = static_cast<float>(fixedDigitalGainDb_);
    options.adaptiveDigitalMaxGainDb = static_cast<float>(adaptiveDigitalMaxGainDb_);
    options.echoEnhancedFilterEnabled = false;
    speechSession_->setAudioProcessingOptions(options);
}
