#include "ThemeManager.h"
#include "utils/settings.h"
#include "utils/logger.h"

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
    , currentTheme_(Settings::instance().getTheme())
{
    Logger::instance().info(QString("ThemeManager initialized with theme: %1").arg(currentTheme_));
}

void ThemeManager::setTheme(const QString& theme)
{
    if (theme != "light" && theme != "dark") {
        Logger::instance().warning(QString("Unknown theme: %1, ignoring").arg(theme));
        return;
    }
    
    if (currentTheme_ == theme) return;
    
    currentTheme_ = theme;
    Settings::instance().setTheme(theme);
    emit themeChanged();
    
    Logger::instance().info(QString("Theme changed to: %1").arg(theme));
}
