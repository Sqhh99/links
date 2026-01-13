/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Frame Data Container
 */

#ifndef DESKTOP_CAPTURE_DESKTOP_FRAME_H_
#define DESKTOP_CAPTURE_DESKTOP_FRAME_H_

#include <cstdint>
#include <memory>
#include <vector>
#include "desktop_geometry.h"

namespace links {
namespace desktop_capture {

// Represents a single captured frame.
// The frame data is stored in RGBA format (4 bytes per pixel).
class DesktopFrame {
public:
    DesktopFrame(const DesktopSize& size, int stride, uint8_t* data);
    virtual ~DesktopFrame();

    // Disable copy
    DesktopFrame(const DesktopFrame&) = delete;
    DesktopFrame& operator=(const DesktopFrame&) = delete;

    // Size of the frame in pixels
    const DesktopSize& size() const { return size_; }
    int width() const { return size_.width(); }
    int height() const { return size_.height(); }

    // Number of bytes per row
    int stride() const { return stride_; }

    // Pointer to the raw pixel data (RGBA format)
    uint8_t* data() const { return data_; }

    // Pointer to data at specific row
    uint8_t* dataAt(int y) const { return data_ + y * stride_; }
    uint8_t* dataAt(const DesktopVector& pos) const {
        return dataAt(pos.y()) + pos.x() * kBytesPerPixel;
    }

    // DPI of the screen where the frame was captured
    DesktopVector dpi() const { return dpi_; }
    void setDpi(const DesktopVector& dpi) { dpi_ = dpi; }

    // Capture timestamp in microseconds
    int64_t captureTimeUs() const { return captureTimeUs_; }
    void setCaptureTimeUs(int64_t time) { captureTimeUs_ = time; }

    // The portion of the frame that contains updated content
    const DesktopRect& updatedRegion() const { return updatedRegion_; }
    void setUpdatedRegion(const DesktopRect& region) { updatedRegion_ = region; }

    // Copies pixels from another frame
    void copyPixelsFrom(const DesktopFrame& src, const DesktopVector& srcPos,
                        const DesktopRect& destRect);

    // Copy entire frame data to vector
    std::vector<uint8_t> copyToVector() const;

    static constexpr int kBytesPerPixel = 4;

protected:
    DesktopFrame(const DesktopSize& size, int stride);

    DesktopSize size_;
    int stride_;
    uint8_t* data_;
    DesktopVector dpi_;
    int64_t captureTimeUs_ = 0;
    DesktopRect updatedRegion_;
};

// A DesktopFrame that owns its own data buffer
class BasicDesktopFrame : public DesktopFrame {
public:
    explicit BasicDesktopFrame(const DesktopSize& size);
    ~BasicDesktopFrame() override;

    // Create a copy of another frame
    static std::unique_ptr<BasicDesktopFrame> copyOf(const DesktopFrame& frame);

private:
    std::vector<uint8_t> buffer_;
};

}  // namespace desktop_capture
}  // namespace links

#endif  // DESKTOP_CAPTURE_DESKTOP_FRAME_H_
