#ifndef CORE_THUMBNAIL_SERVICE_H
#define CORE_THUMBNAIL_SERVICE_H

#include <optional>
#include <vector>

#include "image_types.h"
#include "window_types.h"

namespace links {
namespace core {

class ThumbnailService {
public:
    ThumbnailService() = default;
    ~ThumbnailService() = default;

    std::vector<std::optional<RawImage>> captureWindowThumbnails(
        const std::vector<WindowInfo>& windows,
        ImageSize targetSize) const;

private:
    std::optional<RawImage> captureWindowThumbnail(const WindowInfo& info, ImageSize targetSize) const;
};

}  // namespace core
}  // namespace links

#endif  // CORE_THUMBNAIL_SERVICE_H
