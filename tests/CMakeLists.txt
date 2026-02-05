# =============================================================================
# Audio Processing Tests
# Tests for AudioProcessingModule and MicrophoneCapturer
# =============================================================================

# AudioProcessingModule does NOT depend on Logger/Qt when compiled alone
# So we create a test-only version that doesn't include logger

# Audio processing module tests (minimal dependencies)
add_executable(audio_processing_tests
    core/test_audio_processing_module.cpp
)

# Create a test-only version of audio_processing_module without Qt logger
target_sources(audio_processing_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/audio_processing_module.cpp
)

# Define a macro to disable logging in tests
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

# =============================================================================
# Desktop Capture Tests (existing)
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
# Test Discovery
# =============================================================================

include(GoogleTest)
gtest_discover_tests(audio_processing_tests DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(microphone_capturer_tests DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(desktop_capture_tests DISCOVERY_MODE PRE_TEST)

# =============================================================================
# Copy DLLs to test output directories
# =============================================================================

# GTest DLLs (from vcpkg)
set(GTEST_DLL_DIR "${CMAKE_SOURCE_DIR}/third_party/vcpkg_installed/x64-windows/bin")

# Copy GTest and WebRTC APM DLLs for audio_processing_tests
add_custom_command(TARGET audio_processing_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GTEST_DLL_DIR}/gtest.dll"
        "$<TARGET_FILE_DIR:audio_processing_tests>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GTEST_DLL_DIR}/gtest_main.dll"
        "$<TARGET_FILE_DIR:audio_processing_tests>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WEBRTC_APM_ROOT}/bin/webrtc-audio-processing-2-1.dll"
        "$<TARGET_FILE_DIR:audio_processing_tests>/"
    COMMENT "Copying DLLs for audio_processing_tests"
)

# Copy all required DLLs for microphone_capturer_tests
add_custom_command(TARGET microphone_capturer_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GTEST_DLL_DIR}/gtest.dll"
        "$<TARGET_FILE_DIR:microphone_capturer_tests>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GTEST_DLL_DIR}/gtest_main.dll"
        "$<TARGET_FILE_DIR:microphone_capturer_tests>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WEBRTC_APM_ROOT}/bin/webrtc-audio-processing-2-1.dll"
        "$<TARGET_FILE_DIR:microphone_capturer_tests>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIVEKIT_SDK_ROOT}/bin/livekit.dll"
        "$<TARGET_FILE_DIR:microphone_capturer_tests>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIVEKIT_SDK_ROOT}/bin/livekit_ffi.dll"
        "$<TARGET_FILE_DIR:microphone_capturer_tests>/"
    COMMENT "Copying DLLs for microphone_capturer_tests"
)

# Copy GTest DLLs for desktop_capture_tests
add_custom_command(TARGET desktop_capture_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GTEST_DLL_DIR}/gtest.dll"
        "$<TARGET_FILE_DIR:desktop_capture_tests>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GTEST_DLL_DIR}/gtest_main.dll"
        "$<TARGET_FILE_DIR:desktop_capture_tests>/"
    COMMENT "Copying DLLs for desktop_capture_tests"
)
