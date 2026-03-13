#include <gtest/gtest.h>

#include "core/recording/frame_clock.h"
#include "core/recording/recording_layout.h"

TEST(RecordingLayoutTest, GridLayoutForFourParticipants)
{
    const auto layout = links::recording::buildGridLayout(1920, 1080, 4);
    EXPECT_EQ(layout.cols, 2);
    EXPECT_EQ(layout.rows, 2);
    ASSERT_EQ(layout.cells.size(), 4u);
    EXPECT_EQ(layout.cells[0].width, 960);
    EXPECT_EQ(layout.cells[0].height, 540);
}

TEST(RecordingLayoutTest, ScreenShareThumbnailsCappedByMaxCount)
{
    const auto layout = links::recording::buildScreenShareLayout(1920, 1080, 9);
    EXPECT_EQ(layout.screenRect.width, 1920);
    EXPECT_EQ(layout.screenRect.height, 1080);
    EXPECT_EQ(layout.thumbnailRects.size(), 4u);
}

TEST(RecordingClockTest, TimestampAlwaysMonotonic)
{
    const std::int64_t intervalUs = 33333;
    const std::int64_t first = links::recording::computeNextFrameStartUs(1000, -1, intervalUs, 0);
    const std::int64_t second = links::recording::computeNextFrameStartUs(900, first, intervalUs, 1);
    EXPECT_GT(second, first);
}
