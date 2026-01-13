/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Windows Graphics Capture (WGC) API Capturer Implementation
 */

#ifdef _WIN32

#include "wgc_capturer.h"
#include "window_utils.h"
#include <chrono>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
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
#include <wrl/client.h>

// Interop interface for DXGI/WinRT
namespace winrt::Windows::Graphics::DirectX::Direct3D11 {
struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")) IDirect3DDxgiInterfaceAccess : ::IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** p) = 0;
};
}

namespace links {
namespace desktop_capture {
namespace win {

namespace {

template<typename T>
winrt::com_ptr<T> getDxgiInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object) {
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

}  // namespace

class WgcCapturer::Impl {
public:
    bool init(HWND hwnd);
    bool capture(std::unique_ptr<DesktopFrame>& outFrame);
    void shutdown();
    void setCopyIntervalMs(int ms) { copyIntervalMs_ = (ms > 0) ? ms : 0; }

private:
    bool createDevice();
    bool recreateFramePool(int width, int height);
    std::unique_ptr<DesktopFrame> frameToDesktopFrame(
        const winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame& frame);
    void onFrameArrived(const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender);
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem createItem(HWND hwnd);

    winrt::com_ptr<ID3D11Device> d3dDevice_;
    winrt::com_ptr<ID3D11DeviceContext> d3dContext_;
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice winrtDevice_{nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool_{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item_{nullptr};
    winrt::com_ptr<ID3D11Texture2D> staging_;
    DesktopSize lastSize_;
    bool initialized_{false};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker frameArrived_;
    std::mutex frameMutex_;
    std::unique_ptr<DesktopFrame> latestFrame_;
    bool hasFrame_{false};
    int copyIntervalMs_{0};
    std::chrono::steady_clock::time_point lastCopy_;
};

bool WgcCapturer::Impl::createDevice() {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
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
    hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(),
        reinterpret_cast<IInspectable**>(inspectable.put()));
    if (FAILED(hr)) {
        return false;
    }

    winrtDevice_ = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    return winrtDevice_ != nullptr;
}

bool WgcCapturer::Impl::init(HWND hwnd) {
    shutdown();

    if (!createDevice()) {
        return false;
    }

    try {
        auto access = winrt::Windows::Graphics::Capture::GraphicsCaptureAccess::RequestAccessAsync(
            winrt::Windows::Graphics::Capture::GraphicsCaptureAccessKind::Borderless);
        access.get();
    } catch (...) {
        // Access request may fail, but capture might still work
    }

    item_ = createItem(hwnd);
    if (!item_) {
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
    } catch (...) {
        shutdown();
        return false;
    }

    lastSize_ = DesktopSize(static_cast<int>(size.Width), static_cast<int>(size.Height));
    initialized_ = true;
    return true;
}

bool WgcCapturer::Impl::recreateFramePool(int width, int height) {
    if (!winrtDevice_ || !item_) {
        return false;
    }
    try {
        framePool_.Recreate(
            winrtDevice_,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            1,
            {width, height});
    } catch (...) {
        return false;
    }
    lastSize_ = DesktopSize(width, height);
    staging_ = nullptr;
    return true;
}

std::unique_ptr<DesktopFrame> WgcCapturer::Impl::frameToDesktopFrame(
    const winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame& frame) {
    if (!frame) {
        return nullptr;
    }

    auto surface = frame.Surface();
    if (!surface) {
        return nullptr;
    }

    winrt::com_ptr<ID3D11Texture2D> texture;
    try {
        auto inspectable = surface.as<winrt::Windows::Foundation::IInspectable>();
        auto tex = getDxgiInterfaceFromObject<ID3D11Texture2D>(inspectable);
        texture = tex;
        if (!texture) {
            return nullptr;
        }
    } catch (...) {
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    if (!staging_ || desc.Width != static_cast<UINT>(lastSize_.width()) ||
        desc.Height != static_cast<UINT>(lastSize_.height())) {
        D3D11_TEXTURE2D_DESC stagingDesc = desc;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.MiscFlags = 0;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.SampleDesc.Quality = 0;
        HRESULT hr = d3dDevice_->CreateTexture2D(&stagingDesc, nullptr, staging_.put());
        if (FAILED(hr)) {
            return nullptr;
        }
    }

    d3dContext_->CopyResource(staging_.get(), texture.get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = d3dContext_->Map(staging_.get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        return nullptr;
    }

    // Create frame and copy data
    auto desktopFrame = std::make_unique<BasicDesktopFrame>(
        DesktopSize(static_cast<int>(desc.Width), static_cast<int>(desc.Height)));

    // Convert BGRA to RGBA and copy
    const uint8_t* src = static_cast<const uint8_t*>(mapped.pData);
    uint8_t* dst = desktopFrame->data();
    for (uint32_t y = 0; y < desc.Height; ++y) {
        const uint8_t* srcRow = src + y * mapped.RowPitch;
        uint8_t* dstRow = dst + y * desktopFrame->stride();
        for (uint32_t x = 0; x < desc.Width; ++x) {
            dstRow[x * 4 + 0] = srcRow[x * 4 + 2];  // R <- B
            dstRow[x * 4 + 1] = srcRow[x * 4 + 1];  // G <- G
            dstRow[x * 4 + 2] = srcRow[x * 4 + 0];  // B <- R
            dstRow[x * 4 + 3] = srcRow[x * 4 + 3];  // A <- A
        }
    }

    d3dContext_->Unmap(staging_.get(), 0);

    desktopFrame->setCaptureTimeUs(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    return desktopFrame;
}

bool WgcCapturer::Impl::capture(std::unique_ptr<DesktopFrame>& outFrame) {
    if (!initialized_ || !framePool_ || !d3dDevice_ || !d3dContext_) {
        return false;
    }
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (!hasFrame_ || !latestFrame_) {
        return false;
    }
    outFrame = BasicDesktopFrame::copyOf(*latestFrame_);
    return true;
}

void WgcCapturer::Impl::onFrameArrived(
    const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender) {
    try {
        auto frame = sender.TryGetNextFrame();
        if (!frame) {
            return;
        }
        auto contentSize = frame.ContentSize();
        if (contentSize.Width != lastSize_.width() ||
            contentSize.Height != lastSize_.height()) {
            if (!recreateFramePool(contentSize.Width, contentSize.Height)) {
                return;
            }
        }
        auto now = std::chrono::steady_clock::now();
        if (copyIntervalMs_ > 0 && hasFrame_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastCopy_).count();
            if (elapsed < copyIntervalMs_) {
                return;
            }
        }
        auto desktopFrame = frameToDesktopFrame(frame);
        if (!desktopFrame) {
            return;
        }
        std::lock_guard<std::mutex> lock(frameMutex_);
        latestFrame_ = std::move(desktopFrame);
        hasFrame_ = true;
        lastCopy_ = now;
    } catch (...) {
        // Ignore frame errors
    }
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem WgcCapturer::Impl::createItem(HWND hwnd) {
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
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

void WgcCapturer::Impl::shutdown() {
    frameArrived_.revoke();
    session_ = nullptr;
    framePool_ = nullptr;
    item_ = nullptr;
    staging_ = nullptr;
    winrtDevice_ = nullptr;
    d3dContext_ = nullptr;
    d3dDevice_ = nullptr;
    lastSize_ = DesktopSize();
    initialized_ = false;
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        latestFrame_.reset();
        hasFrame_ = false;
    }
}

// WgcCapturer public interface
WgcCapturer::WgcCapturer(const CaptureOptions& options)
    : impl_(std::make_unique<Impl>()) {
    options_ = options;
}

WgcCapturer::~WgcCapturer() {
    stop();
}

void WgcCapturer::start(Callback* callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = callback;
    if (selectedSource_ != 0) {
        impl_->setCopyIntervalMs(1000 / std::max(1, options_.targetFps));
        if (impl_->init(reinterpret_cast<HWND>(selectedSource_))) {
            started_ = true;
        }
    }
}

void WgcCapturer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    started_ = false;
    impl_->shutdown();
    callback_ = nullptr;
}

void WgcCapturer::captureFrame() {
    if (!started_ || !callback_) {
        return;
    }
    std::unique_ptr<DesktopFrame> frame;
    if (impl_->capture(frame) && frame) {
        callback_->onCaptureResult(Result::SUCCESS, std::move(frame));
    } else {
        callback_->onCaptureResult(Result::ERROR_TEMPORARY, nullptr);
    }
}

bool WgcCapturer::getSourceList(SourceList* sources) {
    if (!sources) {
        return false;
    }
    sources->clear();
    auto windows = enumerateCaptureWindows();
    for (const auto& w : windows) {
        Source src;
        src.id = reinterpret_cast<SourceId>(w.hwnd);
        // Convert wide string to UTF-8
        int len = WideCharToMultiByte(CP_UTF8, 0, w.title.c_str(), -1,
                                      nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            std::string utf8(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, w.title.c_str(), -1,
                               utf8.data(), len, nullptr, nullptr);
            src.title = utf8;
        }
        sources->push_back(src);
    }
    return true;
}

bool WgcCapturer::selectSource(SourceId id) {
    if (!isSourceValid(id)) {
        return false;
    }
    selectedSource_ = id;
    return true;
}

bool WgcCapturer::isSourceValid(SourceId id) {
    return isWindowValid(reinterpret_cast<HWND>(id));
}

WgcCapturer::SourceId WgcCapturer::selectedSource() const {
    return selectedSource_;
}

bool WgcCapturer::isSupported() {
    return isWgcSupported();
}

}  // namespace win
}  // namespace desktop_capture
}  // namespace links

#endif  // _WIN32
