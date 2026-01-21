#include "settings.h"
#include "logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

// Helper function to get the config file path
static QString getConfigFilePath()
{
    // Use AppData/Local/SQLink directory for config file
    // This works even when app is installed in Program Files (which is read-only)
    // Use GenericDataLocation to avoid Qt appending organization/app name automatically
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString configDir = baseDir + "/SQLink";
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return configDir + "/sqlink_config.ini";
}

Settings& Settings::instance()
{
    static Settings instance;
    return instance;
}

Settings::Settings()
    : settings_(getConfigFilePath(), QSettings::IniFormat)
{
    Logger::instance().info(QString("Settings file location: %1").arg(settings_.fileName()));
}

QString Settings::getServerUrl() const
{
    return settings_.value("server/url", "https://sqhh99.dpdns.org:8443").toString();
}

void Settings::setServerUrl(const QString& url)
{
    settings_.setValue("server/url", url);
}

QString Settings::getApiUrl() const
{
    return settings_.value("server/api_url", "http://localhost:8081").toString();
}

void Settings::setApiUrl(const QString& url)
{
    settings_.setValue("server/api_url", url);
}

QString Settings::getLastUserName() const
{
    return settings_.value("user/last_name", "").toString();
}

void Settings::setLastUserName(const QString& name)
{
    settings_.setValue("user/last_name", name);
}

QString Settings::getLastRoomName() const
{
    return settings_.value("user/last_room", "").toString();
}

void Settings::setLastRoomName(const QString& name)
{
    settings_.setValue("user/last_room", name);
}

bool Settings::isMicrophoneEnabledByDefault() const
{
    return settings_.value("media/microphone_enabled", true).toBool();
}

void Settings::setMicrophoneEnabledByDefault(bool enabled)
{
    settings_.setValue("media/microphone_enabled", enabled);
}

bool Settings::isCameraEnabledByDefault() const
{
    return settings_.value("media/camera_enabled", true).toBool();
}

void Settings::setCameraEnabledByDefault(bool enabled)
{
    settings_.setValue("media/camera_enabled", enabled);
}

QString Settings::getSelectedCameraId() const
{
    return settings_.value("device/camera_id", "").toString();
}

void Settings::setSelectedCameraId(const QString& deviceId)
{
    settings_.setValue("device/camera_id", deviceId);
}

QString Settings::getSelectedMicrophoneId() const
{
    return settings_.value("device/microphone_id", "").toString();
}

void Settings::setSelectedMicrophoneId(const QString& deviceId)
{
    settings_.setValue("device/microphone_id", deviceId);
}

QString Settings::getSelectedSpeakerId() const
{
    return settings_.value("device/speaker_id", "").toString();
}

void Settings::setSelectedSpeakerId(const QString& deviceId)
{
    settings_.setValue("device/speaker_id", deviceId);
}

QString Settings::getSignalingServerUrl() const
{
    return settings_.value("server/signaling_url", "https://sqhh99.dpdns.org:8443").toString();
}

void Settings::setSignalingServerUrl(const QString& url)
{
    settings_.setValue("server/signaling_url", url);
    settings_.sync(); // Immediately write to disk
}

void Settings::sync()
{
    settings_.sync();
    Logger::instance().info("Settings synced to disk");
}

// Auth data methods
QString Settings::getAuthToken() const
{
    return settings_.value("auth/token", "").toString();
}

void Settings::setAuthToken(const QString& token)
{
    settings_.setValue("auth/token", token);
    settings_.sync();
}

QString Settings::getUserId() const
{
    return settings_.value("auth/user_id", "").toString();
}

void Settings::setUserId(const QString& userId)
{
    settings_.setValue("auth/user_id", userId);
    settings_.sync();
}

QString Settings::getUserEmail() const
{
    return settings_.value("auth/email", "").toString();
}

void Settings::setUserEmail(const QString& email)
{
    settings_.setValue("auth/email", email);
    settings_.sync();
}

QString Settings::getDisplayName() const
{
    return settings_.value("auth/display_name", "").toString();
}

void Settings::setDisplayName(const QString& name)
{
    settings_.setValue("auth/display_name", name);
    settings_.sync();
}

bool Settings::hasAuthData() const
{
    return !getAuthToken().isEmpty() && !getUserId().isEmpty();
}

void Settings::clearAuthData()
{
    settings_.remove("auth/token");
    settings_.remove("auth/user_id");
    settings_.remove("auth/email");
    settings_.remove("auth/display_name");
    settings_.sync();
    Logger::instance().info("Auth data cleared");
}
