#ifndef CORE_WINDOW_TYPES_H
#define CORE_WINDOW_TYPES_H

#include <QRect>
#include <QString>
#include <QtGlobal>

namespace links {
namespace core {

using WindowId = quint64;

struct WindowInfo {
    QString title;
    WindowId id{0};
    QRect geometry;
};

}  // namespace core
}  // namespace links

#endif  // CORE_WINDOW_TYPES_H
