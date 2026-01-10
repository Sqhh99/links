#include "screen_picker_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QColor>
#include <QImage>
#include <QScrollBar>
#include <QVariant>
#include <chrono>
#include <thread>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowsapp.lib")

// Forward declare interop interface if projection header doesn't place it in winrt namespace
struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")) IDirect3DDxgiInterfaceAccess : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** p) = 0;
};
#endif

namespace {
constexpr int kPreviewRole = Qt::UserRole + 1;
const QSize kThumbSize(260, 160);

bool isGeometryUsable(const QRect& rect) {
    return rect.width() > 80 && rect.height() > 60;
}

#ifdef Q_OS_WIN
template<typename T>
winrt::com_ptr<T> GetDXGIInterfaceFromObject(const winrt::Windows::Foundation::IInspectable& object)
{
    winrt::com_ptr<T> result;
    winrt::com_ptr<IDirect3DDxgiInterfaceAccess> access;
    auto unk = object.as<IUnknown>();
    if (!unk) return result;
    if (FAILED(unk->QueryInterface(__uuidof(IDirect3DDxgiInterfaceAccess), access.put_void())) || !access) {
        return result;
    }
    if (SUCCEEDED(access->GetInterface(winrt::guid_of<T>(), result.put_void()))) {
        return result;
    }
    return nullptr;
}

QPixmap grabWindowWithWinRT(WId id)
{
    static bool winrtInit = false;
    if (!winrtInit) {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (...) {
        }
        winrtInit = true;
    }

    HWND hwnd = reinterpret_cast<HWND>(id);
    if (!hwnd || !IsWindow(hwnd)) {
        return {};
    }

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3
    };

    winrt::com_ptr<ID3D11Device> d3dDevice;
    winrt::com_ptr<ID3D11DeviceContext> d3dContext;
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        d3dDevice.put(),
        nullptr,
        d3dContext.put());
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            d3dDevice.put(),
            nullptr,
            d3dContext.put());
    }
    if (FAILED(hr) || !d3dDevice || !d3dContext) {
        return {};
    }

    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    d3dDevice.as(dxgiDevice);
    if (!dxgiDevice) {
        return {};
    }
    winrt::com_ptr<IInspectable> inspectable;
    hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), reinterpret_cast<IInspectable**>(inspectable.put()));
    if (FAILED(hr) || !inspectable) {
        return {};
    }
    auto winrtDevice = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    if (!winrtDevice) {
        return {};
    }

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{ nullptr };
    try {
        winrt::com_ptr<IGraphicsCaptureItemInterop> interop =
            winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem,
                                          IGraphicsCaptureItemInterop>();
        hr = interop->CreateForWindow(
            hwnd,
            winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
            reinterpret_cast<void**>(winrt::put_abi(item)));
        if (FAILED(hr) || !item) {
            return {};
        }
    } catch (...) {
        return {};
    }

    auto size = item.Size();
    auto framePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
        winrtDevice,
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        1,
        size);
    auto session = framePool.CreateCaptureSession(item);
    session.IsCursorCaptureEnabled(false);
    session.StartCapture();

    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame{ nullptr };
    for (int i = 0; i < 6; ++i) { // up to ~120ms
        frame = framePool.TryGetNextFrame();
        if (frame) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    session.Close();
    framePool.Close();

    if (!frame) {
        return {};
    }

    auto surface = frame.Surface();
    if (!surface) return {};
    auto tex = GetDXGIInterfaceFromObject<ID3D11Texture2D>(surface);
    if (!tex) return {};

    D3D11_TEXTURE2D_DESC desc{};
    tex->GetDesc(&desc);
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.MiscFlags = 0;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;

    winrt::com_ptr<ID3D11Texture2D> staging;
    if (FAILED(d3dDevice->CreateTexture2D(&stagingDesc, nullptr, staging.put()))) {
        return {};
    }
    d3dContext->CopyResource(staging.get(), tex.get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(d3dContext->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        return {};
    }
    QImage bgra(static_cast<uchar*>(mapped.pData),
                static_cast<int>(desc.Width),
                static_cast<int>(desc.Height),
                static_cast<int>(mapped.RowPitch),
                QImage::Format_ARGB32);
    QImage copy = bgra.copy();
    d3dContext->Unmap(staging.get(), 0);
    if (copy.isNull()) return {};
    return QPixmap::fromImage(copy.convertToFormat(QImage::Format_RGBA8888));
}

QPixmap grabWindowWithPrintApi(WId id) {
    HWND hwnd = reinterpret_cast<HWND>(id);
    if (!hwnd || !IsWindow(hwnd)) {
        return {};
    }

    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        return {};
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
        return {};
    }

    HDC hdcWindow = GetWindowDC(hwnd);
    if (!hdcWindow) {
        return {};
    }
    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hbm = CreateDIBSection(hdcMemDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hbm || !bits) {
        if (hbm) DeleteObject(hbm);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwnd, hdcWindow);
        return {};
    }

    HGDIOBJ old = SelectObject(hdcMemDC, hbm);
    BOOL ok = PrintWindow(hwnd, hdcMemDC, PW_RENDERFULLCONTENT);
    if (!ok) {
        SelectObject(hdcMemDC, old);
        DeleteObject(hbm);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwnd, hdcWindow);
        return {};
    }

    QImage image(reinterpret_cast<uchar*>(bits), width, height, QImage::Format_ARGB32);
    QImage copy = image.copy();

    SelectObject(hdcMemDC, old);
    DeleteObject(hbm);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwnd, hdcWindow);

    return QPixmap::fromImage(copy);
}

