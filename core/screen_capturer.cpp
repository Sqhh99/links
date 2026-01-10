#include "screen_capturer.h"
#include "../utils/logger.h"
#include "livekit/video_frame.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QRect>
#include <QPoint>
#include <QImage>
#include <QDateTime>
#include <chrono>
#include <algorithm>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <wrl/client.h>
#include <unknwn.h>
#include <ShellScalingApi.h>
#include <DispatcherQueue.h>

// Define interop interface in winrt namespace when not projected
namespace winrt::Windows::Graphics::DirectX::Direct3D11 {
struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")) IDirect3DDxgiInterfaceAccess : ::IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** p) = 0;
};
}

#endif

namespace {
QRect windowRectFromId(WId id) {
#ifdef Q_OS_WIN
    RECT rect;
    if (id != 0 && IsWindow(reinterpret_cast<HWND>(id)) && GetWindowRect(reinterpret_cast<HWND>(id), &rect)) {
        return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    }
#endif
    Q_UNUSED(id);
    return {};
}

#ifdef Q_OS_WIN
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
#endif

QPixmap cropFromScreen(QScreen* screen, const QRect& windowRect)
{
    if (!screen || windowRect.isNull()) {
        return {};
    }
    QRect screenGeo = screen->geometry();
    QRect target = windowRect.intersected(screenGeo);
    if (target.isEmpty()) {
        return {};
    }
    QPixmap screenPix = screen->grabWindow(0);
    if (screenPix.isNull()) {
        return {};
    }
    QRect localRect = target.translated(-screenGeo.topLeft());
    return screenPix.copy(localRect);
}

QPixmap grabWindowPixmap(QScreen* screen, WId id, const QRect& knownRect = {}) {
    if (!screen || id == 0) {
        return {};
    }

    QPixmap pix = screen->grabWindow(id);
    if (!pix.isNull()) {
        return pix;
    }

#ifdef Q_OS_WIN
    QPixmap fallback = grabWindowWithPrintApi(id);
    if (!fallback.isNull()) {
        return fallback;
    }
#endif
    QRect rect = knownRect.isNull() ? windowRectFromId(id) : knownRect;
    QPixmap cropped = cropFromScreen(screen, rect);
    if (!cropped.isNull()) {
        return cropped;
    }
    return pix;
}

#ifdef Q_OS_WIN
RECT windowRectPhysical(HWND hwnd)
{
    RECT rect{};
    if (!hwnd) {
        return rect;
    }
    // Prefer extended frame bounds for accurate sizing
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect)))) {
        // use as-is
    } else {
        GetWindowRect(hwnd, &rect);
    }
    return rect;
}

template<typename T>
winrt::com_ptr<T> GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
{
    auto access = object.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    if (!access) {
        return result;
    }
    HRESULT hr = access->GetInterface(winrt::guid_of<T>(), result.put_void());
    if (FAILED(hr)) {
        result = nullptr;
    }
    return result;
}
#endif
} // namespace

#ifdef Q_OS_WIN
class ScreenCapturer::WinGraphicsWindowCapturer {
public:
    bool init(HWND hwnd);
    bool capture(QImage& outFrame);
    void shutdown();
    void setCopyIntervalMs(int ms) { copyIntervalMs_ = (ms > 0) ? ms : 0; }

private:
    bool createDevice();
    bool recreateFramePool(int width, int height);
    QImage frameToImage(const winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame& frame);
    void onFrameArrived(const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender);
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem createItem(HWND hwnd);

    winrt::com_ptr<ID3D11Device> d3dDevice_;
    winrt::com_ptr<ID3D11DeviceContext> d3dContext_;
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice winrtDevice_{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool_{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item_{ nullptr };
    winrt::com_ptr<ID3D11Texture2D> staging_;
    QSize lastSize_;
    bool initialized_{ false };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker frameArrived_;
    std::mutex frameMutex_;
    QImage latestFrame_;
    bool hasFrame_{ false };
    int copyIntervalMs_{ 0 };
    std::chrono::steady_clock::time_point lastCopy_;
};

bool ScreenCapturer::WinGraphicsWindowCapturer::createDevice()
{
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        d3dDevice_.put(),
        nullptr,
        d3dContext_.put()
    );

    if (FAILED(hr)) {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            flags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            d3dDevice_.put(),
            nullptr,
            d3dContext_.put()
        );
        if (FAILED(hr)) {
            return false;
        }
    }

    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    d3dDevice_.as(dxgiDevice);

    winrt::com_ptr<IInspectable> inspectable;
    hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), reinterpret_cast<IInspectable**>(inspectable.put()));
    if (FAILED(hr)) {
        return false;
    }

    winrtDevice_ = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    return winrtDevice_ != nullptr;
}

