# =============================================================================
# Audio Processing Tests
# Tests for AudioProcessingModule and MicrophoneCapturer
# =============================================================================

add_executable(audio_processing_tests
    core/test_audio_processing_module.cpp
)

target_sources(audio_processing_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/audio_processing_module.cpp
)

target_compile_definitions(audio_processing_tests PRIVATE
    AUDIO_PROCESSING_TESTS
)

set_target_properties(audio_processing_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(audio_processing_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${WEBRTC_APM_LIBRARY}
)

target_include_directories(audio_processing_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/utils
    ${WEBRTC_APM_INCLUDE_DIR}
    ${WEBRTC_ABSEIL_INCLUDE_DIR}
)

# =============================================================================
# Microphone Capturer Tests (requires Qt and LiveKit)
# =============================================================================

add_executable(microphone_capturer_tests
    core/test_microphone_capturer.cpp
    ${CMAKE_SOURCE_DIR}/core/microphone_capturer.cpp
    ${CMAKE_SOURCE_DIR}/core/audio_processing_module.cpp
    ${CMAKE_SOURCE_DIR}/utils/logger.cpp
)

set_target_properties(microphone_capturer_tests PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(microphone_capturer_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
    Qt6::Multimedia
    ${LIVEKIT_LIBRARY}
    ${LIVEKIT_FFI_LIBRARY}
    ${WEBRTC_APM_LIBRARY}
)

target_include_directories(microphone_capturer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/utils
    ${LIVEKIT_INCLUDE_DIR}
    ${WEBRTC_APM_INCLUDE_DIR}
    ${WEBRTC_ABSEIL_INCLUDE_DIR}
)

if(UNIX)
    set_target_properties(audio_processing_tests PROPERTIES
        BUILD_RPATH "${WEBRTC_APM_BIN_DIR}"
    )
    set_target_properties(microphone_capturer_tests PROPERTIES
        BUILD_RPATH "${WEBRTC_APM_BIN_DIR};${LIVEKIT_BIN_DIR}"
    )
endif()

# =============================================================================
# Desktop Capture Unit Tests
# =============================================================================

add_executable(desktop_capture_tests
    core/test_desktop_geometry.cpp
    core/test_desktop_frame.cpp
    ${CMAKE_SOURCE_DIR}/core/desktop_capture/desktop_frame.cpp
)

set_target_properties(desktop_capture_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

target_link_libraries(desktop_capture_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
)

target_include_directories(desktop_capture_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/utils
)

# =============================================================================
# Cross-platform capture tests (macOS/Linux only)
# =============================================================================

if(APPLE OR (UNIX AND NOT WIN32))
    set(CAPTURE_PLATFORM_OPS_SOURCES)
    set(CAPTURE_PLATFORM_CAPTURER_SOURCES)
    set(CAPTURE_PLATFORM_LIBS)

    if(APPLE)
        list(APPEND CAPTURE_PLATFORM_OPS_SOURCES
            ${CMAKE_SOURCE_DIR}/core/desktop_capture/mac/platform_window_ops_mac.cpp
            ${CMAKE_SOURCE_DIR}/core/desktop_capture/mac/screen_capture_kit_adapter.mm
        )
        list(APPEND CAPTURE_PLATFORM_CAPTURER_SOURCES
            ${CMAKE_SOURCE_DIR}/core/desktop_capture/mac/mac_capturer.cpp
        )

        find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
        find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
        find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
        find_library(CORE_MEDIA_FRAMEWORK CoreMedia REQUIRED)
        find_library(CORE_VIDEO_FRAMEWORK CoreVideo REQUIRED)
        find_library(AVFOUNDATION_FRAMEWORK AVFoundation REQUIRED)
        find_library(SCREEN_CAPTURE_KIT_FRAMEWORK ScreenCaptureKit)
        list(APPEND CAPTURE_PLATFORM_LIBS
            ${CORE_GRAPHICS_FRAMEWORK}
            ${CORE_FOUNDATION_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
            ${CORE_MEDIA_FRAMEWORK}
            ${CORE_VIDEO_FRAMEWORK}
            ${AVFOUNDATION_FRAMEWORK}
        )
        if(SCREEN_CAPTURE_KIT_FRAMEWORK)
            list(APPEND CAPTURE_PLATFORM_LIBS ${SCREEN_CAPTURE_KIT_FRAMEWORK})
        endif()
    elseif(UNIX)
        list(APPEND CAPTURE_PLATFORM_OPS_SOURCES
            ${CMAKE_SOURCE_DIR}/core/desktop_capture/linux/x11/platform_window_ops_linux_x11.cpp
        )
        list(APPEND CAPTURE_PLATFORM_CAPTURER_SOURCES
            ${CMAKE_SOURCE_DIR}/core/desktop_capture/linux/x11/x11_capturer.cpp
        )

        find_package(X11 REQUIRED)
        list(APPEND CAPTURE_PLATFORM_LIBS X11::X11)
    endif()

    add_executable(capture_platform_tests
        core/test_platform_window_ops_capability.cpp
        core/test_thumbnail_service.cpp
        ${CMAKE_SOURCE_DIR}/core/platform_window_ops.cpp
        ${CMAKE_SOURCE_DIR}/core/thumbnail_service.cpp
        ${CAPTURE_PLATFORM_OPS_SOURCES}
    )

    set_target_properties(capture_platform_tests PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
    )

    target_link_libraries(capture_platform_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        ${CAPTURE_PLATFORM_LIBS}
    )

    target_include_directories(capture_platform_tests PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/utils
    )

    add_executable(desktop_capture_integration_tests
        integration/test_screen_capture_smoke.cpp
        integration/test_window_capture_smoke.cpp
        integration/test_capture_backend_benchmark.cpp
        ${CMAKE_SOURCE_DIR}/core/desktop_capture/desktop_frame.cpp
        ${CMAKE_SOURCE_DIR}/core/desktop_capture/desktop_capturer.cpp
        ${CMAKE_SOURCE_DIR}/core/platform_window_ops.cpp
        ${CAPTURE_PLATFORM_OPS_SOURCES}
        ${CAPTURE_PLATFORM_CAPTURER_SOURCES}
    )

    set_target_properties(desktop_capture_integration_tests PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
    )

    target_link_libraries(desktop_capture_integration_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        ${CAPTURE_PLATFORM_LIBS}
    )

    target_include_directories(desktop_capture_integration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/utils
    )
endif()

# =============================================================================
# Test Discovery
# =============================================================================

include(GoogleTest)
gtest_discover_tests(audio_processing_tests DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(microphone_capturer_tests DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(desktop_capture_tests DISCOVERY_MODE PRE_TEST)

if(TARGET capture_platform_tests)
    gtest_discover_tests(capture_platform_tests DISCOVERY_MODE PRE_TEST)
endif()

if(TARGET desktop_capture_integration_tests)
    gtest_discover_tests(desktop_capture_integration_tests DISCOVERY_MODE PRE_TEST)
endif()

# =============================================================================
# Runtime dependencies for tests
# =============================================================================

function(copy_runtime_if_exists target source_file)
    if(EXISTS "${source_file}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${source_file}"
                "$<TARGET_FILE_DIR:${target}>/"
            COMMENT "Copying runtime dependency for ${target}: ${source_file}"
        )
    endif()
endfunction()

if(WIN32)
    if(DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET STREQUAL "")
        set(GTEST_DLL_DIR "${CMAKE_SOURCE_DIR}/third_party/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
    else()
        set(GTEST_DLL_DIR "${CMAKE_SOURCE_DIR}/third_party/vcpkg_installed/x64-windows/bin")
    endif()

    copy_runtime_if_exists(audio_processing_tests "${GTEST_DLL_DIR}/gtest.dll")
    copy_runtime_if_exists(audio_processing_tests "${GTEST_DLL_DIR}/gtest_main.dll")
    copy_runtime_if_exists(audio_processing_tests "${WEBRTC_APM_SHARED_LIB}")

    copy_runtime_if_exists(microphone_capturer_tests "${GTEST_DLL_DIR}/gtest.dll")
    copy_runtime_if_exists(microphone_capturer_tests "${GTEST_DLL_DIR}/gtest_main.dll")
    copy_runtime_if_exists(microphone_capturer_tests "${WEBRTC_APM_SHARED_LIB}")
    copy_runtime_if_exists(microphone_capturer_tests "${LIVEKIT_BIN_DIR}/livekit.dll")
    copy_runtime_if_exists(microphone_capturer_tests "${LIVEKIT_BIN_DIR}/livekit_ffi.dll")

    copy_runtime_if_exists(desktop_capture_tests "${GTEST_DLL_DIR}/gtest.dll")
    copy_runtime_if_exists(desktop_capture_tests "${GTEST_DLL_DIR}/gtest_main.dll")
endif()
