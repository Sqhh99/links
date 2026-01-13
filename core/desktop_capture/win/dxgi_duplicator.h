/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - DXGI Desktop Duplication Capturer
 */

#ifndef DESKTOP_CAPTURE_WIN_DXGI_DUPLICATOR_H_
#define DESKTOP_CAPTURE_WIN_DXGI_DUPLICATOR_H_

#ifdef _WIN32

#include "../desktop_capturer.h"
#include <memory>
#include <atomic>
#include <mutex>

namespace links {
namespace desktop_capture {
namespace win {

// DXGI Desktop Duplication based capturer.
// Captures the entire desktop and can crop to specific windows.
// Works on Windows 8+ and doesn't require special permissions.
class DxgiDuplicator : public DesktopCapturer {
public:
    explicit DxgiDuplicator(const CaptureOptions& options);
    ~DxgiDuplicator() override;

    // DesktopCapturer interface
    void start(Callback* callback) override;
    void stop() override;
    void captureFrame() override;
    bool getSourceList(SourceList* sources) override;
    bool selectSource(SourceId id) override;
    bool isSourceValid(SourceId id) override;
    SourceId selectedSource() const override;

    // Check if DXGI duplication is available
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
#endif  // DESKTOP_CAPTURE_WIN_DXGI_DUPLICATOR_H_
