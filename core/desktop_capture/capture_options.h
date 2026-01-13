/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Capture Options
 */

#ifndef DESKTOP_CAPTURE_CAPTURE_OPTIONS_H_
#define DESKTOP_CAPTURE_CAPTURE_OPTIONS_H_

#include <cstdint>

namespace links {
namespace desktop_capture {

// Configuration options for desktop capture
struct CaptureOptions {
    // Target frames per second
    int targetFps = 30;

    // Whether to capture the mouse cursor
    bool captureCursor = false;

    // Preferred capture method (platform-specific interpretation)
    enum class CaptureMethod {
        kAuto,      // Let system choose best method
        kHardware,  // Prefer hardware-accelerated capture (WGC, DXGI)
        kSoftware   // Prefer software capture (GDI, Qt)
    };
    CaptureMethod preferredMethod = CaptureMethod::kAuto;

    // Whether to detect and handle fullscreen windows specially
    bool detectFullscreenWindow = true;

    // Timeout in milliseconds for stall detection
    int stallTimeoutMs = 5000;

    // Number of consecutive failures before fallback
    int failureThreshold = 3;

    // Factory methods for common configurations
    static CaptureOptions defaultOptions() {
        return CaptureOptions{};
    }

    static CaptureOptions lowLatency() {
        CaptureOptions opts;
        opts.targetFps = 60;
        opts.preferredMethod = CaptureMethod::kHardware;
        return opts;
    }

    static CaptureOptions lowCpu() {
        CaptureOptions opts;
        opts.targetFps = 15;
        opts.preferredMethod = CaptureMethod::kSoftware;
        return opts;
    }
};

}  // namespace desktop_capture
}  // namespace links

#endif  // DESKTOP_CAPTURE_CAPTURE_OPTIONS_H_