bool ScreenCapturer::WinGraphicsWindowCapturer::init(HWND hwnd)
{
    shutdown();

    if (!createDevice()) {
        return false;
    }

    try {
        auto access = winrt::Windows::Graphics::Capture::GraphicsCaptureAccess::RequestAccessAsync(
            winrt::Windows::Graphics::Capture::GraphicsCaptureAccessKind::Borderless);
        access.get();
    } catch (const winrt::hresult_error& e) {
        Logger::instance().warning(QString("WinRT access request failed: 0x%1 %2")
                                   .arg(QString::number(e.code().value, 16))
                                   .arg(QString::fromWCharArray(e.message().c_str())));
    } catch (...) {
        Logger::instance().warning("WinRT access request threw unknown exception");
    }

    item_ = createItem(hwnd);
    if (!item_) {
        Logger::instance().warning("WinRT createItem failed, falling back");
        shutdown();
        return false;
    }

    auto size = item_.Size();
    try {
        framePool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
            winrtDevice_,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            size);
        session_ = framePool_.CreateCaptureSession(item_);
        session_.IsCursorCaptureEnabled(false);
        session_.StartCapture();
        frameArrived_ = framePool_.FrameArrived(
            winrt::auto_revoke,
            [this](const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender,
                   const winrt::Windows::Foundation::IInspectable&) {
                onFrameArrived(sender);
            });
    } catch (const winrt::hresult_error&) {
        shutdown();
        return false;
    } catch (...) {
        shutdown();
        return false;
    }

    lastSize_ = QSize(static_cast<int>(size.Width), static_cast<int>(size.Height));
    initialized_ = true;
    return true;
}

bool ScreenCapturer::WinGraphicsWindowCapturer::recreateFramePool(int width, int height)
{
    if (!winrtDevice_ || !item_) {
        return false;
    }
    try {
    framePool_.Recreate(
        winrtDevice_,
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        1,
        { width, height });
    } catch (const winrt::hresult_error&) {
        return false;
    }
    lastSize_ = QSize(width, height);
    staging_ = nullptr;
    return true;
}

QImage ScreenCapturer::WinGraphicsWindowCapturer::frameToImage(
    const winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame& frame)
{
    if (!frame) {
        return {};
    }

    auto surface = frame.Surface();
    if (!surface) {
        return {};
    }

    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    try {
        auto inspectable = surface.as<winrt::Windows::Foundation::IInspectable>();
        auto tex = GetDXGIInterfaceFromObject<ID3D11Texture2D>(inspectable);
        texture = tex.detach();
        if (!texture) {
            return {};
        }
    } catch (const winrt::hresult_error&) {
        return {};
    }

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    if (!staging_ || desc.Width != static_cast<UINT>(lastSize_.width()) || desc.Height != static_cast<UINT>(lastSize_.height())) {
        D3D11_TEXTURE2D_DESC stagingDesc = desc;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.MiscFlags = 0;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.SampleDesc.Quality = 0;
        hr = d3dDevice_->CreateTexture2D(&stagingDesc, nullptr, staging_.put());
        if (FAILED(hr)) {
            return {};
        }
    }

    d3dContext_->CopyResource(staging_.get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = d3dContext_->Map(staging_.get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        return {};
    }

    QImage bgra(static_cast<uchar*>(mapped.pData),
                static_cast<int>(desc.Width),
                static_cast<int>(desc.Height),
                static_cast<int>(mapped.RowPitch),
                QImage::Format_ARGB32);
    QImage copy = bgra.copy(); // detach from GPU memory
    d3dContext_->Unmap(staging_.get(), 0);

    if (copy.isNull()) {
        return {};
    }
    return copy.convertToFormat(QImage::Format_RGBA8888);
}

bool ScreenCapturer::WinGraphicsWindowCapturer::capture(QImage& outFrame)
{
    if (!initialized_ || !framePool_ || !d3dDevice_ || !d3dContext_) {
        return false;
    }
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (!hasFrame_ || latestFrame_.isNull()) {
        return false;
    }
    outFrame = latestFrame_.copy();
    return true;
}

void ScreenCapturer::WinGraphicsWindowCapturer::onFrameArrived(
    const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender)
{
    try {
        auto frame = sender.TryGetNextFrame();
        if (!frame) {
            return;
        }
        auto contentSize = frame.ContentSize();
        if (contentSize.Width != lastSize_.width() || contentSize.Height != lastSize_.height()) {
            if (!recreateFramePool(contentSize.Width, contentSize.Height)) {
                return;
            }
        }
        auto now = std::chrono::steady_clock::now();
        if (copyIntervalMs_ > 0 && hasFrame_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCopy_).count();
            if (elapsed < copyIntervalMs_) {
                return;
            }
        }
        QImage img = frameToImage(frame);
        if (img.isNull()) {
            return;
        }
        std::lock_guard<std::mutex> lock(frameMutex_);
        latestFrame_ = (img.format() == QImage::Format_RGBA8888) ? img : img.convertToFormat(QImage::Format_RGBA8888);
        hasFrame_ = true;
        lastCopy_ = now;
    } catch (const winrt::hresult_error& e) {
        Logger::instance().warning(QString("WinRT frame error: 0x%1 %2")
                                   .arg(QString::number(e.code().value, 16))
                                   .arg(QString::fromWCharArray(e.message().c_str())));
    } catch (...) {
        Logger::instance().warning("WinRT frame error: unknown");
    }
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem ScreenCapturer::WinGraphicsWindowCapturer::createItem(HWND hwnd)
{
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{ nullptr };
    try {
        winrt::com_ptr<IGraphicsCaptureItemInterop> interop =
            winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem,
                                          IGraphicsCaptureItemInterop>();
        HRESULT hr = interop->CreateForWindow(
            hwnd,
            winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
            reinterpret_cast<void**>(winrt::put_abi(item)));
        if (FAILED(hr)) item = nullptr;
    } catch (...) {
        item = nullptr;
    }
    return item;
}

void ScreenCapturer::WinGraphicsWindowCapturer::shutdown()
{
    session_ = nullptr;
    framePool_ = nullptr;
    item_ = nullptr;
    staging_ = nullptr;
    winrtDevice_ = nullptr;
    d3dContext_ = nullptr;
    d3dDevice_ = nullptr;
    lastSize_ = {};
    initialized_ = false;
    frameArrived_.revoke();
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        latestFrame_ = QImage();
        hasFrame_ = false;
    }
}
#endif // Q_OS_WIN

