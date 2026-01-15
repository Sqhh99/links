#include "platform_window_ops.h"
#include <QGuiApplication>
#include <QWindow>

#ifdef Q_OS_WIN
#include "desktop_capture/win/platform_window_ops_win.h"
#endif

namespace links {
namespace core {

QList<WindowInfo> enumerateWindows()
{
#ifdef Q_OS_WIN
    return win::enumerateWindows();
#else
    QList<WindowInfo> result;
    const QString unnamedWindow = QString::fromUtf8("\xE6\x9C\xAA\xE5\x91\xBD\xE5\x90\x8D\xE7\xAA\x97\xE5\x8F\xA3");
    const QString keywordThumbnail = QString::fromUtf8("\xE7\xBC\xA9\xE7\x95\xA5\xE5\x9B\xBE");
    const QString keywordInputExperience = QString::fromUtf8("\xE8\xBE\x93\xE5\x85\xA5\xE4\xBD\x93\xE9\xAA\x8C");
    const auto topLevel = QGuiApplication::topLevelWindows();
    for (auto* window : topLevel) {
        if (!window->isVisible()) {
            continue;
        }
        WindowInfo info;
        info.id = static_cast<WindowId>(window->winId());
        info.title = window->title().isEmpty() ? unnamedWindow : window->title();
        QString lower = info.title.toLower();
        if (lower.contains(keywordThumbnail) || lower.contains("thumbnail")
            || lower.contains("windows input experience") || lower.contains(keywordInputExperience)) {
            continue;
        }
        info.geometry = window->geometry();
        result.append(info);
    }
    return result;
#endif
}

bool bringWindowToForeground(WindowId id)
{
#ifdef Q_OS_WIN
    return win::bringWindowToForeground(id);
#else
    Q_UNUSED(id);
    return false;
#endif
}

bool excludeFromCapture(WindowId id)
{
#ifdef Q_OS_WIN
    return win::excludeFromCapture(id);
#else
    Q_UNUSED(id);
    return false;
#endif
}

bool isWindowValid(WindowId id)
{
#ifdef Q_OS_WIN
    return win::isWindowValid(id);
#else
    return id != 0;
#endif
}

bool isWindowMinimized(WindowId id)
{
#ifdef Q_OS_WIN
    return win::isWindowMinimized(id);
#else
    Q_UNUSED(id);
    return false;
#endif
}

QPixmap grabWindowWithWinRt(WindowId id)
{
#ifdef Q_OS_WIN
    return win::grabWindowWithWinRt(id);
#else
    Q_UNUSED(id);
    return {};
#endif
}

QPixmap grabWindowWithPrintApi(WindowId id)
{
#ifdef Q_OS_WIN
    return win::grabWindowWithPrintApi(id);
#else
    Q_UNUSED(id);
    return {};
#endif
}

}  // namespace core
}  // namespace links