bool isShareableWindow(HWND hwnd) {
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd)) {
        return false;
    }
    if (IsIconic(hwnd)) {
        return false;
    }
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }
    HWND owner = GetWindow(hwnd, GW_OWNER);
    if (owner != nullptr && IsWindowVisible(owner)) {
        return false;
    }
    RECT rect{};
    if (!GetWindowRect(hwnd, &rect)) {
        return false;
    }
    if ((rect.right - rect.left) < 100 || (rect.bottom - rect.top) < 80) {
        return false;
    }
    wchar_t title[512];
    int len = GetWindowTextW(hwnd, title, 512);
    return len > 0;
}
#endif
} // namespace

#ifdef Q_OS_WIN
static BOOL CALLBACK CollectWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto windows = reinterpret_cast<QVector<ScreenPickerDialog::WindowInfo>*>(lParam);
    if (!windows) {
        return FALSE;
    }

    if (!isShareableWindow(hwnd)) {
        return TRUE;
    }

    ScreenPickerDialog::WindowInfo info;
    info.id = reinterpret_cast<WId>(hwnd);
    RECT rect{};
    if (GetWindowRect(hwnd, &rect)) {
        info.geometry = QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    }
    wchar_t title[512];
    int len = GetWindowTextW(hwnd, title, 512);
    if (len > 0) {
        info.title = QString::fromWCharArray(title, len).trimmed();
    }
    QString lower = info.title.toLower();
    if (info.title.isEmpty()
        || lower.contains("缩略图")
        || lower.contains("thumbnail")
        || lower.contains("windows input experience")
        || lower.contains("输入体验")
        || lower.contains("设置")) {
        return TRUE;
    }

    windows->append(info);
    return TRUE;
}
#endif