#ifdef Q_OS_WIN
void ScreenCapturer::runWinRtCapture(HWND hwnd)
{
    try {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
    } catch (...) {
        Logger::instance().warning("WinRT thread: failed to init apartment");
    }

    auto capturer = std::make_unique<WinGraphicsWindowCapturer>();
    capturer->setCopyIntervalMs(1000 / (std::max)(1, fps_));
    if (!capturer->init(hwnd)) {
        Logger::instance().warning("WinRT capture thread init failed; falling back");
        winrtEnabled_ = false;
        winrtRunning_ = false;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(winrtMutex_);
        winWindowCapturer_ = std::move(capturer);
    }
    winrtEnabled_ = true;
    winrtRunning_ = true;
    winrtFailCount_ = 0;

    while (!winrtStop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    {
        std::lock_guard<std::mutex> lock(winrtMutex_);
        if (winWindowCapturer_) {
            winWindowCapturer_->shutdown();
            winWindowCapturer_.reset();
        }
    }
    winrtEnabled_ = false;
    winrtRunning_ = false;
}
#endif

#ifdef Q_OS_WIN
class ScreenCapturer::DxgiWindowDuplicator {
public:
    bool init(HWND hwnd);
    bool capture(QImage& outFrame, bool& noNewFrame);
    void shutdown();

private:
    bool createDevice();
    bool updateOutput();
    bool acquireFrame(bool& softMiss);
    QImage frameToImage();

    HWND hwnd_{nullptr};
    winrt::com_ptr<ID3D11Device> device_;
    winrt::com_ptr<ID3D11DeviceContext> context_;
    winrt::com_ptr<IDXGIOutputDuplication> duplication_;
    winrt::com_ptr<IDXGIOutput> output_;
    winrt::com_ptr<ID3D11Texture2D> lastFrame_;
    DXGI_OUTDUPL_FRAME_INFO frameInfo_{};
    SIZE outputSize_{};
    POINT desktopOrigin_{};
    HMONITOR currentMonitor_{nullptr};
};

bool ScreenCapturer::DxgiWindowDuplicator::createDevice()
{
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        device_.put(),
        nullptr,
        context_.put());

    if (FAILED(hr)) {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            flags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            device_.put(),
            nullptr,
            context_.put());
    }
    return SUCCEEDED(hr) && device_ && context_;
}

