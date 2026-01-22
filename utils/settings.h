#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>

class Settings
{
public:
    static Settings& instance();
    
    // Server settings
    QString getServerUrl() const;
    void setServerUrl(const QString& url);
    
    QString getApiUrl() const;
    void setApiUrl(const QString& url);
    
    // User settings
    QString getLastUserName() const;
    void setLastUserName(const QString& name);
    
    QString getLastRoomName() const;
    void setLastRoomName(const QString& name);
    
    // Audio/Video settings
    bool isMicrophoneEnabledByDefault() const;
    void setMicrophoneEnabledByDefault(bool enabled);
    
    bool isCameraEnabledByDefault() const;
    void setCameraEnabledByDefault(bool enabled);
    
    // Audio processing options
    bool isEchoCancellationEnabled() const;
    void setEchoCancellationEnabled(bool enabled);
    
    bool isNoiseSuppressionEnabled() const;
    void setNoiseSuppressionEnabled(bool enabled);
    
    bool isAutoGainControlEnabled() const;
    void setAutoGainControlEnabled(bool enabled);
    
    // Device selection
    QString getSelectedCameraId() const;
    void setSelectedCameraId(const QString& deviceId);
    
    QString getSelectedMicrophoneId() const;
    void setSelectedMicrophoneId(const QString& deviceId);
    
    QString getSelectedSpeakerId() const;
    void setSelectedSpeakerId(const QString& deviceId);
    
    // Signaling server
    QString getSignalingServerUrl() const;
    void setSignalingServerUrl(const QString& url);
    
    // Auth data
    QString getAuthToken() const;
    void setAuthToken(const QString& token);
    QString getUserId() const;
    void setUserId(const QString& userId);
    QString getUserEmail() const;
    void setUserEmail(const QString& email);
    QString getDisplayName() const;
    void setDisplayName(const QString& name);
    bool hasAuthData() const;
    void clearAuthData();
    
    // Force sync to disk
    void sync();
    
private:
    Settings();
    ~Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    
    QSettings settings_;
};

#endif // SETTINGS_H
