#include "video_frame.h"

#include <cstring>

namespace links {
namespace core {

VideoFrame VideoFrame::copyFrom(const std::uint8_t* source,
                                int width,
                                int height,
                                int stride,
                                PixelFormat format)
{
    if (!source || width <= 0 || height <= 0) {
        return VideoFrame();
    }

    const int packedStride = width * 4;
    if (stride < packedStride) {
        stride = packedStride;
    }

    RawImage image;
    image.width = width;
    image.height = height;
    image.stride = packedStride;
    image.format = format;
    image.pixels.resize(static_cast<std::size_t>(packedStride) * static_cast<std::size_t>(height));

    if (stride == packedStride) {
        std::memcpy(image.pixels.data(), source, image.pixels.size());
    } else {
        // Source rows are padded; drop the padding so downstream consumers can
        // assume stride == width * 4.
        for (int row = 0; row < height; ++row) {
            std::memcpy(image.pixels.data() + static_cast<std::size_t>(row) * packedStride,
                        source + static_cast<std::size_t>(row) * stride,
                        static_cast<std::size_t>(packedStride));
        }
    }

    return VideoFrame::adopt(std::move(image));
}

std::vector<std::uint8_t> VideoFrame::toPackedRgba() const
{
    if (!data_) {
        return {};
    }

    const int packedStride = data_->width * 4;
    std::vector<std::uint8_t> out(
        static_cast<std::size_t>(packedStride) * static_cast<std::size_t>(data_->height));

    if (data_->stride == packedStride) {
        std::memcpy(out.data(), data_->pixels.data(), out.size());
    } else {
        for (int row = 0; row < data_->height; ++row) {
            std::memcpy(out.data() + static_cast<std::size_t>(row) * packedStride,
                        data_->pixels.data() + static_cast<std::size_t>(row) * data_->stride,
                        static_cast<std::size_t>(packedStride));
        }
    }

    if (data_->format == PixelFormat::BGRA8888) {
        for (std::size_t i = 0; i + 3 < out.size(); i += 4) {
            std::swap(out[i], out[i + 2]);
        }
    }

    return out;
}

}  // namespace core
}  // namespace links
