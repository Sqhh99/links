#ifdef __APPLE__

#include "mac_capturer.h"

#include <CoreGraphics/CoreGraphics.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#include "../../image_types.h"
#include "../../window_types.h"
#include "platform_window_ops_mac.h"
#include "screen_capture_kit_adapter.h"

namespace links {
namespace desktop_capture {
namespace mac {
namespace {

std::unique_ptr<DesktopFrame> toDesktopFrame(const core::RawImage& raw)
{
    if (!raw.isValid()) {
        return nullptr;
    }

    auto frame = std::make_unique<BasicDesktopFrame>(DesktopSize(raw.width, raw.height));
    frame->setCaptureTimeUs(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    for (int y = 0; y < raw.height; ++y) {
        const std::uint8_t* src = raw.pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(raw.stride);
        std::uint8_t* dst = frame->dataAt(y);

        if (raw.format == core::PixelFormat::RGBA8888) {
            std::memcpy(dst, src, static_cast<std::size_t>(raw.width) * 4U);
            continue;
        }

        for (int x = 0; x < raw.width; ++x) {
            const std::uint8_t b = src[x * 4 + 0];
            const std::uint8_t g = src[x * 4 + 1];
            const std::uint8_t r = src[x * 4 + 2];
            const std::uint8_t a = src[x * 4 + 3];
            dst[x * 4 + 0] = r;
            dst[x * 4 + 1] = g;
            dst[x * 4 + 2] = b;
            dst[x * 4 + 3] = a;
        }
    }

    return frame;
}

const char* backendName(DesktopCapturer::CaptureBackend backend)
{
    switch (backend) {
        case DesktopCapturer::CaptureBackend::ScreenCaptureKit:
            return "ScreenCaptureKit";
        case DesktopCapturer::CaptureBackend::CoreGraphics:
            return "CoreGraphics";
        case DesktopCapturer::CaptureBackend::X11:
            return "X11";
        case DesktopCapturer::CaptureBackend::Wgc:
            return "WGC";
        case DesktopCapturer::CaptureBackend::Dxgi:
            return "DXGI";
        case DesktopCapturer::CaptureBackend::Gdi:
            return "GDI";
        default:
            return "Unknown";
    }
}

}  // namespace

MacScreenCapturer::MacScreenCapturer(const CaptureOptions& options)
{
    options_ = options;
    setLastError(CaptureError::Ok);
}

MacScreenCapturer::~MacScreenCapturer() = default;

void MacScreenCapturer::start(Callback* callback)
{
    callback_ = callback;
    started_ = callback_ != nullptr;
    if (!core::mac::hasScreenRecordingPermission()) {
        setLastError(CaptureError::NoPermission);
    } else {
        setLastError(CaptureError::Ok);
    }
}

void MacScreenCapturer::stop()
{
    started_ = false;
    callback_ = nullptr;
}

void MacScreenCapturer::captureFrame()
{
    if (!started_ || !callback_) {
        return;
    }

    if (!core::mac::hasScreenRecordingPermission()) {
        setLastError(CaptureError::NoPermission);
        callback_->onCaptureResult(Result::ERROR_PERMANENT, nullptr);
        return;
    }

    const std::uint32_t displayId = selectedSource_ == 0
        ? static_cast<std::uint32_t>(CGMainDisplayID())
        : static_cast<std::uint32_t>(selectedSource_);

    auto image = core::mac::captureDisplayWithScreenCaptureKit(displayId);
    if (image.has_value()) {
        setBackend(CaptureBackend::ScreenCaptureKit);
        logBackendSelectionIfNeeded();
    } else {
        image = core::mac::captureDisplayWithCoreGraphics(displayId);
        if (image.has_value()) {
            setBackend(CaptureBackend::CoreGraphics);
            logBackendSelectionIfNeeded();
        }
    }

    if (!image.has_value()) {
        if (!core::mac::isScreenCaptureKitAvailable()) {
            setLastError(CaptureError::BackendUnavailable);
        } else {
            setLastError(CaptureError::RuntimeFailure);
        }
        callback_->onCaptureResult(Result::ERROR_TEMPORARY, nullptr);
        return;
    }

    auto frame = toDesktopFrame(*image);
    if (!frame) {
        setLastError(CaptureError::RuntimeFailure);
        callback_->onCaptureResult(Result::ERROR_TEMPORARY, nullptr);
        return;
    }

    setLastError(CaptureError::Ok);
    callback_->onCaptureResult(Result::SUCCESS, std::move(frame));
}

bool MacScreenCapturer::getSourceList(SourceList* sources)
{
    if (!sources) {
        return false;
    }

    sources->clear();
    const auto displays = core::mac::enumerateDisplays();
    sources->reserve(displays.size());

    for (std::uint32_t displayId : displays) {
        Source source;
        source.id = static_cast<SourceId>(displayId);
        source.displayId = static_cast<int64_t>(displayId);
        source.title = "Display " + std::to_string(displayId);
        sources->push_back(std::move(source));
    }

    setLastError(CaptureError::Ok);
    return true;
}

bool MacScreenCapturer::selectSource(SourceId id)
{
    if (id == 0) {
        selectedSource_ = static_cast<SourceId>(CGMainDisplayID());
        setLastError(CaptureError::Ok);
        return true;
    }

    if (!isSourceValid(id)) {
        setLastError(CaptureError::BackendUnavailable);
        return false;
    }

    selectedSource_ = id;
    setLastError(CaptureError::Ok);
    return true;
}

bool MacScreenCapturer::isSourceValid(SourceId id)
{
    if (id == 0) {
        return true;
    }

    const auto displays = core::mac::enumerateDisplays();
    for (std::uint32_t displayId : displays) {
        if (static_cast<SourceId>(displayId) == id) {
            return true;
        }
    }

    return false;
}

MacScreenCapturer::SourceId MacScreenCapturer::selectedSource() const
{
    return selectedSource_;
}

void MacScreenCapturer::logBackendSelectionIfNeeded()
{
    if (backend() == CaptureBackend::Unknown || loggedBackend_ == backend()) {
        return;
    }

    loggedBackend_ = backend();
    std::clog << "mac capture backend = " << backendName(loggedBackend_) << '\n';
}

MacWindowCapturer::MacWindowCapturer(const CaptureOptions& options)
{
    options_ = options;
    setBackend(CaptureBackend::CoreGraphics);
    setLastError(CaptureError::Ok);
}

MacWindowCapturer::~MacWindowCapturer() = default;

void MacWindowCapturer::start(Callback* callback)
{
    callback_ = callback;
    started_ = callback_ != nullptr;
    logBackendSelectionIfNeeded();
    if (!core::mac::hasScreenRecordingPermission()) {
        setLastError(CaptureError::NoPermission);
    } else {
        setLastError(CaptureError::Ok);
    }
}

void MacWindowCapturer::stop()
{
    started_ = false;
    callback_ = nullptr;
}

void MacWindowCapturer::captureFrame()
{
    if (!started_ || !callback_) {
        return;
    }

    if (!core::mac::hasScreenRecordingPermission()) {
        setLastError(CaptureError::NoPermission);
        callback_->onCaptureResult(Result::ERROR_PERMANENT, nullptr);
        return;
    }

    if (selectedSource_ == 0) {
        setLastError(CaptureError::BackendUnavailable);
        callback_->onCaptureResult(Result::ERROR_PERMANENT, nullptr);
        return;
    }

    auto image = core::mac::captureWindowWithCoreGraphics(static_cast<core::WindowId>(selectedSource_));
    if (!image.has_value()) {
        setLastError(CaptureError::RuntimeFailure);
        callback_->onCaptureResult(Result::ERROR_TEMPORARY, nullptr);
        return;
    }

    auto frame = toDesktopFrame(*image);
    if (!frame) {
        setLastError(CaptureError::RuntimeFailure);
        callback_->onCaptureResult(Result::ERROR_TEMPORARY, nullptr);
        return;
    }

    setLastError(CaptureError::Ok);
    callback_->onCaptureResult(Result::SUCCESS, std::move(frame));
}

bool MacWindowCapturer::getSourceList(SourceList* sources)
{
    if (!sources) {
        return false;
    }

    sources->clear();
    const auto windows = core::mac::enumerateWindows();
    sources->reserve(windows.size());

    for (const auto& window : windows) {
        Source source;
        source.id = static_cast<SourceId>(window.id);
        source.title = window.title;
        sources->push_back(std::move(source));
    }

    setLastError(CaptureError::Ok);
    return true;
}

bool MacWindowCapturer::selectSource(SourceId id)
{
    if (!isSourceValid(id)) {
        setLastError(CaptureError::BackendUnavailable);
        return false;
    }

    selectedSource_ = id;
    setLastError(CaptureError::Ok);
    return true;
}

bool MacWindowCapturer::isSourceValid(SourceId id)
{
    return core::mac::isWindowValid(static_cast<core::WindowId>(id));
}

MacWindowCapturer::SourceId MacWindowCapturer::selectedSource() const
{
    return selectedSource_;
}

void MacWindowCapturer::logBackendSelectionIfNeeded()
{
    if (loggedBackend_ == backend()) {
        return;
    }

    loggedBackend_ = backend();
    std::clog << "mac capture backend = " << backendName(loggedBackend_) << '\n';
}

}  // namespace mac
}  // namespace desktop_capture
}  // namespace links

#endif  // __APPLE__
