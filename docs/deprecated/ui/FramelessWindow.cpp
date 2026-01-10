/* include ---------------------------------------------------------------- 80 // ! ----------------------------- 120 */

#include "FramelessWindow.h"
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <QWindow>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#endif

/* variable --------------------------------------------------------------- 80 // ! ----------------------------- 120 */
constexpr int RESIZE_WINDOW_WIDTH = 8;

/* function --------------------------------------------------------------- 80 // ! ----------------------------- 120 */

FramelessWindow::FramelessWindow(QWidget* parent) : QWidget(parent) {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_TranslucentBackground);

    HWND hwnd = HWND(winId());
    LONG style = ::GetWindowLong(hwnd, GWL_STYLE);
    ::SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX);
    UINT preference = DWMWCP_ROUND;

    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
}

bool FramelessWindow::nativeEvent(const QByteArray& eventType, void* message, qint64* result) {
    MSG* msg = reinterpret_cast <MSG*>(message);

    switch (msg->message) {
        case WM_NCCALCSIZE:
        {
            *result = 0;

            return (true);
        }
        case WM_NCHITTEST:
        {
            long x = GET_X_LPARAM(msg->lParam);
            long y = GET_Y_LPARAM(msg->lParam);
            QPoint mouse_pos(x, y);

            *result = this->adjustResizeWindow(mouse_pos);

            if (0 != *result)
                return (true);
            return (false);
        }
        default:
            return (QWidget::nativeEvent(eventType, message, result));
    }
}

int FramelessWindow::adjustResizeWindow(const QPoint& pos) {
    int result = 0;
    RECT winrect;

    GetWindowRect(HWND(this->winId()), &winrect);

    int mouse_x = pos.x();
    int mouse_y = pos.y();
    bool resizeWidth = this->minimumWidth() != this->maximumWidth();
    bool resizeHieght = this->minimumHeight() != this->maximumHeight();

    if (resizeWidth) {
        if ((mouse_x > winrect.left) && (mouse_x < winrect.left + RESIZE_WINDOW_WIDTH))
            result = HTLEFT;


        if ((mouse_x < winrect.right) && (mouse_x >= winrect.right - RESIZE_WINDOW_WIDTH))
            result = HTRIGHT;
    }

    if (resizeHieght) {
        if ((mouse_y < winrect.top + RESIZE_WINDOW_WIDTH) && (mouse_y >= winrect.top))
            result = HTTOP;


        if ((mouse_y <= winrect.bottom) && (mouse_y > winrect.bottom - RESIZE_WINDOW_WIDTH))
            result = HTBOTTOM;
    }

    if (resizeWidth && resizeHieght) {
        // topleft corner
        if ((mouse_x >= winrect.left) && (mouse_x < winrect.left + RESIZE_WINDOW_WIDTH) && (mouse_y >= winrect.top) && (mouse_y < winrect.top + RESIZE_WINDOW_WIDTH)) {
            result = HTTOPLEFT;
        }

        // topRight corner
        if ((mouse_x <= winrect.right) && (mouse_x > winrect.right - RESIZE_WINDOW_WIDTH) && (mouse_y >= winrect.top) && (mouse_y < winrect.top + RESIZE_WINDOW_WIDTH))
            result = HTTOPRIGHT;

        // leftBottom  corner
        if ((mouse_x >= winrect.left) && (mouse_x < winrect.left + RESIZE_WINDOW_WIDTH) && (mouse_y <= winrect.bottom) && (mouse_y > winrect.bottom - RESIZE_WINDOW_WIDTH))
            result = HTBOTTOMLEFT;

        // rightbottom  corner
        if ((mouse_x <= winrect.right) && (mouse_x > winrect.right - RESIZE_WINDOW_WIDTH) && (mouse_y <= winrect.bottom) && (mouse_y > winrect.bottom - RESIZE_WINDOW_WIDTH))
            result = HTBOTTOMRIGHT;
    }
    return (result);
}

void FramelessWindow::mousePressEvent(QMouseEvent* event) {
    if ((event->button() == Qt::LeftButton) && (event->pos().y() <= 63)) {
        m_dragging = true;
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void FramelessWindow::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void FramelessWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QWidget::mouseReleaseEvent(event);
}
