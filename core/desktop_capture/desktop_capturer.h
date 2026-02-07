/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Abstract Capturer Interface
 */

#ifndef DESKTOP_CAPTURE_DESKTOP_CAPTURER_H_
#define DESKTOP_CAPTURE_DESKTOP_CAPTURER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "desktop_frame.h"
#include "capture_options.h"

namespace links {
namespace desktop_capture {

class DesktopCapturer {
public:
    enum class Result {
        SUCCESS,
        ERROR_TEMPORARY,
        ERROR_PERMANENT
    };

    enum class CaptureBackend {
        Unknown,
        ScreenCaptureKit,
        CoreGraphics,
        X11,
        Wgc,
        Dxgi,
        Gdi
    };

    enum class CaptureError {
        Ok,
        NoPermission,
        BackendUnavailable,
        RuntimeFailure
    };

    class Callback {
    public:
        virtual ~Callback() = default;
        virtual void onCaptureResult(Result result,
                                     std::unique_ptr<DesktopFrame> frame) = 0;
    };

#ifdef _WIN32
    using SourceId = intptr_t;
#else
    using SourceId = int64_t;
#endif

    struct Source {
        SourceId id = 0;
        std::string title;
        int64_t displayId = -1;
    };

    using SourceList = std::vector<Source>;

    virtual ~DesktopCapturer() = default;

    virtual void start(Callback* callback) = 0;
    virtual void stop() = 0;
    virtual void captureFrame() = 0;

    virtual bool getSourceList(SourceList* sources) = 0;
    virtual bool selectSource(SourceId id) = 0;
    virtual bool isSourceValid(SourceId id) = 0;
    virtual SourceId selectedSource() const = 0;

    virtual CaptureBackend backend() const { return backend_; }
    virtual CaptureError lastError() const { return lastError_; }

    static std::unique_ptr<DesktopCapturer> createScreenCapturer(
        const CaptureOptions& options = CaptureOptions::defaultOptions());

    static std::unique_ptr<DesktopCapturer> createWindowCapturer(
        const CaptureOptions& options = CaptureOptions::defaultOptions());

protected:
    void setBackend(CaptureBackend backend) { backend_ = backend; }
    void setLastError(CaptureError error) { lastError_ = error; }

    CaptureOptions options_;
    CaptureBackend backend_{CaptureBackend::Unknown};
    CaptureError lastError_{CaptureError::Ok};
};

}  // namespace desktop_capture
}  // namespace links

#endif  // DESKTOP_CAPTURE_DESKTOP_CAPTURER_H_
