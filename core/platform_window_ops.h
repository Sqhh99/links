#ifndef CORE_PLATFORM_WINDOW_OPS_H
#define CORE_PLATFORM_WINDOW_OPS_H

#include <QList>
#include <QPixmap>
#include "window_types.h"

namespace links {
namespace core {

QList<WindowInfo> enumerateWindows();
bool bringWindowToForeground(WindowId id);
bool excludeFromCapture(WindowId id);
bool isWindowValid(WindowId id);
bool isWindowMinimized(WindowId id);
QPixmap grabWindowWithWinRt(WindowId id);
QPixmap grabWindowWithPrintApi(WindowId id);

}  // namespace core
}  // namespace links

#endif  // CORE_PLATFORM_WINDOW_OPS_H
