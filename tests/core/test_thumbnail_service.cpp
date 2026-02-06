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

TEST(ThumbnailServiceTest, InvalidWindowReturnsNoThumbnail)
{
    links::core::ThumbnailService service;

    links::core::WindowInfo window;
    window.id = 0;
    window.title = "invalid-window";

    const auto thumbnails = service.captureWindowThumbnails(
        std::vector<links::core::WindowInfo>{window},
        links::core::ImageSize{0, 100});

    ASSERT_EQ(thumbnails.size(), 1u);
    EXPECT_FALSE(thumbnails.front().has_value());
}
