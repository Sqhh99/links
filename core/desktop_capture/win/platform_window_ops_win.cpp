#ifdef _WIN32

#include "platform_window_ops_win.h"
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
#include <QImage>
#include <QVector>
#include <chrono>
#include <thread>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowsapp.lib")

struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")) IDirect3DDxgiInterfaceAccess : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** p) = 0;
};

namespace links {
namespace core {
namespace win {
namespace {

HWND toHwnd(WindowId id)
{
    return reinterpret_cast<HWND>(static_cast<quintptr>(id));
}

bool isShareableWindow(HWND hwnd);

struct EnumContext {
    QVector<WindowInfo>* windows{nullptr};
    const QString* keywordThumbnail{nullptr};
    const QString* keywordInputExperience{nullptr};
    const QString* keywordSettings{nullptr};
};

static BOOL CALLBACK collectWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto* context = reinterpret_cast<EnumContext*>(lParam);
    if (!context || !context->windows) {
        return FALSE;
    }

    if (!isShareableWindow(hwnd)) {
        return TRUE;
    }

    WindowInfo info;
    info.id = static_cast<WindowId>(reinterpret_cast<quintptr>(hwnd));
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
        || lower.contains(*context->keywordThumbnail)
        || lower.contains("thumbnail")
        || lower.contains("windows input experience")
        || lower.contains(*context->keywordInputExperience)
        || lower.contains(*context->keywordSettings)) {
        return TRUE;
    }

    context->windows->append(info);
    return TRUE;
}

bool isShareableWindow(HWND hwnd)
{
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

template<typename T>
winrt::com_ptr<T> getDxgiInterfaceFromObject(const winrt::Windows::Foundation::IInspectable& object)
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

}  // namespace

QList<WindowInfo> enumerateWindows()
{
    QVector<WindowInfo> collected;
    const QString keywordThumbnail = QString::fromUtf8("\xE7\xBC\xA9\xE7\x95\xA5\xE5\x9B\xBE");
    const QString keywordInputExperience = QString::fromUtf8("\xE8\xBE\x93\xE5\x85\xA5\xE4\xBD\x93\xE9\xAA\x8C");
    const QString keywordSettings = QString::fromUtf8("\xE8\xAE\xBE\xE7\xBD\xAE");
    EnumContext context;
    context.windows = &collected;
    context.keywordThumbnail = &keywordThumbnail;
    context.keywordInputExperience = &keywordInputExperience;
    context.keywordSettings = &keywordSettings;
    EnumWindows(&collectWindowsProc, reinterpret_cast<LPARAM>(&context));

    QList<WindowInfo> result;
    result.reserve(collected.size());
    for (const auto& info : collected) {
        result.append(info);
    }
    return result;
}

bool bringWindowToForeground(WindowId id)
{
    HWND hwnd = toHwnd(id);
    if (!IsWindow(hwnd)) {
        return false;
    }
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }
    return SetForegroundWindow(hwnd) != 0;
}

bool excludeFromCapture(WindowId id)
{
    HWND hwnd = toHwnd(id);
    if (!hwnd) {
        return false;
    }

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

    return SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE) != 0;
}

bool isWindowValid(WindowId id)
{
    HWND hwnd = toHwnd(id);
    return hwnd != nullptr && IsWindow(hwnd);
}

bool isWindowMinimized(WindowId id)
{
    HWND hwnd = toHwnd(id);
    return hwnd != nullptr && IsIconic(hwnd);
}

QPixmap grabWindowWithWinRt(WindowId id)
{
    static bool winrtInit = false;
    if (!winrtInit) {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (...) {
        }
        winrtInit = true;
    }

    HWND hwnd = toHwnd(id);
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
    hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(),
                                              reinterpret_cast<IInspectable**>(inspectable.put()));
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
    auto tex = getDxgiInterfaceFromObject<ID3D11Texture2D>(surface);
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

QPixmap grabWindowWithPrintApi(WindowId id)
{
    HWND hwnd = toHwnd(id);
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

}  // namespace win
}  // namespace core
}  // namespace links

#endif  // _WIN32
