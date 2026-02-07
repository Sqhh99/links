#include "qt_capture_adapter.h"

#include <QString>

namespace links {
namespace qt_adapter {

QImage toQImage(const core::RawImage& image)
{
    if (!image.isValid()) {
        return {};
    }

    QImage::Format format = QImage::Format_RGBA8888;
    if (image.format == core::PixelFormat::BGRA8888) {
        format = QImage::Format_ARGB32;
    }

    QImage wrapped(
        image.pixels.data(),
        image.width,
        image.height,
        image.stride,
        format);

    QImage copy = wrapped.copy();
    if (copy.isNull()) {
        return {};
    }

    if (image.format == core::PixelFormat::BGRA8888) {
        return copy.convertToFormat(QImage::Format_RGBA8888);
    }

    return copy;
}

QVariantMap makeWindowItem(int index, const core::WindowInfo& info, const QImage& thumbnail)
{
    const QString title = QString::fromStdString(info.title);

    QVariantMap item;
    item["index"] = index;
    item["title"] = title;
    item["thumbnail"] = thumbnail;
    item["tooltip"] = title;
    item["windowId"] = static_cast<qulonglong>(info.id);
    return item;
}

}  // namespace qt_adapter
}  // namespace links