bool ScreenCapturer::DxgiWindowDuplicator::updateOutput()
{
    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    try {
        dxgiDevice = device_.as<IDXGIDevice>();
    } catch (...) {
        return false;
    }
    if (!dxgiDevice) {
        return false;
    }
    winrt::com_ptr<IDXGIAdapter> adapter;
    HRESULT hr = dxgiDevice->GetAdapter(adapter.put());
    if (FAILED(hr) || !adapter) {
        return false;
    }
    dxgiDevice = nullptr;
    HMONITOR targetMonitor = hwnd_ ? MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST) : nullptr;
    winrt::com_ptr<IDXGIOutput> output;
    for (UINT i = 0; ; ++i) {
        winrt::com_ptr<IDXGIOutput> out;
        if (adapter->EnumOutputs(i, out.put()) == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (!out) continue;
        DXGI_OUTPUT_DESC desc{};
        if (SUCCEEDED(out->GetDesc(&desc))) {
            if (!targetMonitor || desc.Monitor == targetMonitor) {
                output = out;
                break;
            }
        }
    }
    if (!output) {
        // fallback to first output
        adapter->EnumOutputs(0, output.put());
    }
    if (!output) {
        return false;
    }
    adapter = nullptr;
    output_ = output;
    currentMonitor_ = targetMonitor ? targetMonitor : nullptr;

    winrt::com_ptr<IDXGIOutput1> output1;
    try {
        output1 = output_.as<IDXGIOutput1>();
    } catch (...) {
        output1 = nullptr;
    }
    if (!output1) {
        return false;
    }
    duplication_ = nullptr;
    hr = output1->DuplicateOutput(device_.get(), duplication_.put());
    if (FAILED(hr)) {
        duplication_ = nullptr;
        return false;
    }
    DXGI_OUTPUT_DESC desc{};
    output_->GetDesc(&desc);
    outputSize_.cx = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
    outputSize_.cy = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
    desktopOrigin_.x = desc.DesktopCoordinates.left;
    desktopOrigin_.y = desc.DesktopCoordinates.top;
    if (!currentMonitor_) {
        currentMonitor_ = desc.Monitor;
    }
    return true;
}

bool ScreenCapturer::DxgiWindowDuplicator::init(HWND hwnd)
{
    hwnd_ = hwnd;
    shutdown();
    currentMonitor_ = hwnd_ ? MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST) : nullptr;
    if (!createDevice()) {
        return false;
    }
    return updateOutput();
}

bool ScreenCapturer::DxgiWindowDuplicator::acquireFrame(bool& softMiss)
{
    softMiss = false;
    // Recreate duplication if window moved to another monitor
    if (hwnd_) {
        HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        if (monitor && monitor != currentMonitor_) {
            duplication_ = nullptr;
            output_ = nullptr;
            lastFrame_ = nullptr;
            currentMonitor_ = monitor;
            if (!updateOutput()) {
                return false;
            }
        }
    }

    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!duplication_) {
            if (!updateOutput()) return false;
        }

        frameInfo_ = {};
        winrt::com_ptr<IDXGIResource> resource;
        HRESULT hr = duplication_->AcquireNextFrame(16, &frameInfo_, resource.put());
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            softMiss = true;
            return false; // no new frame yet
        }
        if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL || hr == DXGI_ERROR_DEVICE_REMOVED) {
            // duplication invalid, force recreation
            duplication_ = nullptr;
            output_ = nullptr;
            lastFrame_ = nullptr;
            continue;
        }
        if (FAILED(hr) || !resource) {
            return false;
        }

        lastFrame_ = nullptr;
        resource.as(lastFrame_);
        duplication_->ReleaseFrame();
        return lastFrame_ != nullptr;
    }
    return false;
}

QImage ScreenCapturer::DxgiWindowDuplicator::frameToImage()
{
    if (!lastFrame_) return {};
    D3D11_TEXTURE2D_DESC desc{};
    lastFrame_->GetDesc(&desc);
    if (desc.Width == 0 || desc.Height == 0) {
        return {};
    }

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.MiscFlags = 0;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;

    winrt::com_ptr<ID3D11Texture2D> staging;
    if (FAILED(device_->CreateTexture2D(&stagingDesc, nullptr, staging.put()))) {
        return {};
    }
    context_->CopyResource(staging.get(), lastFrame_.get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context_->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        return {};
    }
    QImage bgra(static_cast<uchar*>(mapped.pData),
                static_cast<int>(desc.Width),
                static_cast<int>(desc.Height),
                static_cast<int>(mapped.RowPitch),
                QImage::Format_ARGB32);
    QImage copy = bgra.copy();
    context_->Unmap(staging.get(), 0);
    return copy.convertToFormat(QImage::Format_RGBA8888);
}

