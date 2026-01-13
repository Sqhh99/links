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

// Abstract interface for screen and window capturers.
class DesktopCapturer {
public:
    // Result of a capture operation
    enum class Result {
        // Frame was captured successfully
        SUCCESS,
        // Temporary error, caller should retry
        ERROR_TEMPORARY,
        // Permanent error, capture cannot continue
        ERROR_PERMANENT
    };

    // Callback interface for receiving captured frames
    class Callback {
    public:
        virtual ~Callback() = default;

        // Called when a frame has been captured
        // frame is nullptr if result is not SUCCESS
        virtual void onCaptureResult(Result result,
                                     std::unique_ptr<DesktopFrame> frame) = 0;
    };

    // Source identifier type (window handle or screen index)
#ifdef _WIN32
    using SourceId = intptr_t;
#else
    using SourceId = int64_t;
#endif

    // Information about a capturable source (screen or window)
    struct Source {
        SourceId id = 0;
        std::string title;
        int64_t displayId = -1;  // For screens, the display identifier
    };

    using SourceList = std::vector<Source>;

    virtual ~DesktopCapturer() = default;

    // Start capturing with the given callback
    // callback must remain valid until stop() is called or capturer is destroyed
    virtual void start(Callback* callback) = 0;

    // Stop capturing and release resources
    virtual void stop() = 0;

    // Capture a single frame (result delivered via callback)
    virtual void captureFrame() = 0;

    // Get list of available sources
    virtual bool getSourceList(SourceList* sources) = 0;

    // Select a source to capture
    virtual bool selectSource(SourceId id) = 0;

    // Check if a source is valid and can be captured
    virtual bool isSourceValid(SourceId id) = 0;

    // Get the currently selected source
    virtual SourceId selectedSource() const = 0;

    // Factory methods to create platform-specific capturers

    // Create a screen capturer for the current platform
    static std::unique_ptr<DesktopCapturer> createScreenCapturer(
        const CaptureOptions& options = CaptureOptions::defaultOptions());

    // Create a window capturer for the current platform
    static std::unique_ptr<DesktopCapturer> createWindowCapturer(
        const CaptureOptions& options = CaptureOptions::defaultOptions());

protected:
    CaptureOptions options_;
};

}  // namespace desktop_capture
}  // namespace links

#endif  // DESKTOP_CAPTURE_DESKTOP_CAPTURER_H_
