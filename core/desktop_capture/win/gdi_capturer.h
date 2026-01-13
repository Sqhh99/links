/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - GDI Fallback Capturer
 */

#ifndef DESKTOP_CAPTURE_WIN_GDI_CAPTURER_H_
#define DESKTOP_CAPTURE_WIN_GDI_CAPTURER_H_

#ifdef _WIN32

#include "../desktop_capturer.h"
#include <memory>

namespace links {
namespace desktop_capture {
namespace win {

// GDI-based window capturer using PrintWindow API.
// Works on all Windows versions but is slower than other methods.
// Used as a fallback when WGC and DXGI are unavailable.
class GdiCapturer : public DesktopCapturer {
public:
    explicit GdiCapturer(const CaptureOptions& options);
    ~GdiCapturer() override;

    // DesktopCapturer interface
    void start(Callback* callback) override;
    void stop() override;
    void captureFrame() override;
    bool getSourceList(SourceList* sources) override;
    bool selectSource(SourceId id) override;
    bool isSourceValid(SourceId id) override;
    SourceId selectedSource() const override;

private:
    std::unique_ptr<DesktopFrame> captureWindow(void* hwnd);

    Callback* callback_ = nullptr;
    SourceId selectedSource_ = 0;
    bool started_ = false;
};

}  // namespace win
}  // namespace desktop_capture
}  // namespace links

#endif  // _WIN32
#endif  // DESKTOP_CAPTURE_WIN_GDI_CAPTURER_H_