bool ScreenCapturer::DxgiWindowDuplicator::capture(QImage& outFrame, bool& noNewFrame)
{
    noNewFrame = false;
    if (!device_ || !context_) {
        return false;
    }
    bool softMiss = false;
    if (!acquireFrame(softMiss)) {
        noNewFrame = softMiss;
        return softMiss;
    }
    QImage img = frameToImage();
    if (img.isNull()) {
        return false;
    }

    if (hwnd_) {
        RECT rect{};
        if (GetWindowRect(hwnd_, &rect)) {
            int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;
            if (w > 10 && h > 10) {
                // Convert desktop coords to frame coords relative to captured output
                int x = rect.left - desktopOrigin_.x;
                int y = rect.top - desktopOrigin_.y;
                int x0 = (std::max)(0, x);
                int y0 = (std::max)(0, y);
                int x1 = (std::min)(x + w, img.width());
                int y1 = (std::min)(y + h, img.height());
                int width = x1 - x0;
                int height = y1 - y0;
                if (width > 1 && height > 1 && x0 < img.width() && y0 < img.height()) {
                    img = img.copy(x0, y0, width, height);
                }
            }
        }
    }
    outFrame = img;
    return true;
}

void ScreenCapturer::DxgiWindowDuplicator::shutdown()
{
    duplication_ = nullptr;
    output_ = nullptr;
    lastFrame_ = nullptr;
    context_ = nullptr;
    device_ = nullptr;
    hwnd_ = nullptr;
    outputSize_ = {};
    frameInfo_ = {};
    currentMonitor_ = nullptr;
}
#endif


ScreenCapturer::ScreenCapturer(QObject* parent)
    : QObject(parent),
      videoSource_(std::make_shared<livekit::VideoSource>(1280, 720))
{
    lastFrameTime_ = std::chrono::steady_clock::now();
}

ScreenCapturer::~ScreenCapturer()
{
    stop();
}