ScreenPickerDialog::ScreenPickerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground, true);
    resize(900, 640);
    setModal(true);
    setSizeGripEnabled(true);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* frame = new QWidget(this);
    frame->setObjectName("dialogFrame");
    auto* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(16, 16, 16, 16);
    frameLayout->setSpacing(12);
    rootLayout->addWidget(frame);

    titleBar_ = new QWidget(frame);
    titleBar_->setObjectName("dialogTitleBar");
    titleBar_->setFixedHeight(44);
    auto* titleLayout = new QHBoxLayout(titleBar_);
    titleLayout->setContentsMargins(12, 0, 12, 0);
    titleLayout->setSpacing(8);
    titleLabel_ = new QLabel("选择共享内容", titleBar_);
    titleLabel_->setObjectName("dialogTitle");
    titleLayout->addWidget(titleLabel_);
    titleLayout->addStretch();
    closeButton_ = new QPushButton(titleBar_);
    closeButton_->setObjectName("dialogClose");
    closeButton_->setFixedSize(32, 24);
    closeButton_->setIcon(QIcon(":/icon/close.png"));
    closeButton_->setIconSize(QSize(14, 14));
    titleLayout->addWidget(closeButton_);
    frameLayout->addWidget(titleBar_);

    tabWidget_ = new QTabWidget(frame);
    tabWidget_->setTabPosition(QTabWidget::North);

    auto* screenPage = new QWidget(tabWidget_);
    auto* screenLayout = new QVBoxLayout(screenPage);
    screenLayout->setContentsMargins(0, 0, 0, 0);
    screenLayout->setSpacing(12);
    screenList_ = new QListWidget(screenPage);
    configureList(screenList_);
    screenLayout->addWidget(screenList_, 1);

    auto* windowPage = new QWidget(tabWidget_);
    auto* windowLayout = new QVBoxLayout(windowPage);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(12);
    auto* windowHeader = new QHBoxLayout();
    refreshWindowsButton_ = new QPushButton("刷新", windowPage);
    refreshWindowsButton_->setObjectName("ghostButton");
    refreshWindowsButton_->setFixedWidth(72);
    windowHeader->addStretch();
    windowHeader->addWidget(refreshWindowsButton_);
    windowList_ = new QListWidget(windowPage);
    configureList(windowList_);
    windowLayout->addLayout(windowHeader);
    windowLayout->addWidget(windowList_, 1);

    tabWidget_->addTab(screenPage, "整个屏幕");
    tabWidget_->addTab(windowPage, "窗口");
    frameLayout->addWidget(tabWidget_, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    cancelButton_ = new QPushButton("取消", frame);
    cancelButton_->setObjectName("ghostButton");
    shareButton_ = new QPushButton("开始共享", frame);
    shareButton_->setObjectName("primaryButton");
    shareButton_->setEnabled(false);
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(shareButton_);
    frameLayout->addLayout(buttonLayout);

    populateScreens();
    populateWindows();
    updateSelection();

    connect(closeButton_, &QPushButton::clicked, this, &ScreenPickerDialog::onReject);
    connect(shareButton_, &QPushButton::clicked, this, &ScreenPickerDialog::onAccept);
    connect(cancelButton_, &QPushButton::clicked, this, &ScreenPickerDialog::onReject);
    connect(refreshWindowsButton_, &QPushButton::clicked, this, &ScreenPickerDialog::populateWindows);
    connect(screenList_, &QListWidget::itemSelectionChanged, this, &ScreenPickerDialog::updateSelection);
    connect(windowList_, &QListWidget::itemSelectionChanged, this, &ScreenPickerDialog::updateSelection);
    connect(tabWidget_, &QTabWidget::currentChanged, this, &ScreenPickerDialog::updateSelection);

    QString styleSheet = R"(
        #dialogFrame {
            background-color: #0f1116;
            border-radius: 14px;
            color: #e9ebf1;
        }

        #dialogTitleBar {
            background-color: transparent;
        }

        #dialogTitle {
            font-size: 16px;
            font-weight: 700;
            color: #e9ebf1;
        }

        #dialogClose {
            border: none;
            background: rgba(255,255,255,0.06);
            border-radius: 6px;
        }
        #dialogClose:hover {
            background: rgba(255,82,82,0.18);
        }

        QTabWidget::pane {
            border: none;
        }

        QTabBar::tab {
            padding: 10px 14px;
            color: #8b90a6;
            background: transparent;
            border-bottom: 2px solid transparent;
            margin-right: 12px;
            min-width: 80px;
        }

        QTabBar::tab:selected {
            color: #b9ff5c;
            border-bottom: 2px solid #b9ff5c;
        }

        QListWidget {
            background-color: #131724;
            border: 1px solid #202538;
            border-radius: 12px;
            padding: 10px;
        }

        QListWidget::item {
            margin: 6px;
            padding: 8px;
            border-radius: 12px;
        }

        QListWidget::item:selected {
            background-color: rgba(88, 101, 242, 0.20);
            border: 1px solid #5865f2;
        }

        QScrollBar:vertical {
            background: transparent;
            width: 12px;
            margin: 6px 0 6px 0;
        }
        QScrollBar::handle:vertical {
            background: #555f7a;
            min-height: 36px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background: #6b74a0;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0;
        }

        QPushButton#primaryButton {
            background-color: #6bbf3e;
            color: #0c0f18;
            border: none;
            border-radius: 10px;
            padding: 10px 18px;
            font-weight: 600;
        }

        QPushButton#primaryButton:disabled {
            background-color: #2f3a2f;
            color: #6b6f7a;
        }

        QPushButton#primaryButton:hover:!disabled {
            background-color: #7bd44a;
        }

        QPushButton#ghostButton {
            background-color: transparent;
            color: #c4c7d3;
            border: 1px solid #2a3041;
            border-radius: 10px;
            padding: 10px 16px;
        }

        QPushButton#ghostButton:hover {
            border-color: #3d4560;
        }
    )";

    setStyleSheet(styleSheet);
}

