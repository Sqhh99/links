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