bool ScreenCapturer::start()
{
    if (isActive_) {
        return true;
    }

    if (!prepareTarget()) {
        return false;
    }

#ifdef Q_OS_WIN
    if (mode_ == Mode::Window) {
        lastWindowRect_ = windowRectFromId(windowId_);
    }
    winrtRunning_ = false;
    winrtEnabled_ = false;
    if (mode_ == Mode::Window) {
        winrtStop_ = false;
        {
            std::lock_guard<std::mutex> lock(winrtMutex_);
            winWindowCapturer_.reset();
        }
        winrtThread_ = std::thread([this]() {
            runWinRtCapture(reinterpret_cast<HWND>(windowId_));
        });
        // Wait shorter time for init to avoid blocking UI
        for (int i = 0; i < 30; ++i) { // wait up to ~300ms for init
            if (winrtRunning_) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (!winrtRunning_) {
            Logger::instance().warning("WinRT capture thread failed to start, falling back to DXGI/legacy.");
            winrtEnabled_ = false;
            winrtFailCount_ = 0;
        }
    }

    if (mode_ == Mode::Window && !winrtRunning_) {
        dxgiDuplicator_ = std::make_unique<DxgiWindowDuplicator>();
        if (!dxgiDuplicator_->init(reinterpret_cast<HWND>(windowId_))) {
            dxgiDuplicator_.reset();
        } else {
            Logger::instance().info("DXGI desktop duplication enabled for window capture");
        }
    }
#endif

    dxgiSoftMissStreak_ = 0;
    dxgiFailCount_ = 0;
    lastFrameTime_ = std::chrono::steady_clock::now();
    pendingResizeReset_ = false;
    lastResizeTime_ = std::chrono::steady_clock::now();
    timer_ = std::make_unique<QTimer>(this);
    timer_->setInterval(1000 / fps_);
    connect(timer_.get(), &QTimer::timeout, this, &ScreenCapturer::captureOnce);
    timer_->start();

    isActive_ = true;
    const char* modeName = mode_ == Mode::Window ? "window" : "screen";
    Logger::instance().info(QString("Screen capture started (%1)").arg(modeName));
    return true;
}

void ScreenCapturer::stop()
{
    if (!isActive_) {
        return;
    }
    if (timer_) {
        timer_->stop();
    }
    timer_.reset();
#ifdef Q_OS_WIN
    winrtStop_ = true;
    // Detach WinRT thread instead of joining to avoid blocking UI
    // The thread will exit on its own when winrtStop_ is true
    if (winrtThread_.joinable()) {
        winrtThread_.detach();
    }
    winrtRunning_ = false;
    winrtEnabled_ = false;
    winrtFailCount_ = 0;
    {
        std::lock_guard<std::mutex> lock(winrtMutex_);
        if (winWindowCapturer_) {
            winWindowCapturer_->shutdown();
            winWindowCapturer_.reset();
        }
    }
    if (dxgiDuplicator_) {
        dxgiDuplicator_->shutdown();
        dxgiDuplicator_.reset();
    }
#endif
    dxgiSoftMissStreak_ = 0;
    dxgiFailCount_ = 0;
    lastFrameTime_ = std::chrono::steady_clock::now();
    pendingResizeReset_ = false;
    lastResizeTime_ = std::chrono::steady_clock::now();
    lastWindowRect_ = {};
    isActive_ = false;
    Logger::instance().info("Screen capture stopped");
}

void ScreenCapturer::captureOnce()
{
    try {
        if (!prepareTarget()) {
            stop();
            return;
        }

#ifdef Q_OS_WIN
        // Handle minimized windows - emit cached frame instead of failing
        if (mode_ == Mode::Window && isWindowMinimized()) {
            if (!lastValidFrame_.isNull()) {
                // Emit the last valid frame while window is minimized
                emit frameCaptured(lastValidFrame_);
                std::vector<uint8_t> frameData(lastValidFrame_.constBits(), 
                    lastValidFrame_.constBits() + lastValidFrame_.sizeInBytes());
                livekit::LKVideoFrame frame(lastValidFrame_.width(), lastValidFrame_.height(),
                    livekit::VideoBufferType::RGBA, std::move(frameData));
                videoSource_->captureFrame(frame, QDateTime::currentMSecsSinceEpoch() * 1000);
            }
            // Don't increment fail counters, just wait for window restore
            return;
        }

        if (mode_ == Mode::Window) {
            QRect currentRect = windowRectFromId(windowId_);
            if (!currentRect.isNull()) {
                if (lastWindowRect_.isNull() || currentRect.size() != lastWindowRect_.size()) {
                    pendingResizeReset_ = true;
                    lastResizeTime_ = std::chrono::steady_clock::now();
                }
                lastWindowRect_ = currentRect;
            }

        }

        if (mode_ == Mode::Window) {
            auto now = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime_).count();
            if (pendingResizeReset_) {
                auto sinceResize = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastResizeTime_).count();
                const int resizeDebounceMs = 350;
                if (sinceResize >= resizeDebounceMs) {
                    Logger::instance().info("Applying debounced capture restart after resize");
                    pendingResizeReset_ = false;
                    // Stop WinRT - detach to avoid blocking UI
                    winrtStop_ = true;
                    if (winrtThread_.joinable()) {
                        winrtThread_.detach();
                    }
                    winrtRunning_ = false;
                    winrtEnabled_ = false;
                    winrtFailCount_ = 0;
                    {
                        std::lock_guard<std::mutex> lock(winrtMutex_);
                        if (winWindowCapturer_) {
                            winWindowCapturer_->shutdown();
                            winWindowCapturer_.reset();
                        }
                    }
                    // Restart WinRT
                    winrtStop_ = false;
                    winrtThread_ = std::thread([this]() {
                        runWinRtCapture(reinterpret_cast<HWND>(windowId_));
                    });
                    // Wait shorter time to avoid blocking UI
                    for (int i = 0; i < 20; ++i) {
                        if (winrtRunning_) break;
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                    // Recreate DXGI duplicator
                    if (!dxgiDuplicator_) {
                        dxgiDuplicator_ = std::make_unique<DxgiWindowDuplicator>();
                    }
                    if (dxgiDuplicator_) {
                        dxgiDuplicator_->shutdown();
                        if (!dxgiDuplicator_->init(reinterpret_cast<HWND>(windowId_))) {
                            dxgiDuplicator_.reset();
                        } else {
                            Logger::instance().info("DXGI duplication reinitialized after resize debounce");
                        }
                    }
                    dxgiSoftMissStreak_ = 0;
                    dxgiFailCount_ = 0;
                    lastFrameTime_ = std::chrono::steady_clock::now();
                    elapsedMs = 0;
                }
            }
            if (elapsedMs > stallRecoverMs_) {
                Logger::instance().warning(QString("Window capture stalled for %1 ms, reinitializing").arg(elapsedMs));
                // Stop WinRT path to avoid stale captures - use detach to avoid blocking
                winrtStop_ = true;
                if (winrtThread_.joinable()) {
                    winrtThread_.detach();
                }
                winrtRunning_ = false;
                winrtEnabled_ = false;
                winrtFailCount_ = 0;
                {
                    std::lock_guard<std::mutex> lock(winrtMutex_);
                    if (winWindowCapturer_) {
                        winWindowCapturer_->shutdown();
                        winWindowCapturer_.reset();
                    }
                }
                // Reinitialize DXGI duplication
                if (!dxgiDuplicator_) {
                    dxgiDuplicator_ = std::make_unique<DxgiWindowDuplicator>();
                }
                if (dxgiDuplicator_) {
                    dxgiDuplicator_->shutdown();
                    if (!dxgiDuplicator_->init(reinterpret_cast<HWND>(windowId_))) {
                        dxgiDuplicator_.reset();
                    } else {
                        Logger::instance().info("DXGI duplication reinitialized after stall");
                    }
                }
                dxgiSoftMissStreak_ = 0;
                dxgiFailCount_ = 0;
                lastFrameTime_ = std::chrono::steady_clock::now();
            }
        }

        if (mode_ == Mode::Window && winrtEnabled_ && winrtRunning_) {
            std::unique_lock<std::mutex> lock(winrtMutex_);
            auto* capturer = winWindowCapturer_.get();
            if (capturer) {
                QImage winrtFrame;
                bool ok = false;
                try {
                    ok = capturer->capture(winrtFrame);
                } catch (const std::exception& e) {
                    Logger::instance().error(QString("WinRT capture exception: %1").arg(e.what()));
                    ok = false;
                }
                if (ok && !winrtFrame.isNull()) {
                    lock.unlock();
                    QImage image = (winrtFrame.format() == QImage::Format_RGBA8888)
                        ? winrtFrame
                        : winrtFrame.convertToFormat(QImage::Format_RGBA8888);
                    try {
                        std::vector<uint8_t> frameData(image.constBits(), 
                            image.constBits() + image.sizeInBytes());
                        livekit::LKVideoFrame frame(image.width(), image.height(),
                            livekit::VideoBufferType::RGBA, std::move(frameData));
                        videoSource_->captureFrame(frame, QDateTime::currentMSecsSinceEpoch() * 1000);
                        emit frameCaptured(image);
                        lastValidFrame_ = image;  // Cache for minimized window handling
                        winrtFailCount_ = 0;
                        lastFrameTime_ = std::chrono::steady_clock::now();
                        return;
                    } catch (const std::exception& e) {
                        Logger::instance().error(QString("WinRT capture frame failed: %1").arg(e.what()));
                    }
                } else {
                    winrtFailCount_++;
                    if (winrtFailCount_ >= 3) {
                        Logger::instance().warning("WinRT capture failed repeatedly, disabling and falling back to DXGI.");
                        capturer->shutdown();
                        winWindowCapturer_.reset();
                        winrtEnabled_ = false;
                        if (!dxgiDuplicator_) {
                            dxgiDuplicator_ = std::make_unique<DxgiWindowDuplicator>();
                            if (dxgiDuplicator_->init(reinterpret_cast<HWND>(windowId_))) {
                                Logger::instance().info("DXGI desktop duplication enabled after WinRT fallback");
                            } else {
                                dxgiDuplicator_.reset();
                            }
                        }
                    }
                }
            }
        }
        // DXGI fallback for window mode
        if (mode_ == Mode::Window) {
                if (dxgiDuplicator_) {
                    QImage dxgiFrame;
                    bool noNewFrame = false;
                    if (dxgiDuplicator_->capture(dxgiFrame, noNewFrame) && !dxgiFrame.isNull()) {
                        QImage image = dxgiFrame.convertToFormat(QImage::Format_RGBA8888);
                        dxgiFailCount_ = 0;
                        dxgiSoftMissStreak_ = 0;
                        std::vector<uint8_t> frameData(image.constBits(), 
                            image.constBits() + image.sizeInBytes());
                        livekit::LKVideoFrame frame(image.width(), image.height(),
                            livekit::VideoBufferType::RGBA, std::move(frameData));
                        videoSource_->captureFrame(frame, QDateTime::currentMSecsSinceEpoch() * 1000);
                        emit frameCaptured(image);
                        lastValidFrame_ = image;  // Cache for minimized window handling
                        lastFrameTime_ = std::chrono::steady_clock::now();
                        return;
                    }
                    if (!noNewFrame) {
                        dxgiFailCount_++;
                        dxgiSoftMissStreak_ = 0;
#ifdef Q_OS_WIN
                        if (dxgiFailCount_ >= 3 && windowId_ != 0) {
                            QPixmap fallbackPix = grabWindowWithPrintApi(windowId_);
                            if (!fallbackPix.isNull()) {
                                QImage image = fallbackPix.toImage().convertToFormat(QImage::Format_RGBA8888);
                                dxgiFailCount_ = 0;
                                std::vector<uint8_t> frameData(image.constBits(), 
                                    image.constBits() + image.sizeInBytes());
                                livekit::LKVideoFrame frame(image.width(), image.height(),
                                    livekit::VideoBufferType::RGBA, std::move(frameData));
                                videoSource_->captureFrame(frame, QDateTime::currentMSecsSinceEpoch() * 1000);
                                emit frameCaptured(image);
                                lastValidFrame_ = image;  // Cache for minimized window handling
                                lastFrameTime_ = std::chrono::steady_clock::now();
                                return;
                            }
                        }
#endif
                        if (dxgiFailCount_ < 10) {
                        Logger::instance().warning("DXGI capture returned no frame, retrying");
                        return;
                    }
                } else {
                    // Soft miss (timeout/no change): track streak; periodically reinit duplication to avoid stalls
                    dxgiSoftMissStreak_++;
                    int softMissThreshold = (std::max)(fps_ * 2, 5); // ~2s worth of misses
                    if (dxgiSoftMissStreak_ >= softMissThreshold) {
                        dxgiSoftMissStreak_ = 0;
                        dxgiFailCount_ = 0;
                        dxgiDuplicator_->shutdown();
                        if (!dxgiDuplicator_->init(reinterpret_cast<HWND>(windowId_))) {
                            dxgiDuplicator_.reset();
                        } else {
                            Logger::instance().info("DXGI duplication reinitialized after consecutive soft misses");
                        }
                        lastFrameTime_ = std::chrono::steady_clock::now();
                    }
                    return;
                }
            }
            // Only stop if window is truly closed, not just minimized
            if (isActive_) {
                bool windowClosed = !validateWindowHandle();
                if (windowClosed) {
                    emit error("窗口已关闭，停止共享");
                    stop();
                } else if (!lastValidFrame_.isNull()) {
                    // Window still valid but capture failed - emit cached frame
                    emit frameCaptured(lastValidFrame_);
                    std::vector<uint8_t> frameData(lastValidFrame_.constBits(), 
                        lastValidFrame_.constBits() + lastValidFrame_.sizeInBytes());
                    livekit::LKVideoFrame frame(lastValidFrame_.width(), lastValidFrame_.height(),
                        livekit::VideoBufferType::RGBA, std::move(frameData));
                    videoSource_->captureFrame(frame, QDateTime::currentMSecsSinceEpoch() * 1000);
                }
            }
            return;
        }
#endif

        // Screen mode or non-Windows fallback
        QPixmap pix;
        if (mode_ == Mode::Screen) {
            pix = screen_->grabWindow(0);
        } else {
            emit error("窗口捕获不可用");
            stop();
            return;
        }

        if (pix.isNull()) {
            Logger::instance().warning("Screen grab returned null pixmap");
            return;
        }

        QImage image = pix.toImage().convertToFormat(QImage::Format_RGBA8888);
        if (image.isNull()) {
            Logger::instance().warning("Screen grab conversion to image failed");
            return;
        }

        try {
            std::vector<uint8_t> frameData(image.constBits(), 
                image.constBits() + image.sizeInBytes());
            livekit::LKVideoFrame frame(image.width(), image.height(),
                livekit::VideoBufferType::RGBA, std::move(frameData));
            videoSource_->captureFrame(frame, QDateTime::currentMSecsSinceEpoch() * 1000);
            emit frameCaptured(image);
            lastFrameTime_ = std::chrono::steady_clock::now();
        } catch (const std::exception& e) {
            Logger::instance().error(QString("Failed to capture screen frame: %1").arg(e.what()));
        }
    } catch (const std::exception& e) {
        Logger::instance().error(QString("captureOnce fatal error: %1").arg(e.what()));
    } catch (...) {
        Logger::instance().error("captureOnce fatal unknown error");
        stop();
    }
}