void ScreenPickerDialog::configureList(QListWidget* list) const
{
    if (!list) return;
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setResizeMode(QListView::Adjust);
    list->setViewMode(QListView::IconMode);
    list->setMovement(QListView::Static);
    list->setFlow(QListView::LeftToRight);
    list->setWrapping(true);
    list->setWordWrap(true);
    list->setIconSize(kThumbSize);
    list->setSpacing(12);
    list->setUniformItemSizes(true);
    list->setGridSize(QSize(kThumbSize.width() + 32, kThumbSize.height() + 52));
    list->setMinimumWidth((kThumbSize.width() + 32 + list->spacing()) * 3 + list->spacing());
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    if (list->verticalScrollBar()) {
        list->verticalScrollBar()->setSingleStep(12);
    }
}

void ScreenPickerDialog::populateScreens()
{
    if (!screenList_) return;
    screenList_->clear();
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        QPixmap thumb = grabScreenThumbnail(screen);
        QString label = QString("屏幕 %1  (%2x%3)")
                            .arg(i + 1)
                            .arg(screen->geometry().width())
                            .arg(screen->geometry().height());
        auto* item = new QListWidgetItem(QIcon(thumb), label);
        item->setData(Qt::UserRole, i);
        item->setData(kPreviewRole, thumb.toImage());
        item->setToolTip(screen->name());
        item->setSizeHint(QSize(kThumbSize.width() + 32, kThumbSize.height() + 38));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        screenList_->addItem(item);
    }
    if (screenList_->count() > 0) {
        screenList_->setCurrentRow(0);
    }
    updateSelection();
}

void ScreenPickerDialog::populateWindows()
{
    if (!windowList_) return;
    windowList_->clear();
    windows_.clear();
    const auto windowList = enumerateWindows();
    windows_.reserve(windowList.size());
    for (const auto& info : windowList) {
        windows_.append(info);
    }

    for (const auto& info : windows_) {
        QPixmap thumb = grabWindowThumbnail(info);
        auto* item = new QListWidgetItem(QIcon(thumb), info.title);
        item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(info.id));
        item->setData(kPreviewRole, thumb.toImage());
        item->setToolTip(info.title);
        item->setSizeHint(QSize(kThumbSize.width() + 32, kThumbSize.height() + 38));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        windowList_->addItem(item);
    }

    if (windowList_->count() > 0) {
        windowList_->setCurrentRow(0);
    }
    updateSelection();
}

