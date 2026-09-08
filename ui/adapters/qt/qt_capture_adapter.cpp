#include "qt_capture_adapter.h"

#include "../../../core/platform_window_ops.h"

#include <QRect>
#include <QScreen>
#include <QString>

#include <memory>

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

QImage toQImage(const core::VideoFrame& frame)
{
    if (!frame.isValid()) {
        return {};
    }

    const QImage::Format format = (frame.format() == core::PixelFormat::BGRA8888)
        ? QImage::Format_ARGB32
        : QImage::Format_RGBA8888;

    // Hold a strong reference to the pixel buffer for as long as the QImage (or
    // any implicitly-shared copy of it) lives.
    auto* keepAlive = new std::shared_ptr<const core::RawImage>(frame.buffer());
    QImage borrowed(
        frame.bits(), frame.width(), frame.height(), frame.stride(), format,
        [](void* p) { delete static_cast<std::shared_ptr<const core::RawImage>*>(p); },
        keepAlive);

    if (frame.format() == core::PixelFormat::BGRA8888) {
        return borrowed.convertToFormat(QImage::Format_RGBA8888);
    }
    return borrowed;
}

namespace {

QString normalizeMonitorName(const QString& name)
{
    QString normalized = name.trimmed().toUpper();
    if (normalized.startsWith("\\\\.\\") || normalized.startsWith("//./")) {
        normalized = normalized.mid(4);
    }
    return normalized;
}

}  // namespace

core::MonitorId resolveMonitorId(const QScreen* screen)
{
    if (!screen) {
        return 0;
    }

    const QString screenName = normalizeMonitorName(screen->name());
    const QRect screenGeometry = screen->geometry();
    const auto monitors = core::enumerateMonitors();

    // Prefer the device-name match, exactly as the old screenSourceId() did.
    for (const auto& monitor : monitors) {
        const QString monitorName =
            normalizeMonitorName(QString::fromStdString(monitor.name));
        if (!monitorName.isEmpty() && monitorName == screenName) {
            return monitor.id;
        }
    }

    // Then fall back to matching geometry.
    for (const auto& monitor : monitors) {
        const QRect bounds(monitor.geometry.x, monitor.geometry.y,
                           monitor.geometry.width, monitor.geometry.height);
        if (bounds == screenGeometry) {
            return monitor.id;
        }
    }

    return 0;
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
