/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Frame Data Container Implementation
 */

#include "desktop_frame.h"
#include <cstring>
#include <algorithm>

namespace links {
namespace desktop_capture {

DesktopFrame::DesktopFrame(const DesktopSize& size, int stride)
    : size_(size), stride_(stride), data_(nullptr) {}

DesktopFrame::DesktopFrame(const DesktopSize& size, int stride, uint8_t* data)
    : size_(size), stride_(stride), data_(data) {}

DesktopFrame::~DesktopFrame() = default;

void DesktopFrame::copyPixelsFrom(const DesktopFrame& src,
                                   const DesktopVector& srcPos,
                                   const DesktopRect& destRect) {
    if (destRect.isEmpty()) {
        return;
    }

    const uint8_t* srcData = src.dataAt(srcPos);
    uint8_t* destData = dataAt(destRect.topLeft());
    int bytesPerRow = destRect.width() * kBytesPerPixel;

    for (int y = 0; y < destRect.height(); ++y) {
        std::memcpy(destData, srcData, bytesPerRow);
        srcData += src.stride();
        destData += stride();
    }
}

std::vector<uint8_t> DesktopFrame::copyToVector() const {
    std::vector<uint8_t> result;
    result.reserve(static_cast<size_t>(height()) * stride());

    for (int y = 0; y < height(); ++y) {
        const uint8_t* row = dataAt(y);
        result.insert(result.end(), row, row + width() * kBytesPerPixel);
    }
    return result;
}

// BasicDesktopFrame implementation
BasicDesktopFrame::BasicDesktopFrame(const DesktopSize& size)
    : DesktopFrame(size, size.width() * kBytesPerPixel) {
    size_t bufferSize = static_cast<size_t>(stride_) * size.height();
    buffer_.resize(bufferSize);
    data_ = buffer_.data();
    updatedRegion_ = DesktopRect::makeSize(size);
}

BasicDesktopFrame::~BasicDesktopFrame() = default;

std::unique_ptr<BasicDesktopFrame> BasicDesktopFrame::copyOf(const DesktopFrame& frame) {
    auto result = std::make_unique<BasicDesktopFrame>(frame.size());
    result->setDpi(frame.dpi());
    result->setCaptureTimeUs(frame.captureTimeUs());
    result->setUpdatedRegion(frame.updatedRegion());

    for (int y = 0; y < frame.height(); ++y) {
        std::memcpy(result->dataAt(y), frame.dataAt(y),
                    frame.width() * kBytesPerPixel);
    }
    return result;
}

}  // namespace desktop_capture
}  // namespace links
