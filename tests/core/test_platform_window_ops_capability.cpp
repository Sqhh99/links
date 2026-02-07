#include <gtest/gtest.h>

#include "core/platform_window_ops.h"

TEST(PlatformWindowOpsCapabilityTest, CapabilityQueryDoesNotCrash)
{
    const bool screenSupported = links::core::isScreenShareSupportedOnCurrentPlatform();
    const bool windowSupported = links::core::isWindowShareSupportedOnCurrentPlatform();

    const auto windows = links::core::enumerateWindows();
    if (!windowSupported) {
        EXPECT_TRUE(windows.empty());
    }
    (void)screenSupported;
}

TEST(PlatformWindowOpsCapabilityTest, InvalidWindowCaptureReturnsEmpty)
{
    const auto winRtImage = links::core::captureWindowWithWinRt(0);
    EXPECT_FALSE(winRtImage.has_value());

    const auto fallbackImage = links::core::captureWindowWithPrintApi(0);
    EXPECT_FALSE(fallbackImage.has_value());
}
