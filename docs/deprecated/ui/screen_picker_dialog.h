#ifndef SCREEN_PICKER_DIALOG_H
#define SCREEN_PICKER_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QWindow>
#include <QVector>
#include <QRect>
#include <QGuiApplication>
#include <QList>
#include <QPoint>
#include <QMouseEvent>

class ScreenPickerDialog : public QDialog {
    Q_OBJECT
public:
    enum class SelectionType {
        Screen,
        Window,
        Cancel
    };

    explicit ScreenPickerDialog(QWidget* parent = nullptr);

    SelectionType selectionType() const { return selectionType_; }
    QScreen* selectedScreen() const { return selectedScreen_; }
    WId selectedWindow() const { return selectedWindow_; }

    struct WindowInfo {
        QString title;
        WId id{0};
        QRect geometry;
    };

private slots:
    void onAccept();
    void onReject();
    void updateSelection();

private:
    void configureList(QListWidget* list) const;
    void populateScreens();
    void populateWindows();
    QList<WindowInfo> enumerateWindows() const;
    QPixmap grabScreenThumbnail(QScreen* screen) const;
    QPixmap grabWindowThumbnail(const WindowInfo& info) const;
    QPixmap placeholderThumbnail(const QString& label) const;
    QListWidgetItem* currentListItem() const;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    QTabWidget* tabWidget_{nullptr};
    QListWidget* screenList_{nullptr};
    QListWidget* windowList_{nullptr};
    QWidget* titleBar_{nullptr};
    QLabel* titleLabel_{nullptr};
    QPushButton* closeButton_{nullptr};
    QPushButton* refreshWindowsButton_{nullptr};
    QPushButton* shareButton_{nullptr};
    QPushButton* cancelButton_{nullptr};

    QVector<WindowInfo> windows_;
    SelectionType selectionType_{SelectionType::Cancel};
    QScreen* selectedScreen_{nullptr};
    WId selectedWindow_{0};
    bool dragging_{false};
    QPoint dragStartPos_;
};

#endif // SCREEN_PICKER_DIALOG_H