bool ScreenCapturer::prepareTarget()
{
    if (mode_ == Mode::Window) {
        if (!validateWindowHandle()) {
            emit error("No valid window selected for capture");
            Logger::instance().warning("Invalid window handle for capture");
            return false;
        }
        screen_ = resolveScreenForWindow();
    } else {
        if (!screen_) {
            screen_ = QGuiApplication::primaryScreen();
        }
    }

    if (!screen_) {
        emit error("No screen available for capture");
        return false;
    }
    return true;
}

bool ScreenCapturer::validateWindowHandle() const
{
#ifdef Q_OS_WIN
    return windowId_ != 0 && IsWindow(reinterpret_cast<HWND>(windowId_));
#else
    return windowId_ != 0;
#endif
}

bool ScreenCapturer::isWindowMinimized() const
{
#ifdef Q_OS_WIN
    if (windowId_ == 0) return false;
    HWND hwnd = reinterpret_cast<HWND>(windowId_);
    return IsWindow(hwnd) && IsIconic(hwnd);
#else
    return false;
#endif
}

QScreen* ScreenCapturer::resolveScreenForWindow() const
{
    QRect rect = windowRectFromId(windowId_);
    if (!rect.isNull()) {
        QPoint center = rect.center();
        if (QScreen* target = QGuiApplication::screenAt(center)) {
            return target;
        }
    }
    return QGuiApplication::primaryScreen();
}

