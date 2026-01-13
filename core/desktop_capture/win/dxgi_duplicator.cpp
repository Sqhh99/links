/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - DXGI Desktop Duplication Capturer Implementation
 */

#ifdef _WIN32

#include "dxgi_duplicator.h"
#include "window_utils.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <algorithm>
#include <chrono>

namespace links {
namespace desktop_capture {
namespace win {

class DxgiDuplicator::Impl {
public:
    bool init(HWND hwnd);
    bool capture(std::unique_ptr<DesktopFrame>& outFrame, bool& noNewFrame);
    void shutdown();

private:
    bool createDevice();
    bool updateOutput();
    bool acquireFrame(bool& softMiss);
    std::unique_ptr<DesktopFrame> frameToDesktopFrame();

    HWND hwnd_{nullptr};
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication_;
    Microsoft::WRL::ComPtr<IDXGIOutput> output_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> lastFrame_;
    DXGI_OUTDUPL_FRAME_INFO frameInfo_{};
    SIZE outputSize_{};
    POINT desktopOrigin_{};
    HMONITOR currentMonitor_{nullptr};
};

bool DxgiDuplicator::Impl::createDevice() {
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
        device_.GetAddressOf(),
        nullptr,
        context_.GetAddressOf());

    if (FAILED(hr)) {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            flags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            device_.GetAddressOf(),
            nullptr,
            context_.GetAddressOf());
    }
    return SUCCEEDED(hr) && device_ && context_;
}

bool DxgiDuplicator::Impl::updateOutput() {
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    HRESULT hr = device_.As(&dxgiDevice);
    if (FAILED(hr) || !dxgiDevice) {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(adapter.GetAddressOf());
    if (FAILED(hr) || !adapter) {
        return false;
    }

    HMONITOR targetMonitor = hwnd_ ? MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST) : nullptr;
    Microsoft::WRL::ComPtr<IDXGIOutput> output;

    for (UINT i = 0;; ++i) {
        Microsoft::WRL::ComPtr<IDXGIOutput> out;
        if (adapter->EnumOutputs(i, out.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) {
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
        // Fallback to first output
        adapter->EnumOutputs(0, output.GetAddressOf());
    }
    if (!output) {
        return false;
    }

    output_ = output;
    currentMonitor_ = targetMonitor ? targetMonitor : nullptr;

    Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
    hr = output_.As(&output1);
    if (FAILED(hr) || !output1) {
        return false;
    }

    duplication_.Reset();
    hr = output1->DuplicateOutput(device_.Get(), duplication_.GetAddressOf());
    if (FAILED(hr)) {
        duplication_.Reset();
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

bool DxgiDuplicator::Impl::init(HWND hwnd) {
    hwnd_ = hwnd;
    shutdown();
    currentMonitor_ = hwnd_ ? MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST) : nullptr;
    if (!createDevice()) {
        return false;
    }
    return updateOutput();
}

bool DxgiDuplicator::Impl::acquireFrame(bool& softMiss) {
    softMiss = false;

    // Recreate duplication if window moved to another monitor
    if (hwnd_) {
        HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        if (monitor && monitor != currentMonitor_) {
            duplication_.Reset();
            output_.Reset();
            lastFrame_.Reset();
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
        Microsoft::WRL::ComPtr<IDXGIResource> resource;
        HRESULT hr = duplication_->AcquireNextFrame(16, &frameInfo_, resource.GetAddressOf());

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            softMiss = true;
            return false;
        }
        if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL ||
            hr == DXGI_ERROR_DEVICE_REMOVED) {
            duplication_.Reset();
            output_.Reset();
            lastFrame_.Reset();
            continue;
        }
        if (FAILED(hr) || !resource) {
            return false;
        }

        lastFrame_.Reset();
        resource.As(&lastFrame_);
        duplication_->ReleaseFrame();
        return lastFrame_ != nullptr;
    }
    return false;
}

std::unique_ptr<DesktopFrame> DxgiDuplicator::Impl::frameToDesktopFrame() {
    if (!lastFrame_) return nullptr;

    D3D11_TEXTURE2D_DESC desc{};
    lastFrame_->GetDesc(&desc);
    if (desc.Width == 0 || desc.Height == 0) {
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.MiscFlags = 0;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device_->CreateTexture2D(&stagingDesc, nullptr, staging.GetAddressOf()))) {
        return nullptr;
    }
    context_->CopyResource(staging.Get(), lastFrame_.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        return nullptr;
    }

    auto frame = std::make_unique<BasicDesktopFrame>(
        DesktopSize(static_cast<int>(desc.Width), static_cast<int>(desc.Height)));

    // Convert BGRA to RGBA and copy
    const uint8_t* src = static_cast<const uint8_t*>(mapped.pData);
    uint8_t* dst = frame->data();
    for (uint32_t y = 0; y < desc.Height; ++y) {
        const uint8_t* srcRow = src + y * mapped.RowPitch;
        uint8_t* dstRow = dst + y * frame->stride();
        for (uint32_t x = 0; x < desc.Width; ++x) {
            dstRow[x * 4 + 0] = srcRow[x * 4 + 2];  // R <- B
            dstRow[x * 4 + 1] = srcRow[x * 4 + 1];  // G <- G
            dstRow[x * 4 + 2] = srcRow[x * 4 + 0];  // B <- R
            dstRow[x * 4 + 3] = srcRow[x * 4 + 3];  // A <- A
        }
    }

    context_->Unmap(staging.Get(), 0);
    return frame;
}

bool DxgiDuplicator::Impl::capture(std::unique_ptr<DesktopFrame>& outFrame, bool& noNewFrame) {
    noNewFrame = false;
    if (!device_ || !context_) {
        return false;
    }
    bool softMiss = false;
    if (!acquireFrame(softMiss)) {
        noNewFrame = softMiss;
        return softMiss;
    }

    auto frame = frameToDesktopFrame();
    if (!frame) {
        return false;
    }

    // Crop to window if specified
    if (hwnd_) {
        RECT rect{};
        if (GetWindowRect(hwnd_, &rect)) {
            int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;
            if (w > 10 && h > 10) {
                int x = rect.left - desktopOrigin_.x;
                int y = rect.top - desktopOrigin_.y;
                int x0 = (std::max)(0, x);
                int y0 = (std::max)(0, y);
                int x1 = (std::min)(x + w, frame->width());
                int y1 = (std::min)(y + h, frame->height());
                int width = x1 - x0;
                int height = y1 - y0;

                if (width > 1 && height > 1 && x0 < frame->width() && y0 < frame->height()) {
                    auto cropped = std::make_unique<BasicDesktopFrame>(DesktopSize(width, height));
                    cropped->copyPixelsFrom(*frame, DesktopVector(x0, y0),
                                            DesktopRect::makeXYWH(0, 0, width, height));
                    frame = std::move(cropped);
                }
            }
        }
    }

    frame->setCaptureTimeUs(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    outFrame = std::move(frame);
    return true;
}

void DxgiDuplicator::Impl::shutdown() {
    duplication_.Reset();
    output_.Reset();
    lastFrame_.Reset();
    context_.Reset();
    device_.Reset();
    hwnd_ = nullptr;
    outputSize_ = {};
    frameInfo_ = {};
    currentMonitor_ = nullptr;
}

// DxgiDuplicator public interface
DxgiDuplicator::DxgiDuplicator(const CaptureOptions& options)
    : impl_(std::make_unique<Impl>()) {
    options_ = options;
}

DxgiDuplicator::~DxgiDuplicator() {
    stop();
}

void DxgiDuplicator::start(Callback* callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = callback;
    if (impl_->init(reinterpret_cast<HWND>(selectedSource_))) {
        started_ = true;
    }
}

void DxgiDuplicator::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    started_ = false;
    impl_->shutdown();
    callback_ = nullptr;
}

void DxgiDuplicator::captureFrame() {
    if (!started_ || !callback_) {
        return;
    }
    std::unique_ptr<DesktopFrame> frame;
    bool noNewFrame = false;
    if (impl_->capture(frame, noNewFrame) && frame) {
        callback_->onCaptureResult(Result::SUCCESS, std::move(frame));
    } else if (noNewFrame) {
        // No new frame available, this is temporary
        callback_->onCaptureResult(Result::ERROR_TEMPORARY, nullptr);
    } else {
        callback_->onCaptureResult(Result::ERROR_TEMPORARY, nullptr);
    }
}

bool DxgiDuplicator::getSourceList(SourceList* sources) {
    if (!sources) {
        return false;
    }
    sources->clear();
    auto windows = enumerateCaptureWindows();
    for (const auto& w : windows) {
        Source src;
        src.id = reinterpret_cast<SourceId>(w.hwnd);
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

bool DxgiDuplicator::selectSource(SourceId id) {
    selectedSource_ = id;
    return true;
}

bool DxgiDuplicator::isSourceValid(SourceId id) {
    if (id == 0) return true;  // Screen capture
    return isWindowValid(reinterpret_cast<HWND>(id));
}

DxgiDuplicator::SourceId DxgiDuplicator::selectedSource() const {
    return selectedSource_;
}

bool DxgiDuplicator::isSupported() {
    return isDxgiDuplicationSupported();
}

}  // namespace win
}  // namespace desktop_capture
}  // namespace links

#endif  // _WIN32
