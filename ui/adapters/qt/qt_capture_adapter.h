#ifndef QT_CAPTURE_ADAPTER_H
#define QT_CAPTURE_ADAPTER_H

#include <QImage>

class QScreen;
#include <QVariantMap>

#include "../../../core/image_types.h"
#include "../../../core/media/video_frame.h"
#include "../../../core/window_types.h"

namespace links {
namespace qt_adapter {

QImage toQImage(const core::RawImage& image);

/**
 * Zero-copy for RGBA (every conference frame): the QImage borrows the frame's
 * buffer and keeps it alive through a cleanup handler, so fanning a frame out
 * to VideoRenderer and LocalRecordingManager costs no pixel copies -- the same
 * as QImage's implicit sharing did before.
 */
QImage toQImage(const core::VideoFrame& frame);
/**
 * Resolve a QScreen to the backend-native monitor id the capturers use.
 *
 * This is the QScreen::name()/geometry() -> HMONITOR match that used to live
 * inside core/screen_capturer.cpp; keeping it here is what lets core drop its
 * Qt::Gui dependency. Returns 0 ("primary") when no monitor matches.
 */
core::MonitorId resolveMonitorId(const QScreen* screen);

QVariantMap makeWindowItem(int index, const core::WindowInfo& info, const QImage& thumbnail);

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_CAPTURE_ADAPTER_H
