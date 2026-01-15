#ifndef CORE_THUMBNAIL_SERVICE_H
#define CORE_THUMBNAIL_SERVICE_H

#include <QFutureWatcher>
#include <QImage>
#include <QSize>
#include <QVector>
#include <QObject>
#include <atomic>
#include <memory>
#include "window_types.h"

namespace links {
namespace core {

class ThumbnailService : public QObject {
    Q_OBJECT

public:
    explicit ThumbnailService(QObject* parent = nullptr);
    ~ThumbnailService() override;

    void requestWindowThumbnails(const QVector<WindowInfo>& windows, const QSize& targetSize);
    void cancel();

signals:
    void thumbnailsReady(const QVector<QImage>& thumbnails);

private:
    QImage grabWindowThumbnail(const WindowInfo& info, const QSize& targetSize) const;
    void cancelAndWait();
    void clearWatcher();

    QFutureWatcher<void>* watcher_{nullptr};
};

}  // namespace core
}  // namespace links

#endif  // CORE_THUMBNAIL_SERVICE_H
