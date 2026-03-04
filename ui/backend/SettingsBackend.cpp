#include "SettingsBackend.h"
#include "../utils/settings.h"
#include "../utils/logger.h"
#include <QMediaDevices>
#include <QCameraDevice>
#include <QAudioDevice>
#include <QVariantMap>

SettingsBackend::SettingsBackend(QObject* parent)
    : QObject(parent),
      resolutions_({"1280x720", "1920x1080", "640x480"})
{
    populateDevices();
    loadFromSettings();
}

void SettingsBackend::setSelectedMicId(const QString& id)
{
    if (selectedMicId_ != id) {
        selectedMicId_ = id;
        emit selectedMicIdChanged();
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
    }
}

void SettingsBackend::setNoiseSuppression(bool enabled)
{
    if (noiseSuppression_ != enabled) {
        noiseSuppression_ = enabled;
        emit noiseSuppressionChanged();
    }
}

void SettingsBackend::setAutoGainControl(bool enabled)
{
    if (autoGainControl_ != enabled) {
        autoGainControl_ = enabled;
        emit autoGainControlChanged();
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
    }
}

void SettingsBackend::setNsLevel(int level)
{
    level = qBound(0, level, 3);
    if (nsLevel_ != level) {
        nsLevel_ = level;
        emit nsLevelChanged();
    }
}

void SettingsBackend::setAgcMode(int mode)
{
    mode = qBound(0, mode, 1);
    if (agcMode_ != mode) {
        agcMode_ = mode;
        emit agcModeChanged();
    }
}

void SettingsBackend::setFixedDigitalGainDb(double gainDb)
{
    gainDb = qBound(0.0, gainDb, 50.0);
    if (!qFuzzyCompare(fixedDigitalGainDb_, gainDb)) {
        fixedDigitalGainDb_ = gainDb;
        emit fixedDigitalGainDbChanged();
    }
}

void SettingsBackend::setAdaptiveDigitalMaxGainDb(double maxGainDb)
{
    maxGainDb = qBound(0.0, maxGainDb, 50.0);
    if (!qFuzzyCompare(adaptiveDigitalMaxGainDb_, maxGainDb)) {
        adaptiveDigitalMaxGainDb_ = maxGainDb;
        emit adaptiveDigitalMaxGainDbChanged();
    }
}

void SettingsBackend::setEchoEnhancedFilter(bool enabled)
{
    if (echoEnhancedFilter_ != enabled) {
        echoEnhancedFilter_ = enabled;
        emit echoEnhancedFilterChanged();
    }
}

void SettingsBackend::refreshDevices()
{
    populateDevices();
}

void SettingsBackend::save()
{
    saveToSettings();
    emit accepted();
}

void SettingsBackend::cancel()
{
    loadFromSettings();  // Revert changes
    emit rejected();
}

void SettingsBackend::loadSettings()
{
    loadFromSettings();
}

int SettingsBackend::findDeviceIndex(const QVariantList& devices, const QString& id) const
{
    for (int i = 0; i < devices.size(); ++i) {
        QVariantMap device = devices[i].toMap();
        if (device["id"].toString() == id) {
            return i;
        }
    }
    return -1;
}

void SettingsBackend::populateDevices()
{
    microphones_.clear();
    speakers_.clear();
    cameras_.clear();
    
    // Populate microphones
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
    
    // Populate speakers
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
    
    // Populate cameras
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
    
    // Audio processing options
    settings.setEchoCancellationEnabled(echoCancel_);
    settings.setNoiseSuppressionEnabled(noiseSuppression_);
    settings.setAutoGainControlEnabled(autoGainControl_);
    
    // Advanced audio processing options
    settings.setHighPassFilterEnabled(highPassFilter_);
    settings.setNoiseSuppressionLevel(nsLevel_);
    settings.setGainControlMode(agcMode_);
    settings.setFixedDigitalGainDb(static_cast<float>(fixedDigitalGainDb_));
    settings.setAdaptiveDigitalMaxGainDb(static_cast<float>(adaptiveDigitalMaxGainDb_));
    settings.setEchoEnhancedFilterEnabled(echoEnhancedFilter_);
    
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
    
    // Audio processing options
    setEchoCancel(settings.isEchoCancellationEnabled());
    setNoiseSuppression(settings.isNoiseSuppressionEnabled());
    setAutoGainControl(settings.isAutoGainControlEnabled());
    
    // Advanced audio processing options
    setHighPassFilter(settings.isHighPassFilterEnabled());
    setNsLevel(settings.noiseSuppressionLevel());
    setAgcMode(settings.gainControlMode());
    setFixedDigitalGainDb(settings.fixedDigitalGainDb());
    setAdaptiveDigitalMaxGainDb(settings.adaptiveDigitalMaxGainDb());
    setEchoEnhancedFilter(settings.isEchoEnhancedFilterEnabled());
}

