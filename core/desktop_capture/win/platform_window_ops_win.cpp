#ifdef _WIN32

#include "platform_window_ops_win.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/base.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowsapp.lib")

struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")) IDirect3DDxgiInterfaceAccess : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** p) = 0;
};

namespace links {
namespace core {
namespace win {
namespace {

HWND toHwnd(WindowId id)
{
    return reinterpret_cast<HWND>(static_cast<std::uintptr_t>(id));
}

bool isShareableWindow(HWND hwnd)
{
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd) || IsIconic(hwnd)) {
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
    return GetWindowTextW(hwnd, title, 512) > 0;
}

std::wstring toLower(const std::wstring& value)
{
    std::wstring result = value;
    for (auto& ch : result) {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }
    return result;
}

bool containsKeyword(const std::wstring& titleLower)
{
    return titleLower.find(L"thumbnail") != std::wstring::npos
        || titleLower.find(L"windows input experience") != std::wstring::npos
        || titleLower.find(L"\x7F29\x7565\x56FE") != std::wstring::npos
        || titleLower.find(L"\x8F93\x5165\x4F53\x9A8C") != std::wstring::npos
        || titleLower.find(L"\x8BBE\x7F6E") != std::wstring::npos;
}

std::string toUtf8(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }

    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

template<typename T>
winrt::com_ptr<T> getDxgiInterfaceFromObject(const winrt::Windows::Foundation::IInspectable& object)
{
    winrt::com_ptr<T> result;
    winrt::com_ptr<IDirect3DDxgiInterfaceAccess> access;
    auto unk = object.as<IUnknown>();
    if (!unk) {
        return result;
    }

    if (FAILED(unk->QueryInterface(__uuidof(IDirect3DDxgiInterfaceAccess), access.put_void())) || !access) {
        return result;
    }

    if (SUCCEEDED(access->GetInterface(winrt::guid_of<T>(), result.put_void()))) {
        return result;
    }

    return nullptr;
}

RawImage makeRgbaImageFromBgra(const std::uint8_t* src, int width, int height, int srcStride)
{
    RawImage image;
    image.width = width;
    image.height = height;
    image.stride = width * 4;
    image.format = PixelFormat::RGBA8888;
    image.pixels.resize(static_cast<std::size_t>(image.stride) * static_cast<std::size_t>(height));

    for (int y = 0; y < height; ++y) {
        const std::uint8_t* srcRow = src + static_cast<std::size_t>(y) * static_cast<std::size_t>(srcStride);
        std::uint8_t* dstRow = image.pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(image.stride);
        for (int x = 0; x < width; ++x) {
            const std::uint8_t b = srcRow[x * 4 + 0];
            const std::uint8_t g = srcRow[x * 4 + 1];
            const std::uint8_t r = srcRow[x * 4 + 2];
            const std::uint8_t a = srcRow[x * 4 + 3];
            dstRow[x * 4 + 0] = r;
            dstRow[x * 4 + 1] = g;
            dstRow[x * 4 + 2] = b;
            dstRow[x * 4 + 3] = a;
        }
    }

    return image;
}

struct EnumContext {
    std::vector<WindowInfo>* windows{nullptr};
};

BOOL CALLBACK collectWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto* context = reinterpret_cast<EnumContext*>(lParam);
    if (!context || !context->windows) {
        return FALSE;
    }

    if (!isShareableWindow(hwnd)) {
        return TRUE;
    }

    wchar_t title[512];
    const int titleLength = GetWindowTextW(hwnd, title, 512);
    if (titleLength <= 0) {
        return TRUE;
    }

    std::wstring windowTitle(title, static_cast<std::size_t>(titleLength));
    const std::wstring lower = toLower(windowTitle);
    if (containsKeyword(lower)) {
        return TRUE;
    }

    WindowInfo info;
    info.id = static_cast<WindowId>(reinterpret_cast<std::uintptr_t>(hwnd));
    info.title = toUtf8(windowTitle);

    RECT rect{};
    if (GetWindowRect(hwnd, &rect)) {
        info.geometry = {
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top
        };
    }

