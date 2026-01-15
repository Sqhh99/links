#ifndef SCREENPICKERBACKEND_H
#define SCREENPICKERBACKEND_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QScreen>
#include <QImage>
#include <QRect>
#include <QVector>
#include <QWindow>
#include "../../core/window_types.h"
#include "../../core/thumbnail_service.h"

class ScreenPickerBackend : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QVariantList screens READ screens NOTIFY screensChanged)
    Q_PROPERTY(QVariantList windows READ windows NOTIFY windowsChanged)
    Q_PROPERTY(int currentTabIndex READ currentTabIndex WRITE setCurrentTabIndex NOTIFY currentTabIndexChanged)
    Q_PROPERTY(int selectedScreenIndex READ selectedScreenIndex WRITE setSelectedScreenIndex NOTIFY selectedScreenIndexChanged)
    Q_PROPERTY(int selectedWindowIndex READ selectedWindowIndex WRITE setSelectedWindowIndex NOTIFY selectedWindowIndexChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(QString shareButtonText READ shareButtonText NOTIFY currentTabIndexChanged)

public:
    enum class SelectionType {
        Screen,
        Window,
        Cancel
    };
    Q_ENUM(SelectionType)
    
    using WindowInfo = links::core::WindowInfo;

    explicit ScreenPickerBackend(QObject* parent = nullptr);
    ~ScreenPickerBackend() override;

    QVariantList screens() const { return screens_; }
    QVariantList windows() const { return windows_; }
    
    int currentTabIndex() const { return currentTabIndex_; }
    void setCurrentTabIndex(int index);
    
    int selectedScreenIndex() const { return selectedScreenIndex_; }
    void setSelectedScreenIndex(int index);
    
    int selectedWindowIndex() const { return selectedWindowIndex_; }
    void setSelectedWindowIndex(int index);
    
    bool hasSelection() const;
    QString shareButtonText() const;
    
    // Selection results
    SelectionType selectionType() const { return selectionType_; }
    QScreen* selectedScreen() const { return selectedScreen_; }
    WId selectedWindow() const { return static_cast<WId>(selectedWindowId_); }

    Q_INVOKABLE void refreshScreens();
    Q_INVOKABLE void refreshWindows();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void cancelPendingOperations();

signals:
    void screensChanged();
    void windowsChanged();
    void currentTabIndexChanged();
    void selectedScreenIndexChanged();
    void selectedWindowIndexChanged();
    void selectionChanged();
    void accepted();
    void rejected();

private:
    QList<WindowInfo> enumerateWindows() const;
    QImage grabScreenThumbnail(QScreen* screen) const;
    QImage placeholderThumbnail(const QString& label) const;
    void captureWindowThumbnailsAsync();
    
    QVariantList screens_;
    QVariantList windows_;
    QVector<WindowInfo> windowInfos_;
    
    int currentTabIndex_{0};
    int selectedScreenIndex_{-1};
    int selectedWindowIndex_{-1};
    
    SelectionType selectionType_{SelectionType::Cancel};
    QScreen* selectedScreen_{nullptr};
    links::core::WindowId selectedWindowId_{0};
    
    links::core::ThumbnailService* thumbnailService_{nullptr};
    
    static constexpr int kThumbWidth = 240;
    static constexpr int kThumbHeight = 140;
};

#endif // SCREENPICKERBACKEND_H
