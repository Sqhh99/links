#include "ScreenPickerBackend.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QColor>
#include "../../core/platform_window_ops.h"

namespace {
const QSize kThumbSize(240, 140);
} // namespace

ScreenPickerBackend::ScreenPickerBackend(QObject* parent)
    : QObject(parent)
{
    thumbnailService_ = new links::core::ThumbnailService(this);
    connect(thumbnailService_, &links::core::ThumbnailService::thumbnailsReady, this,
            [this](const QVector<QImage>& thumbnails) {
                for (int i = 0; i < windows_.size() && i < thumbnails.size(); ++i) {
                    const QImage& thumb = thumbnails[i];
                    if (!thumb.isNull()) {
                        QVariantMap item = windows_[i].toMap();
                        item["thumbnail"] = thumb;
                        windows_[i] = item;
                    }
                }
                emit windowsChanged();
            });
    // Don't scan windows on construction, only when dialog opens
    // refreshScreens() and refreshWindows() will be called from QML
}

ScreenPickerBackend::~ScreenPickerBackend()
{
}

void ScreenPickerBackend::setCurrentTabIndex(int index)
{
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
    } else {
        return selectedWindowIndex_ >= 0 && selectedWindowIndex_ < windows_.size();
    }
}

QString ScreenPickerBackend::shareButtonText() const
{
    return currentTabIndex_ == 0 ? "共享屏幕" : "共享窗口";
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
    windows_.clear();
    windowInfos_.clear();
    
    const auto windowList = enumerateWindows();
    windowInfos_.reserve(windowList.size());
    
    for (const auto& info : windowList) {
        windowInfos_.append(info);
    }
    
    // First, populate with placeholder thumbnails for immediate UI response
    for (int i = 0; i < windowInfos_.size(); ++i) {
        const auto& info = windowInfos_[i];
        QImage placeholder = placeholderThumbnail(info.title);
        
        QVariantMap item;
        item["index"] = i;
        item["title"] = info.title;
        item["thumbnail"] = placeholder;
        item["tooltip"] = info.title;
        item["windowId"] = static_cast<qulonglong>(info.id);
        windows_.append(item);
    }
    
    if (!windows_.isEmpty() && selectedWindowIndex_ < 0) {
        selectedWindowIndex_ = 0;
        emit selectedWindowIndexChanged();
    }
    
    emit windowsChanged();
    emit selectionChanged();
    
    // Now capture real thumbnails asynchronously
    captureWindowThumbnailsAsync();
}

void ScreenPickerBackend::captureWindowThumbnailsAsync()
{
    if (!thumbnailService_) {
        return;
    }
    thumbnailService_->requestWindowThumbnails(windowInfos_, kThumbSize);
}

QList<ScreenPickerBackend::WindowInfo> ScreenPickerBackend::enumerateWindows() const
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
    } else {
        if (selectedWindowIndex_ >= 0 && selectedWindowIndex_ < windowInfos_.size()) {
            selectedWindowId_ = windowInfos_[selectedWindowIndex_].id;
            if (selectedWindowId_ != 0) {
                selectionType_ = SelectionType::Window;
                // Bring selected window to foreground so it's visible
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
    if (thumbnailService_) {
        thumbnailService_->cancel();
    }
}
