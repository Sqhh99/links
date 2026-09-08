#ifndef CORE_MEDIA_VIDEO_FRAME_H
#define CORE_MEDIA_VIDEO_FRAME_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "../image_types.h"

namespace links {
namespace core {

/**
 * Immutable, cheaply copyable RGBA/BGRA frame -- the replacement for QImage on
 * the core video path.
 *
 * Copying is a shared_ptr bump, which is the same O(1) fan-out QImage gave us
 * through implicit sharing. Passing core::RawImage by value instead would add
 * a full-frame memcpy at every hop (MediaPipeline -> DeviceController ->
 * ConferenceManager -> ConferenceBackend -> VideoRenderer, plus
 * LocalRecordingManager), for every participant, at 15-30 fps.
 */
class VideoFrame {
public:
    VideoFrame() = default;

    /// Take ownership of an already-built RawImage. No copy.
    static VideoFrame adopt(RawImage&& image)
    {
        VideoFrame frame;
        if (image.isValid()) {
            frame.data_ = std::make_shared<const RawImage>(std::move(image));
        }
        return frame;
    }

    /**
     * One deep copy out of a foreign buffer (a livekit::VideoFrame, a
     * DesktopFrame, a mapped QVideoFrame). Rows are repacked so that
     * stride == width * 4.
     *
     * The repack matters: DXGI can hand back a stride wider than width*4, and
     * QImage::copy() silently normalised that. Skipping it would tear or skew
     * screen-share frames.
     */
    static VideoFrame copyFrom(const std::uint8_t* source,
                               int width,
                               int height,
                               int stride,
                               PixelFormat format);

    bool isValid() const { return static_cast<bool>(data_); }
    int width() const { return data_ ? data_->width : 0; }
    int height() const { return data_ ? data_->height : 0; }
    int stride() const { return data_ ? data_->stride : 0; }
    PixelFormat format() const { return data_ ? data_->format : PixelFormat::RGBA8888; }

    const std::uint8_t* bits() const { return data_ ? data_->pixels.data() : nullptr; }
    std::size_t sizeInBytes() const { return data_ ? data_->pixels.size() : 0; }

    /// Tightly packed RGBA, ready to move into a livekit::VideoFrame.
    std::vector<std::uint8_t> toPackedRgba() const;

    /**
     * Keeps the pixel buffer alive independently of this VideoFrame. The Qt
     * adapter uses it to build a QImage that borrows these bytes without
     * copying them.
     */
    std::shared_ptr<const RawImage> buffer() const { return data_; }

private:
    std::shared_ptr<const RawImage> data_;
};

}  // namespace core
}  // namespace links

#endif  // CORE_MEDIA_VIDEO_FRAME_H
