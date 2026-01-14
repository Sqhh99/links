# Test executable
add_executable(links_tests
    test_main.cpp
    # Add more test files here:
    # core/test_conference_manager.cpp
    # core/test_media_manager.cpp
    core/test_desktop_geometry.cpp
    core/test_desktop_frame.cpp
    ${CMAKE_SOURCE_DIR}/core/desktop_capture/desktop_frame.cpp
)

set_target_properties(links_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(links_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Include project headers for testing
target_include_directories(links_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/ui
)

# Discover and register tests with CTest
include(GoogleTest)
gtest_discover_tests(links_tests
    DISCOVERY_MODE PRE_TEST
)
