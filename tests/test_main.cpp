#include <gtest/gtest.h>

// =============================================================================
// Sanity Check Tests
// Verify that GTest is properly configured and working
// =============================================================================

TEST(SanityCheck, BasicAssertion) {
    EXPECT_EQ(1 + 1, 2);
}

TEST(SanityCheck, StringComparison) {
    std::string expected = "links";
    EXPECT_EQ(expected, "links");
}

// =============================================================================
// Add your test files here:
//
// Example structure:
//   tests/
//     CMakeLists.txt
//     test_main.cpp
//     core/
//       test_conference_manager.cpp
//       test_media_manager.cpp
//       test_network_client.cpp
//     utils/
//       test_logger.cpp
//       test_settings.cpp
//
// To add new test files:
// 1. Create the test file in the appropriate subdirectory
// 2. Add the file to tests/CMakeLists.txt
// =============================================================================