QList<ScreenPickerDialog::WindowInfo> ScreenPickerDialog::enumerateWindows() const
{
    QList<WindowInfo> result;

#ifdef Q_OS_WIN
    QVector<WindowInfo> collected;
    EnumWindows(&CollectWindowsProc, reinterpret_cast<LPARAM>(&collected));
    for (const auto& info : collected) {
        result.append(info);
    }
#else
    const auto topLevel = QGuiApplication::topLevelWindows();
    for (auto* window : topLevel) {
        if (!window->isVisible()) {
            continue;
        }
        WindowInfo info;
        info.id = window->winId();
        info.title = window->title().isEmpty() ? "未命名窗口" : window->title();
        QString lower = info.title.toLower();
        if (lower.contains("缩略图") || lower.contains("thumbnail") || lower.contains("windows input experience") || lower.contains("输入体验")) {
            continue;
        }
        info.geometry = window->geometry();
        result.append(info);
    }
#endif

    return result;
}

QPixmap ScreenPickerDialog::grabScreenThumbnail(QScreen* screen) const
{
    if (!screen) {
        return placeholderThumbnail("屏幕不可用");
    }

    QPixmap pix = screen->grabWindow(0);
    if (pix.isNull()) {
        return placeholderThumbnail(screen->name());
    }
    return pix.scaled(kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ScreenPickerDialog::grabWindowThumbnail(const WindowInfo& info) const
{
    if (info.id == 0) {
        return placeholderThumbnail(info.title);
    }

#ifdef Q_OS_WIN
    QPixmap winrtPix = grabWindowWithWinRT(info.id);
    if (!winrtPix.isNull()) {
        return winrtPix.scaled(kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
        return placeholderThumbnail(info.title);
    }

    QPixmap pix = screen->grabWindow(info.id);
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
        return placeholderThumbnail(info.title);
    }

    return pix.scaled(kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ScreenPickerDialog::placeholderThumbnail(const QString& label) const
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
    return pix;
}

QListWidgetItem* ScreenPickerDialog::currentListItem() const
{
    if (tabWidget_->currentIndex() == 0 && screenList_) {
        return screenList_->currentItem();
    }
    if (tabWidget_->currentIndex() == 1 && windowList_) {
        return windowList_->currentItem();
    }
    return nullptr;
}

void ScreenPickerDialog::updateSelection()
{
    selectionType_ = (tabWidget_->currentIndex() == 0)
        ? SelectionType::Screen
        : SelectionType::Window;

    if (shareButton_) {
        shareButton_->setText(selectionType_ == SelectionType::Screen ? "共享屏幕" : "共享窗口");
    }

    QListWidgetItem* item = currentListItem();
    if (shareButton_) {
        shareButton_->setEnabled(item != nullptr);
    }
}

void ScreenPickerDialog::onAccept()
{
    selectionType_ = SelectionType::Cancel;

    if (tabWidget_->currentIndex() == 0) {
        auto* item = screenList_ ? screenList_->currentItem() : nullptr;
        if (item) {
            int idx = item->data(Qt::UserRole).toInt();
            const auto screens = QGuiApplication::screens();
            if (idx >= 0 && idx < screens.size()) {
                selectedScreen_ = screens[idx];
                selectionType_ = SelectionType::Screen;
            }
        }
    } else {
        auto* item = windowList_ ? windowList_->currentItem() : nullptr;
        if (item) {
            selectedWindow_ = static_cast<WId>(item->data(Qt::UserRole).toULongLong());
            if (selectedWindow_ != 0) {
                selectionType_ = SelectionType::Window;
            }
        }
    }

    if (selectionType_ != SelectionType::Cancel) {
        accept();
    }
}

void ScreenPickerDialog::onReject()
{
    selectionType_ = SelectionType::Cancel;
    reject();
}

void ScreenPickerDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && titleBar_ && titleBar_->geometry().contains(event->position().toPoint())) {
        dragging_ = true;
        dragStartPos_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void ScreenPickerDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_ && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragStartPos_);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void ScreenPickerDialog::mouseReleaseEvent(QMouseEvent* event)
{
    dragging_ = false;
    QDialog::mouseReleaseEvent(event);
}
