#ifdef __linux__

#include "x11_capturer.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

#include "../../../image_types.h"
#include "platform_window_ops_linux_x11.h"

namespace links {
namespace desktop_capture {
namespace linux_x11 {
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

}  // namespace

X11ScreenCapturer::X11ScreenCapturer(const CaptureOptions& options)
{
    options_ = options;
    setBackend(CaptureBackend::X11);
    setLastError(CaptureError::Ok);
}

X11ScreenCapturer::~X11ScreenCapturer() = default;

void X11ScreenCapturer::start(Callback* callback)
{
    callback_ = callback;
    started_ = callback_ != nullptr;
}

void X11ScreenCapturer::stop()
{
    started_ = false;
    callback_ = nullptr;
}

void X11ScreenCapturer::captureFrame()
{
    if (!started_ || !callback_) {
        return;
    }

    auto image = core::linux_x11::captureRootScreenWithX11();
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

bool X11ScreenCapturer::getSourceList(SourceList* sources)
{
    if (!sources) {
        return false;
    }

    sources->clear();
    if (!core::linux_x11::isScreenShareSupported()) {
        setLastError(CaptureError::BackendUnavailable);
        return false;
    }

    Source source;
    source.id = 1;
    source.displayId = 0;
    source.title = "Primary screen";
    sources->push_back(std::move(source));
    setLastError(CaptureError::Ok);
    return true;
}

bool X11ScreenCapturer::selectSource(SourceId id)
{
    if (!isSourceValid(id)) {
        setLastError(CaptureError::BackendUnavailable);
        return false;
    }
    selectedSource_ = id;
    setLastError(CaptureError::Ok);
    return true;
}

bool X11ScreenCapturer::isSourceValid(SourceId id)
{
    return id == 1 && core::linux_x11::isScreenShareSupported();
}

X11ScreenCapturer::SourceId X11ScreenCapturer::selectedSource() const
{
    return selectedSource_;
}

X11WindowCapturer::X11WindowCapturer(const CaptureOptions& options)
{
    options_ = options;
    setBackend(CaptureBackend::X11);
    setLastError(CaptureError::Ok);
}

X11WindowCapturer::~X11WindowCapturer() = default;

void X11WindowCapturer::start(Callback* callback)
{
    callback_ = callback;
    started_ = callback_ != nullptr;
}

void X11WindowCapturer::stop()
{
    started_ = false;
    callback_ = nullptr;
}

void X11WindowCapturer::captureFrame()
{
    if (!started_ || !callback_) {
        return;
    }

    if (selectedSource_ == 0) {
        setLastError(CaptureError::BackendUnavailable);
        callback_->onCaptureResult(Result::ERROR_PERMANENT, nullptr);
        return;
    }

    auto image = core::linux_x11::captureWindowWithX11(static_cast<core::WindowId>(selectedSource_));
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

bool X11WindowCapturer::getSourceList(SourceList* sources)
{
    if (!sources) {
        return false;
    }

    sources->clear();
    const auto windows = core::linux_x11::enumerateWindows();
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

bool X11WindowCapturer::selectSource(SourceId id)
{
    if (!isSourceValid(id)) {
        setLastError(CaptureError::BackendUnavailable);
        return false;
    }

    selectedSource_ = id;
    setLastError(CaptureError::Ok);
    return true;
}

bool X11WindowCapturer::isSourceValid(SourceId id)
{
    return core::linux_x11::isWindowValid(static_cast<core::WindowId>(id));
}

X11WindowCapturer::SourceId X11WindowCapturer::selectedSource() const
{
    return selectedSource_;
}

}  // namespace linux_x11
}  // namespace desktop_capture
}  // namespace links

#endif  // __linux__
