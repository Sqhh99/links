#include "ScreenPickerBackend.h"

#include <QColor>
#include <algorithm>
#include <cstddef>
#include <QFont>
#include <QGuiApplication>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QtConcurrent/QtConcurrent>

#include "../adapters/qt/qt_capture_adapter.h"
#include "../../core/platform_window_ops.h"

namespace {
const QSize kThumbSize(240, 140);
}  // namespace

ScreenPickerBackend::ScreenPickerBackend(QObject* parent)
    : QObject(parent)
{
}

ScreenPickerBackend::~ScreenPickerBackend()
{
    cancelPendingOperations();
}

void ScreenPickerBackend::setCurrentTabIndex(int index)
{
    if (!windowShareSupported() && index == 1) {
        index = 0;
    }

    if (currentTabIndex_ != index) {
        currentTabIndex_ = index;
        emit currentTabIndexChanged();
        emit selectionChanged();
    }
}

void ScreenPickerBackend::setSelectedScreenIndex(int index)
{
    if (selectedScreenIndex_ != index) {
        selectedScreenIndex_ = index;
        emit selectedScreenIndexChanged();
        emit selectionChanged();
    }
}

void ScreenPickerBackend::setSelectedWindowIndex(int index)
{
    if (selectedWindowIndex_ != index) {
        selectedWindowIndex_ = index;
        emit selectedWindowIndexChanged();
        emit selectionChanged();
    }
}

bool ScreenPickerBackend::hasSelection() const
{
    if (currentTabIndex_ == 0) {
        return selectedScreenIndex_ >= 0 && selectedScreenIndex_ < screens_.size();
    }
    if (windowShareSupported()) {
        return selectedWindowIndex_ >= 0 && selectedWindowIndex_ < windows_.size();
    }
    return false;
}

QString ScreenPickerBackend::shareButtonText() const
{
    return currentTabIndex_ == 0 || !windowShareSupported() ? "共享屏幕" : "共享窗口";
}

bool ScreenPickerBackend::windowShareSupported() const
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

void ScreenPickerBackend::refreshScreens()
{
    screens_.clear();
    const auto screenList = QGuiApplication::screens();

    for (int i = 0; i < screenList.size(); ++i) {
        QScreen* screen = screenList[i];
        QImage thumb = grabScreenThumbnail(screen);
        QString label = QString("屏幕 %1  (%2x%3)")
                            .arg(i + 1)
                            .arg(screen->geometry().width())
                            .arg(screen->geometry().height());

        QVariantMap item;
        item["index"] = i;
        item["title"] = label;
        item["thumbnail"] = thumb;
        item["tooltip"] = screen->name();
        screens_.append(item);
    }

    if (!screens_.isEmpty() && selectedScreenIndex_ < 0) {
        selectedScreenIndex_ = 0;
        emit selectedScreenIndexChanged();
    }

    emit screensChanged();
    emit selectionChanged();
}

void ScreenPickerBackend::refreshWindows()
{
    cancelPendingOperations();

    windows_.clear();
    windowInfos_.clear();

    if (!windowShareSupported()) {
        selectedWindowIndex_ = -1;
        emit selectedWindowIndexChanged();
        emit windowsChanged();
        emit selectionChanged();
        return;
    }

    windowInfos_ = enumerateWindows();

    for (int i = 0; i < static_cast<int>(windowInfos_.size()); ++i) {
        const auto& info = windowInfos_[i];
        const QImage placeholder = placeholderThumbnail(QString::fromStdString(info.title));
        windows_.append(links::qt_adapter::makeWindowItem(i, info, placeholder));
    }

    if (!windows_.isEmpty() && selectedWindowIndex_ < 0) {
        selectedWindowIndex_ = 0;
        emit selectedWindowIndexChanged();
    }

    emit windowsChanged();
    emit selectionChanged();

    captureWindowThumbnailsAsync();
}

