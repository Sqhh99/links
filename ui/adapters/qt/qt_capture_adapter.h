#ifndef QT_CAPTURE_ADAPTER_H
#define QT_CAPTURE_ADAPTER_H

#include <QImage>
#include <QVariantMap>

#include "../../../core/image_types.h"
#include "../../../core/window_types.h"

namespace links {
namespace qt_adapter {

QImage toQImage(const core::RawImage& image);
QVariantMap makeWindowItem(int index, const core::WindowInfo& info, const QImage& thumbnail);

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_CAPTURE_ADAPTER_H
