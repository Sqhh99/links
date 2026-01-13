/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Windows Graphics Capture (WGC) API Capturer
 */

#ifndef DESKTOP_CAPTURE_WIN_WGC_CAPTURER_H_
#define DESKTOP_CAPTURE_WIN_WGC_CAPTURER_H_

#ifdef _WIN32

#include "../desktop_capturer.h"
#include <memory>
#include <atomic>
#include <mutex>

// Forward declarations
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace links {
namespace desktop_capture {
namespace win {

// Windows Graphics Capture (WGC) based capturer.
// Uses the modern Windows.Graphics.Capture API available on Windows 10 1903+.
class WgcCapturer : public DesktopCapturer {
public:
    explicit WgcCapturer(const CaptureOptions& options);
    ~WgcCapturer() override;

    // DesktopCapturer interface
    void start(Callback* callback) override;
    void stop() override;
    void captureFrame() override;
    bool getSourceList(SourceList* sources) override;
    bool selectSource(SourceId id) override;
    bool isSourceValid(SourceId id) override;
    SourceId selectedSource() const override;

    // Check if WGC is available on this system
    static bool isSupported();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    Callback* callback_ = nullptr;
    SourceId selectedSource_ = 0;
    std::atomic<bool> started_{false};
    std::mutex mutex_;
};

}  // namespace win
}  // namespace desktop_capture
}  // namespace links

#endif  // _WIN32
#endif  // DESKTOP_CAPTURE_WIN_WGC_CAPTURER_H_
