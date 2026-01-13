/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Factory Implementation
 */

#include "desktop_capturer.h"

#ifdef _WIN32
#include "win/wgc_capturer.h"
#include "win/dxgi_duplicator.h"
#include "win/gdi_capturer.h"
#endif

namespace links {
namespace desktop_capture {

std::unique_ptr<DesktopCapturer> DesktopCapturer::createScreenCapturer(
    const CaptureOptions& options) {
#ifdef _WIN32
    // For screen capture, use DXGI duplicator
    return std::make_unique<win::DxgiDuplicator>(options);
#else
    // TODO: Implement for other platforms
    return nullptr;
#endif
}

std::unique_ptr<DesktopCapturer> DesktopCapturer::createWindowCapturer(
    const CaptureOptions& options) {
#ifdef _WIN32
    // Prefer WGC if available, fallback to DXGI, then GDI
    if (options.preferredMethod != CaptureOptions::CaptureMethod::kSoftware) {
        if (win::WgcCapturer::isSupported()) {
            return std::make_unique<win::WgcCapturer>(options);
        }
        if (win::DxgiDuplicator::isSupported()) {
            return std::make_unique<win::DxgiDuplicator>(options);
        }
    }
    // Software/fallback
    return std::make_unique<win::GdiCapturer>(options);
#else
    // TODO: Implement for other platforms
    return nullptr;
#endif
}

}  // namespace desktop_capture
}  // namespace links