    context->windows->push_back(std::move(info));
    return TRUE;
}

}  // namespace

std::vector<WindowInfo> enumerateWindows()
{
    std::vector<WindowInfo> collected;
    EnumContext context;
    context.windows = &collected;
    EnumWindows(&collectWindowsProc, reinterpret_cast<LPARAM>(&context));
    return collected;
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

std::optional<RawImage> captureWindowWithWinRt(WindowId id)
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
        return std::nullopt;
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
        return std::nullopt;
    }

    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    d3dDevice.as(dxgiDevice);
    if (!dxgiDevice) {
        return std::nullopt;
    }

    winrt::com_ptr<IInspectable> inspectable;
    hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(),
                                              reinterpret_cast<IInspectable**>(inspectable.put()));
    if (FAILED(hr) || !inspectable) {
        return std::nullopt;
    }

    auto winrtDevice = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    if (!winrtDevice) {
        return std::nullopt;
    }

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
    try {
        winrt::com_ptr<IGraphicsCaptureItemInterop> interop =
            winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem,
                                          IGraphicsCaptureItemInterop>();
        hr = interop->CreateForWindow(
            hwnd,
            winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
            reinterpret_cast<void**>(winrt::put_abi(item)));
        if (FAILED(hr) || !item) {
            return std::nullopt;
        }
    } catch (...) {
        return std::nullopt;
    }

    auto framePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
        winrtDevice,
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        1,
        item.Size());
    auto session = framePool.CreateCaptureSession(item);
    session.IsCursorCaptureEnabled(false);
    session.StartCapture();

    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame{nullptr};
    for (int i = 0; i < 6; ++i) {
        frame = framePool.TryGetNextFrame();
        if (frame) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    session.Close();
    framePool.Close();

    if (!frame) {
        return std::nullopt;
    }

    auto surface = frame.Surface();
    if (!surface) {
        return std::nullopt;
    }

    auto texture = getDxgiInterfaceFromObject<ID3D11Texture2D>(surface);
    if (!texture) {
        return std::nullopt;
    }

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.MiscFlags = 0;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;

    winrt::com_ptr<ID3D11Texture2D> staging;
    if (FAILED(d3dDevice->CreateTexture2D(&stagingDesc, nullptr, staging.put()))) {
        return std::nullopt;
    }

    d3dContext->CopyResource(staging.get(), texture.get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(d3dContext->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        return std::nullopt;
    }

    RawImage image = makeRgbaImageFromBgra(
        static_cast<const std::uint8_t*>(mapped.pData),
        static_cast<int>(desc.Width),
        static_cast<int>(desc.Height),
        static_cast<int>(mapped.RowPitch));

    d3dContext->Unmap(staging.get(), 0);

    if (!image.isValid()) {
        return std::nullopt;
    }
    return image;
}

std::optional<RawImage> captureWindowWithPrintApi(WindowId id)
{
    HWND hwnd = toHwnd(id);
    if (!hwnd || !IsWindow(hwnd)) {
        return std::nullopt;
    }

    RECT rect{};
    if (!GetWindowRect(hwnd, &rect)) {
        return std::nullopt;
    }

    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }

    HDC windowDc = GetWindowDC(hwnd);
    if (!windowDc) {
        return std::nullopt;
    }

    HDC memDc = CreateCompatibleDC(windowDc);
    if (!memDc) {
        ReleaseDC(hwnd, windowDc);
        return std::nullopt;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(memDc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap || !bits) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        DeleteDC(memDc);
        ReleaseDC(hwnd, windowDc);
        return std::nullopt;
    }

    HGDIOBJ oldObject = SelectObject(memDc, bitmap);
    const BOOL ok = PrintWindow(hwnd, memDc, PW_RENDERFULLCONTENT);

    std::optional<RawImage> result;
    if (ok) {
        RawImage image = makeRgbaImageFromBgra(static_cast<const std::uint8_t*>(bits), width, height, width * 4);
        if (image.isValid()) {
            result = std::move(image);
        }
    }

    SelectObject(memDc, oldObject);
    DeleteObject(bitmap);
    DeleteDC(memDc);
    ReleaseDC(hwnd, windowDc);

    return result;
}

}  // namespace win
}  // namespace core
}  // namespace links

#endif  // _WIN32
