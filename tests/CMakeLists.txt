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

# Ensure test executables can resolve third-party shared libraries at runtime.
if(UNIX)
    set_target_properties(audio_processing_tests PROPERTIES
        BUILD_RPATH "${WEBRTC_APM_BIN_DIR}"
    )
    set_target_properties(microphone_capturer_tests PROPERTIES
        BUILD_RPATH "${WEBRTC_APM_BIN_DIR};${LIVEKIT_BIN_DIR}"
    )
endif()

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

# On Windows, tests need runtime DLLs copied next to test binaries.
# On Linux/macOS, shared library lookup is handled by loader paths/RPATH.
if(WIN32)
    if(DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET STREQUAL "")
        set(GTEST_DLL_DIR "${CMAKE_SOURCE_DIR}/third_party/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
    else()
        set(GTEST_DLL_DIR "${CMAKE_SOURCE_DIR}/third_party/vcpkg_installed/x64-windows/bin")
    endif()

    # audio_processing_tests
    copy_runtime_if_exists(audio_processing_tests "${GTEST_DLL_DIR}/gtest.dll")
    copy_runtime_if_exists(audio_processing_tests "${GTEST_DLL_DIR}/gtest_main.dll")
    copy_runtime_if_exists(audio_processing_tests "${WEBRTC_APM_SHARED_LIB}")

    # microphone_capturer_tests
    copy_runtime_if_exists(microphone_capturer_tests "${GTEST_DLL_DIR}/gtest.dll")
    copy_runtime_if_exists(microphone_capturer_tests "${GTEST_DLL_DIR}/gtest_main.dll")
    copy_runtime_if_exists(microphone_capturer_tests "${WEBRTC_APM_SHARED_LIB}")
    copy_runtime_if_exists(microphone_capturer_tests "${LIVEKIT_BIN_DIR}/livekit.dll")
    copy_runtime_if_exists(microphone_capturer_tests "${LIVEKIT_BIN_DIR}/livekit_ffi.dll")

    # desktop_capture_tests
    copy_runtime_if_exists(desktop_capture_tests "${GTEST_DLL_DIR}/gtest.dll")
    copy_runtime_if_exists(desktop_capture_tests "${GTEST_DLL_DIR}/gtest_main.dll")
endif()
