#ifndef DESKTOP_CAPTURE_WIN_PLATFORM_WINDOW_OPS_WIN_H_
#define DESKTOP_CAPTURE_WIN_PLATFORM_WINDOW_OPS_WIN_H_

#ifdef _WIN32

#include <QList>
#include <QPixmap>
#include "../../window_types.h"

namespace links {
namespace core {
namespace win {

QList<WindowInfo> enumerateWindows();
bool bringWindowToForeground(WindowId id);
bool excludeFromCapture(WindowId id);
bool isWindowValid(WindowId id);
bool isWindowMinimized(WindowId id);
QPixmap grabWindowWithWinRt(WindowId id);
QPixmap grabWindowWithPrintApi(WindowId id);

}  // namespace win
}  // namespace core
}  // namespace links

#endif  // _WIN32
#endif  // DESKTOP_CAPTURE_WIN_PLATFORM_WINDOW_OPS_WIN_H_
