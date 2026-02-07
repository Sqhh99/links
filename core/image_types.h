#ifndef CORE_IMAGE_TYPES_H
#define CORE_IMAGE_TYPES_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace links {
namespace core {

enum class PixelFormat {
    RGBA8888,
    BGRA8888
};

struct ImageSize {
    int width{0};
    int height{0};
};

struct RawImage {
    int width{0};
    int height{0};
    int stride{0};
    PixelFormat format{PixelFormat::RGBA8888};
    std::vector<std::uint8_t> pixels;

    bool isValid() const
    {
        if (width <= 0 || height <= 0 || stride < width * 4) {
            return false;
        }
        const auto expectedSize = static_cast<std::size_t>(stride) * static_cast<std::size_t>(height);
        return pixels.size() >= expectedSize;
    }
};

}  // namespace core
}  // namespace links

#endif  // CORE_IMAGE_TYPES_H
