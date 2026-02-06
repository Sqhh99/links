#include <gtest/gtest.h>

#include "core/thumbnail_service.h"

TEST(ThumbnailServiceTest, EmptyInputReturnsEmptyBatch)
{
    links::core::ThumbnailService service;
    const links::core::ImageSize targetSize{240, 140};
    const std::vector<links::core::WindowInfo> windows;

    const auto result = service.captureWindowThumbnails(windows, targetSize);
    EXPECT_TRUE(result.empty());
}

TEST(ThumbnailServiceTest, InvalidTargetSizeReturnsNoThumbnail)
{
    links::core::ThumbnailService service;

    links::core::WindowInfo window;
    window.id = 1;
    window.title = "invalid-target";

    const auto thumbnail = service.captureWindowThumbnail(window, links::core::ImageSize{0, 100});
    EXPECT_FALSE(thumbnail.has_value());
}
