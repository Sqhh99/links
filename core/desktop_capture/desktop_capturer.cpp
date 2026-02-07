/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Factory Implementation
 */

#include "desktop_capturer.h"

#ifdef _WIN32
#include "win/wgc_capturer.h"
#include "win/dxgi_duplicator.h"
#include "win/gdi_capturer.h"
#elif defined(__APPLE__)
#include "mac/mac_capturer.h"
#elif defined(__linux__)
#include "linux/x11/x11_capturer.h"
#include "linux/x11/platform_window_ops_linux_x11.h"
#endif

namespace links {
namespace desktop_capture {

std::unique_ptr<DesktopCapturer> DesktopCapturer::createScreenCapturer(
    const CaptureOptions& options) {
#ifdef _WIN32
    if (win::WgcCapturer::isSupported()) {
        return std::make_unique<win::WgcCapturer>(options);
    }
    if (win::DxgiDuplicator::isSupported()) {
        return std::make_unique<win::DxgiDuplicator>(options);
    }
    return nullptr;
#elif defined(__APPLE__)
    return std::make_unique<mac::MacScreenCapturer>(options);
#elif defined(__linux__)
    if (!core::linux_x11::isScreenShareSupported()) {
        return nullptr;
    }
    return std::make_unique<linux_x11::X11ScreenCapturer>(options);
#else
    return nullptr;
#endif
}

std::unique_ptr<DesktopCapturer> DesktopCapturer::createWindowCapturer(
    const CaptureOptions& options) {
#ifdef _WIN32
    if (options.preferredMethod != CaptureOptions::CaptureMethod::kSoftware) {
        if (win::WgcCapturer::isSupported()) {
            return std::make_unique<win::WgcCapturer>(options);
        }
        if (win::DxgiDuplicator::isSupported()) {
            return std::make_unique<win::DxgiDuplicator>(options);
        }
    }
    return std::make_unique<win::GdiCapturer>(options);
#elif defined(__APPLE__)
    return std::make_unique<mac::MacWindowCapturer>(options);
#elif defined(__linux__)
    if (!core::linux_x11::isWindowShareSupported()) {
        return nullptr;
    }
    return std::make_unique<linux_x11::X11WindowCapturer>(options);
#else
    return nullptr;
#endif
}

}  // namespace desktop_capture
}  // namespace links
