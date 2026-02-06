#include "thumbnail_service.h"

#include "platform_window_ops.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <utility>

namespace links {
namespace core {
namespace {

bool isTargetSizeValid(const ImageSize& size)
{
    return size.width > 0 && size.height > 0;
}

RawImage resizeKeepAspect(const RawImage& src, const ImageSize& target)
{
    const double srcAspect = static_cast<double>(src.width) / static_cast<double>(src.height);
    int outputWidth = target.width;
    int outputHeight = static_cast<int>(std::lround(static_cast<double>(outputWidth) / srcAspect));

    if (outputHeight > target.height) {
        outputHeight = target.height;
        outputWidth = static_cast<int>(std::lround(static_cast<double>(outputHeight) * srcAspect));
    }

    outputWidth = std::max(outputWidth, 1);
    outputHeight = std::max(outputHeight, 1);

    RawImage resized;
    resized.width = outputWidth;
    resized.height = outputHeight;
    resized.stride = outputWidth * 4;
    resized.format = src.format;
    resized.pixels.resize(static_cast<std::size_t>(resized.stride) * static_cast<std::size_t>(resized.height));

    for (int y = 0; y < outputHeight; ++y) {
        const int srcY = std::min((y * src.height) / outputHeight, src.height - 1);
        const std::uint8_t* srcRow = src.pixels.data() + static_cast<std::size_t>(srcY) * static_cast<std::size_t>(src.stride);
        std::uint8_t* dstRow = resized.pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(resized.stride);

        for (int x = 0; x < outputWidth; ++x) {
            const int srcX = std::min((x * src.width) / outputWidth, src.width - 1);
            const std::uint8_t* srcPixel = srcRow + srcX * 4;
            std::uint8_t* dstPixel = dstRow + x * 4;
            std::memcpy(dstPixel, srcPixel, 4);
        }
    }

    return resized;
}

}  // namespace

std::vector<std::optional<RawImage>> ThumbnailService::captureWindowThumbnails(
    const std::vector<WindowInfo>& windows,
    ImageSize targetSize) const
{
    std::vector<std::optional<RawImage>> thumbnails;
    thumbnails.reserve(windows.size());

    for (const auto& window : windows) {
        thumbnails.push_back(captureWindowThumbnail(window, targetSize));
    }

    return thumbnails;
}

std::optional<RawImage> ThumbnailService::captureWindowThumbnail(const WindowInfo& info, ImageSize targetSize) const
{
    if (info.id == 0) {
        return std::nullopt;
    }

    std::optional<RawImage> image = captureWindowWithWinRt(info.id);
    if (!image || !image->isValid()) {
        image = captureWindowWithPrintApi(info.id);
    }

    if (!image || !image->isValid()) {
        return std::nullopt;
    }

    if (!isTargetSizeValid(targetSize)) {
        return image;
    }

    if (image->width <= targetSize.width && image->height <= targetSize.height) {
        return image;
    }

    RawImage resized = resizeKeepAspect(*image, targetSize);
    if (!resized.isValid()) {
        return std::nullopt;
    }
    return resized;
}

}  // namespace core
}  // namespace links
