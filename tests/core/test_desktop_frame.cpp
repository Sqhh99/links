#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "desktop_capture/desktop_frame.h"

namespace links {
namespace desktop_capture {

namespace {

void FillPattern(BasicDesktopFrame& frame) {
    for (int y = 0; y < frame.height(); ++y) {
        uint8_t* row = frame.dataAt(y);
        for (int x = 0; x < frame.width(); ++x) {
            row[x * 4 + 0] = static_cast<uint8_t>(x);
            row[x * 4 + 1] = static_cast<uint8_t>(y);
            row[x * 4 + 2] = static_cast<uint8_t>(x + y);
            row[x * 4 + 3] = 0xFF;
        }
    }
}

}  // namespace

TEST(DesktopFrameTest, CopyPixelsFrom) {
    BasicDesktopFrame src(DesktopSize(3, 3));
    FillPattern(src);

    BasicDesktopFrame dst(DesktopSize(4, 4));
    std::fill(dst.data(), dst.data() + dst.height() * dst.stride(), 0);

    dst.copyPixelsFrom(src, DesktopVector(1, 1), DesktopRect::makeXYWH(0, 0, 2, 2));

    for (int y = 0; y < 2; ++y) {
        const uint8_t* row = dst.dataAt(y);
        for (int x = 0; x < 2; ++x) {
            EXPECT_EQ(row[x * 4 + 0], static_cast<uint8_t>(x + 1));
            EXPECT_EQ(row[x * 4 + 1], static_cast<uint8_t>(y + 1));
            EXPECT_EQ(row[x * 4 + 2], static_cast<uint8_t>(x + y + 2));
            EXPECT_EQ(row[x * 4 + 3], 0xFF);
        }
    }
}

TEST(DesktopFrameTest, CopyToVector) {
    BasicDesktopFrame frame(DesktopSize(2, 2));
    FillPattern(frame);

    std::vector<uint8_t> data = frame.copyToVector();
    ASSERT_EQ(data.size(), static_cast<size_t>(2 * 2 * DesktopFrame::kBytesPerPixel));

    EXPECT_EQ(data[0], 0);
    EXPECT_EQ(data[1], 0);
    EXPECT_EQ(data[2], 0);
    EXPECT_EQ(data[3], 0xFF);

    EXPECT_EQ(data[4], 1);
    EXPECT_EQ(data[5], 0);
    EXPECT_EQ(data[6], 1);
    EXPECT_EQ(data[7], 0xFF);
}

TEST(DesktopFrameTest, CopyOfPreservesMetadataAndPixels) {
    BasicDesktopFrame frame(DesktopSize(2, 2));
    FillPattern(frame);
    frame.setCaptureTimeUs(12345);
    frame.setDpi(DesktopVector(120, 120));
    frame.setUpdatedRegion(DesktopRect::makeXYWH(1, 1, 1, 1));

    auto copy = BasicDesktopFrame::copyOf(frame);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->size(), frame.size());
    EXPECT_EQ(copy->captureTimeUs(), frame.captureTimeUs());
    EXPECT_EQ(copy->dpi(), frame.dpi());
    EXPECT_EQ(copy->updatedRegion(), frame.updatedRegion());

    for (int y = 0; y < frame.height(); ++y) {
        const uint8_t* srcRow = frame.dataAt(y);
        const uint8_t* dstRow = copy->dataAt(y);
        for (int x = 0; x < frame.width() * DesktopFrame::kBytesPerPixel; ++x) {
            EXPECT_EQ(dstRow[x], srcRow[x]);
        }
    }
}

}  // namespace desktop_capture
}  // namespace links
