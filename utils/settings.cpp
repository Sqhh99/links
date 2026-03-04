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

// Audio processing options
bool Settings::isEchoCancellationEnabled() const
{
    return settings_.value("audio/echo_cancellation", true).toBool();
}

void Settings::setEchoCancellationEnabled(bool enabled)
{
    settings_.setValue("audio/echo_cancellation", enabled);
}

bool Settings::isNoiseSuppressionEnabled() const
{
    return settings_.value("audio/noise_suppression", true).toBool();
}

void Settings::setNoiseSuppressionEnabled(bool enabled)
{
    settings_.setValue("audio/noise_suppression", enabled);
}

bool Settings::isAutoGainControlEnabled() const
{
    return settings_.value("audio/auto_gain_control", true).toBool();
}

void Settings::setAutoGainControlEnabled(bool enabled)
{
    settings_.setValue("audio/auto_gain_control", enabled);
}

// Advanced audio processing options
bool Settings::isHighPassFilterEnabled() const
{
    return settings_.value("audio/high_pass_filter", true).toBool();
}

void Settings::setHighPassFilterEnabled(bool enabled)
{
    settings_.setValue("audio/high_pass_filter", enabled);
}

int Settings::noiseSuppressionLevel() const
{
    return qBound(0, settings_.value("audio/ns_level", 1).toInt(), 3); // 1 = Moderate
}

void Settings::setNoiseSuppressionLevel(int level)
{
    settings_.setValue("audio/ns_level", qBound(0, level, 3));
}

int Settings::gainControlMode() const
{
    return qBound(0, settings_.value("audio/agc_mode", 0).toInt(), 1); // 0 = AdaptiveDigital
}

void Settings::setGainControlMode(int mode)
{
    settings_.setValue("audio/agc_mode", qBound(0, mode, 1));
}

float Settings::fixedDigitalGainDb() const
{
    return settings_.value("audio/fixed_digital_gain_db", 0.0).toFloat();
}

void Settings::setFixedDigitalGainDb(float gainDb)
{
    settings_.setValue("audio/fixed_digital_gain_db", qBound(0.0f, gainDb, 50.0f));
}

float Settings::adaptiveDigitalMaxGainDb() const
{
    return settings_.value("audio/adaptive_digital_max_gain_db", 50.0).toFloat();
}

void Settings::setAdaptiveDigitalMaxGainDb(float maxGainDb)
{
    settings_.setValue("audio/adaptive_digital_max_gain_db", qBound(0.0f, maxGainDb, 50.0f));
}

bool Settings::isEchoEnhancedFilterEnabled() const
{
    return settings_.value("audio/echo_enhanced_filter", true).toBool();
}

void Settings::setEchoEnhancedFilterEnabled(bool enabled)
{
    settings_.setValue("audio/echo_enhanced_filter", enabled);
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

QString Settings::getTheme() const
{
    return settings_.value("appearance/theme", "light").toString();
}

void Settings::setTheme(const QString& theme)
{
    settings_.setValue("appearance/theme", theme);
    settings_.sync();
}

bool Settings::isAutoHideConferenceChromeEnabled() const
{
    return settings_.value("appearance/auto_hide_chrome", true).toBool();
}

void Settings::setAutoHideConferenceChromeEnabled(bool enabled)
{
    settings_.setValue("appearance/auto_hide_chrome", enabled);
    settings_.sync();
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
    QString userId = settings_.value("auth/user_id", "").toString();
    if (userId.isEmpty()) {
        userId = settings_.value("auth/userId", "").toString();
    }
    return userId;
}

void Settings::setUserId(const QString& userId)
{
    settings_.setValue("auth/user_id", userId);
    settings_.setValue("auth/userId", userId);
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
    return !getAuthToken().isEmpty()
        && (!getUserId().isEmpty() || !getUserEmail().isEmpty());
}

void Settings::clearAuthData()
{
    settings_.remove("auth/token");
    settings_.remove("auth/user_id");
    settings_.remove("auth/userId");
    settings_.remove("auth/email");
    settings_.remove("auth/display_name");
    settings_.sync();
    Logger::instance().info("Auth data cleared");
}