void ScreenPickerBackend::captureWindowThumbnailsAsync()
{
    if (windowInfos_.empty()) {
        return;
    }

    const std::uint64_t generation = thumbnailGeneration_.fetch_add(1) + 1;
    const std::vector<WindowInfo> windows = windowInfos_;
    const links::core::ImageSize targetSize{kThumbWidth, kThumbHeight};
    clearThumbnailWatcher();

    thumbnailWatcher_ = new QFutureWatcher<ThumbnailBatch>(this);
    connect(thumbnailWatcher_, &QFutureWatcher<ThumbnailBatch>::finished, this, [this, generation]() {
        if (!thumbnailWatcher_) {
            return;
        }

        ThumbnailBatch thumbnails = thumbnailWatcher_->result();
        clearThumbnailWatcher();

        if (generation != thumbnailGeneration_.load()) {
            return;
        }

        applyWindowThumbnails(thumbnails);
    });

    auto future = QtConcurrent::run([windows, targetSize]() {
        links::core::ThumbnailService service;
        return service.captureWindowThumbnails(windows, targetSize);
    });

    thumbnailWatcher_->setFuture(future);
}

void ScreenPickerBackend::applyWindowThumbnails(const ThumbnailBatch& thumbnails)
{
    bool updated = false;
    const int count = std::min(static_cast<int>(windows_.size()), static_cast<int>(thumbnails.size()));

    for (int i = 0; i < count; ++i) {
        if (!thumbnails[static_cast<std::size_t>(i)].has_value()) {
            continue;
        }

        const QImage thumb = links::qt_adapter::toQImage(*thumbnails[static_cast<std::size_t>(i)]);
        if (thumb.isNull()) {
            continue;
        }

        QVariantMap item = windows_[i].toMap();
        item["thumbnail"] = thumb;
        windows_[i] = item;
        updated = true;
    }

    if (updated) {
        emit windowsChanged();
    }
}

void ScreenPickerBackend::clearThumbnailWatcher()
{
    if (!thumbnailWatcher_) {
        return;
    }

    thumbnailWatcher_->disconnect(this);
    thumbnailWatcher_->cancel();
    thumbnailWatcher_->deleteLater();
    thumbnailWatcher_ = nullptr;
}

std::vector<ScreenPickerBackend::WindowInfo> ScreenPickerBackend::enumerateWindows() const
{
    return links::core::enumerateWindows();
}

QImage ScreenPickerBackend::grabScreenThumbnail(QScreen* screen) const
{
    if (!screen) {
        return placeholderThumbnail("屏幕不可用");
    }

    QPixmap pix = screen->grabWindow(0);
    if (pix.isNull()) {
        return placeholderThumbnail(screen->name());
    }

    return pix.scaled(kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation).toImage();
}

QImage ScreenPickerBackend::placeholderThumbnail(const QString& label) const
{
    QPixmap pix(kThumbSize);
    pix.fill(QColor("#181b26"));

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#2f3444"), 2));
    painter.drawRoundedRect(pix.rect().adjusted(1, 1, -2, -2), 10, 10);
    painter.setPen(QColor("#6f7487"));
    painter.setFont(QFont("Segoe UI", 10, QFont::DemiBold));
    painter.drawText(pix.rect().adjusted(6, 6, -6, -6), Qt::AlignCenter, label);
    return pix.toImage();
}

void ScreenPickerBackend::accept()
{
    selectionType_ = SelectionType::Cancel;
    selectedScreen_ = nullptr;
    selectedWindowId_ = 0;

    if (currentTabIndex_ == 0) {
        const auto screenList = QGuiApplication::screens();
        if (selectedScreenIndex_ >= 0 && selectedScreenIndex_ < screenList.size()) {
            selectedScreen_ = screenList[selectedScreenIndex_];
            selectionType_ = SelectionType::Screen;
        }
    } else if (windowShareSupported()) {
        if (selectedWindowIndex_ >= 0
            && selectedWindowIndex_ < static_cast<int>(windowInfos_.size())) {
            selectedWindowId_ = windowInfos_[selectedWindowIndex_].id;
            if (selectedWindowId_ != 0) {
                selectionType_ = SelectionType::Window;
                links::core::bringWindowToForeground(selectedWindowId_);
            }
        }
    }

    if (selectionType_ != SelectionType::Cancel) {
        emit accepted();
    }
}

void ScreenPickerBackend::cancel()
{
    cancelPendingOperations();
    selectionType_ = SelectionType::Cancel;
    selectedScreen_ = nullptr;
    selectedWindowId_ = 0;
    emit rejected();
}

void ScreenPickerBackend::cancelPendingOperations()
{
    thumbnailGeneration_.fetch_add(1);
    clearThumbnailWatcher();
}
