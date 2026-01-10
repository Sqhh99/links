#ifndef SETTINGSBACKEND_H
#define SETTINGSBACKEND_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

class SettingsBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList microphones READ microphones NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList speakers READ speakers NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList cameras READ cameras NOTIFY devicesChanged)
    Q_PROPERTY(QStringList resolutions READ resolutions CONSTANT)
    Q_PROPERTY(QString selectedMicId READ selectedMicId WRITE setSelectedMicId NOTIFY selectedMicIdChanged)
    Q_PROPERTY(QString selectedSpeakerId READ selectedSpeakerId WRITE setSelectedSpeakerId NOTIFY selectedSpeakerIdChanged)
    Q_PROPERTY(QString selectedCameraId READ selectedCameraId WRITE setSelectedCameraId NOTIFY selectedCameraIdChanged)
    Q_PROPERTY(int selectedResolutionIndex READ selectedResolutionIndex WRITE setSelectedResolutionIndex NOTIFY selectedResolutionIndexChanged)
    Q_PROPERTY(bool echoCancel READ echoCancel WRITE setEchoCancel NOTIFY echoCancelChanged)
    Q_PROPERTY(bool noiseSuppression READ noiseSuppression WRITE setNoiseSuppression NOTIFY noiseSuppressionChanged)
    Q_PROPERTY(bool hardwareAccel READ hardwareAccel WRITE setHardwareAccel NOTIFY hardwareAccelChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)

public:
    explicit SettingsBackend(QObject* parent = nullptr);
    ~SettingsBackend() override = default;

    QVariantList microphones() const { return microphones_; }
    QVariantList speakers() const { return speakers_; }
    QVariantList cameras() const { return cameras_; }
    QStringList resolutions() const { return resolutions_; }
    
    QString selectedMicId() const { return selectedMicId_; }
    void setSelectedMicId(const QString& id);
    
    QString selectedSpeakerId() const { return selectedSpeakerId_; }
    void setSelectedSpeakerId(const QString& id);
    
    QString selectedCameraId() const { return selectedCameraId_; }
    void setSelectedCameraId(const QString& id);
    
    int selectedResolutionIndex() const { return selectedResolutionIndex_; }
    void setSelectedResolutionIndex(int index);
    
    bool echoCancel() const { return echoCancel_; }
    void setEchoCancel(bool enabled);
    
    bool noiseSuppression() const { return noiseSuppression_; }
    void setNoiseSuppression(bool enabled);
    
    bool hardwareAccel() const { return hardwareAccel_; }
    void setHardwareAccel(bool enabled);
    
    QString apiUrl() const { return apiUrl_; }
    void setApiUrl(const QString& url);

    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void save();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE int findDeviceIndex(const QVariantList& devices, const QString& id) const;

signals:
    void devicesChanged();
    void selectedMicIdChanged();
    void selectedSpeakerIdChanged();
    void selectedCameraIdChanged();
    void selectedResolutionIndexChanged();
    void echoCancelChanged();
    void noiseSuppressionChanged();
    void hardwareAccelChanged();
    void apiUrlChanged();
    void accepted();
    void rejected();

private:
    void populateDevices();
    void saveToSettings();
    void loadFromSettings();

    QVariantList microphones_;
    QVariantList speakers_;
    QVariantList cameras_;
    QStringList resolutions_;
    
    QString selectedMicId_;
    QString selectedSpeakerId_;
    QString selectedCameraId_;
    int selectedResolutionIndex_{0};
    bool echoCancel_{true};
    bool noiseSuppression_{true};
    bool hardwareAccel_{true};
    QString apiUrl_;
};

#endif // SETTINGSBACKEND_H
