#include "AppearanceManager.h"
#include "utils/settings.h"
#include "utils/logger.h"

AppearanceManager::AppearanceManager(QObject* parent)
    : QObject(parent)
    , autoHideConferenceChrome_(Settings::instance().isAutoHideConferenceChromeEnabled())
{
    Logger::instance().info(QString("AppearanceManager initialized (auto hide conference chrome: %1)")
        .arg(autoHideConferenceChrome_ ? "true" : "false"));
}

void AppearanceManager::setAutoHideConferenceChrome(bool enabled)
{
    if (autoHideConferenceChrome_ == enabled) {
        return;
    }

    autoHideConferenceChrome_ = enabled;
    Settings::instance().setAutoHideConferenceChromeEnabled(enabled);
    emit autoHideConferenceChromeChanged();

    Logger::instance().info(QString("Appearance auto hide conference chrome set to: %1")
        .arg(enabled ? "true" : "false"));
}
