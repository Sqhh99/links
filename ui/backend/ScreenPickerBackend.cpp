#include "ScreenPickerBackend.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QColor>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <chrono>
#include <thread>
#include <atomic>

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

struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")) IDirect3DDxgiInterfaceAccess : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** p) = 0;
};
#endif

namespace {
const QSize kThumbSize(240, 140);

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
    for (int i = 0; i < 6; ++i) {
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
    bmi.bmiHeader.biHeight = -height;
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

static BOOL CALLBACK CollectWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto windows = reinterpret_cast<QVector<ScreenPickerBackend::WindowInfo>*>(lParam);
    if (!windows) {
        return FALSE;
    }

    if (!isShareableWindow(hwnd)) {
        return TRUE;
    }

    ScreenPickerBackend::WindowInfo info;
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
} // namespace

ScreenPickerBackend::ScreenPickerBackend(QObject* parent)
    : QObject(parent)
{
    // Don't scan windows on construction, only when dialog opens
    // refreshScreens() and refreshWindows() will be called from QML
}

ScreenPickerBackend::~ScreenPickerBackend()
{
    // Cancel any pending async operations
    if (thumbnailWatcher_) {
        thumbnailWatcher_->cancel();
        thumbnailWatcher_->waitForFinished();
        delete thumbnailWatcher_;
        thumbnailWatcher_ = nullptr;
    }
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
    // Cancel any pending capture
    if (thumbnailWatcher_) {
        thumbnailWatcher_->cancel();
        thumbnailWatcher_->waitForFinished();
        delete thumbnailWatcher_;
        thumbnailWatcher_ = nullptr;
    }
    
    // Capture a copy of window infos for the async task
    QVector<WindowInfo> infos = windowInfos_;
    int count = infos.size();
    
    // Create a shared result container
    auto capturedThumbnails = std::make_shared<QVector<QImage>>(count);
    auto cancelled = std::make_shared<std::atomic<bool>>(false);
    
    thumbnailWatcher_ = new QFutureWatcher<void>(this);
    
    connect(thumbnailWatcher_, &QFutureWatcher<void>::finished, this, [this, capturedThumbnails, cancelled]() {
        if (cancelled->load()) {
            if (thumbnailWatcher_) {
                thumbnailWatcher_->deleteLater();
                thumbnailWatcher_ = nullptr;
            }
            return;
        }
        
        // Update windows_ with captured thumbnails on main thread
        for (int i = 0; i < windows_.size() && i < capturedThumbnails->size(); ++i) {
            const QImage& thumb = (*capturedThumbnails)[i];
            if (!thumb.isNull()) {
                QVariantMap item = windows_[i].toMap();
                item["thumbnail"] = thumb;
                windows_[i] = item;
            }
        }
        
        emit windowsChanged();
        
        if (thumbnailWatcher_) {
            thumbnailWatcher_->deleteLater();
            thumbnailWatcher_ = nullptr;
        }
    });
    
    // Store cancelled flag for cleanup
    auto cancelledPtr = cancelled;
    connect(thumbnailWatcher_, &QFutureWatcher<void>::canceled, this, [cancelledPtr]() {
        cancelledPtr->store(true);
    });
    
    // Start the async capture in background thread
    auto future = QtConcurrent::run([this, infos, capturedThumbnails, cancelled]() {
        for (int i = 0; i < infos.size(); ++i) {
            if (cancelled->load()) {
                return;
            }
            (*capturedThumbnails)[i] = grabWindowThumbnail(infos[i]);
        }
    });
    
    thumbnailWatcher_->setFuture(future);
}

QList<ScreenPickerBackend::WindowInfo> ScreenPickerBackend::enumerateWindows() const
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
        if (lower.contains("缩略图") || lower.contains("thumbnail") || 
            lower.contains("windows input experience") || lower.contains("输入体验")) {
            continue;
        }
        info.geometry = window->geometry();
        result.append(info);
    }
#endif

    return result;
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

QImage ScreenPickerBackend::grabWindowThumbnail(const WindowInfo& info) const
{
    if (info.id == 0) {
        return placeholderThumbnail(info.title);
    }

#ifdef Q_OS_WIN
    QPixmap winrtPix = grabWindowWithWinRT(info.id);
    if (!winrtPix.isNull()) {
        return winrtPix.scaled(kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation).toImage();
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
#ifdef Q_OS_WIN
                // Bring selected window to foreground so it's visible
                HWND hwnd = reinterpret_cast<HWND>(selectedWindowId_);
                if (IsWindow(hwnd) && IsIconic(hwnd)) {
                    ShowWindow(hwnd, SW_RESTORE);
                }
                SetForegroundWindow(hwnd);
#endif
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
    if (thumbnailWatcher_) {
        thumbnailWatcher_->cancel();
        // Don't wait, just disconnect and delete later
        thumbnailWatcher_->disconnect();
        thumbnailWatcher_->deleteLater();
        thumbnailWatcher_ = nullptr;
    }
}
