#include "thumbnail_service.h"
#include "platform_window_ops.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QtGui/qwindowdefs.h>
#include <QtConcurrent/QtConcurrent>

namespace links {
namespace core {
namespace {

bool isGeometryUsable(const QRect& rect)
{
    return rect.width() > 80 && rect.height() > 60;
}

}  // namespace

ThumbnailService::ThumbnailService(QObject* parent)
    : QObject(parent)
{
}

ThumbnailService::~ThumbnailService()
{
    cancelAndWait();
}

void ThumbnailService::requestWindowThumbnails(const QVector<WindowInfo>& windows, const QSize& targetSize)
{
    cancelAndWait();

    auto capturedThumbnails = std::make_shared<QVector<QImage>>(windows.size());
    auto cancelled = std::make_shared<std::atomic<bool>>(false);

    watcher_ = new QFutureWatcher<void>(this);

    connect(watcher_, &QFutureWatcher<void>::finished, this,
            [this, capturedThumbnails, cancelled]() {
                if (cancelled->load()) {
                    clearWatcher();
                    return;
                }

                emit thumbnailsReady(*capturedThumbnails);
                clearWatcher();
            });

    auto cancelledPtr = cancelled;
    connect(watcher_, &QFutureWatcher<void>::canceled, this, [cancelledPtr]() {
        cancelledPtr->store(true);
    });

    auto future = QtConcurrent::run([this, windows, targetSize, capturedThumbnails, cancelled]() {
        for (int i = 0; i < windows.size(); ++i) {
            if (cancelled->load()) {
                return;
            }
            (*capturedThumbnails)[i] = grabWindowThumbnail(windows[i], targetSize);
        }
    });

    watcher_->setFuture(future);
}

void ThumbnailService::cancel()
{
    if (!watcher_) {
        return;
    }
    watcher_->cancel();
    watcher_->disconnect();
    watcher_->deleteLater();
    watcher_ = nullptr;
}

QImage ThumbnailService::grabWindowThumbnail(const WindowInfo& info, const QSize& targetSize) const
{
    if (info.id == 0) {
        return {};
    }

#ifdef Q_OS_WIN
    QPixmap winrtPix = grabWindowWithWinRt(info.id);
    if (!winrtPix.isNull()) {
        return winrtPix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation).toImage();
    }
#endif

    QScreen* screen = nullptr;
    if (isGeometryUsable(info.geometry)) {
        screen = QGuiApplication::screenAt(info.geometry.center());
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return {};
    }

    QPixmap pix = screen->grabWindow(static_cast<WId>(info.id));
#ifdef Q_OS_WIN
    if (pix.isNull()) {
        pix = grabWindowWithPrintApi(info.id);
    }
#endif
    if (pix.isNull() && isGeometryUsable(info.geometry)) {
        QRect screenGeo = screen->geometry();
        QRect target = info.geometry.intersected(screenGeo);
        if (!target.isEmpty()) {
            QPixmap screenPix = screen->grabWindow(0);
            if (!screenPix.isNull()) {
                QRect localRect = target.translated(-screenGeo.topLeft());
                pix = screenPix.copy(localRect);
            }
        }
    }
    if (pix.isNull()) {
        return {};
    }

    return pix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation).toImage();
}

void ThumbnailService::clearWatcher()
{
    if (watcher_) {
        watcher_->deleteLater();
        watcher_ = nullptr;
    }
}

void ThumbnailService::cancelAndWait()
{
    if (!watcher_) {
        return;
    }
    watcher_->cancel();
    watcher_->waitForFinished();
    clearWatcher();
}

}  // namespace core
}  // namespace links
