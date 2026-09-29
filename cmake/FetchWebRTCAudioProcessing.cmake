# =============================================================================
# FetchWebRTCAudioProcessing.cmake
# Automatically download and configure WebRTC Audio Processing based on platform
# =============================================================================

# Release tag of https://github.com/Sqhh99/webrtc-audio-processing
# (can be overridden before including this module)
if(NOT DEFINED WEBRTC_APM_VERSION)
    set(WEBRTC_APM_VERSION "m153.8010.0.2")
endif()

# Shared SDK architecture selector across third-party fetch modules.
# Supported values: x64, arm64. Default is x64.
if(NOT DEFINED LINKS_SDK_ARCH)
    set(LINKS_SDK_ARCH "x64")
endif()
string(TOLOWER "${LINKS_SDK_ARCH}" WEBRTC_ARCH)
if(NOT WEBRTC_ARCH MATCHES "^(x64|arm64)$")
    message(FATAL_ERROR "Unsupported LINKS_SDK_ARCH: ${LINKS_SDK_ARCH}. Expected x64 or arm64.")
endif()

# =============================================================================
# Platform Detection
# =============================================================================
# Release assets are named webrtc-audio-processing.<platform>_<arch>.<ext>.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WEBRTC_PLATFORM "windows")
    set(WEBRTC_ARCHIVE_EXT "zip")
    set(WEBRTC_PACKAGE_ARCH_X64 "x86_64")
    set(WEBRTC_PACKAGE_ARCH_ARM64 "arm64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux packages are built per distro; ubuntu-24.04 matches the LiveKit SDK
    # (FetchLiveKitSDK.cmake). ubuntu-22.04 and ubuntu-26.04 are also published.
    set(WEBRTC_PLATFORM "ubuntu-24.04")
    set(WEBRTC_ARCHIVE_EXT "tar.gz")
    set(WEBRTC_PACKAGE_ARCH_X64 "x86_64")
    set(WEBRTC_PACKAGE_ARCH_ARM64 "armv8")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(WEBRTC_PLATFORM "macos")
    set(WEBRTC_ARCHIVE_EXT "tar.gz")
    set(WEBRTC_PACKAGE_ARCH_X64 "x86_64")
    set(WEBRTC_PACKAGE_ARCH_ARM64 "arm64")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

if(WEBRTC_ARCH STREQUAL "arm64")
    set(WEBRTC_PACKAGE_ARCH "${WEBRTC_PACKAGE_ARCH_ARM64}")
else()
    set(WEBRTC_PACKAGE_ARCH "${WEBRTC_PACKAGE_ARCH_X64}")
endif()

# =============================================================================
# SDK Paths
# =============================================================================
set(WEBRTC_APM_NAME "webrtc-audio-processing.${WEBRTC_PLATFORM}_${WEBRTC_PACKAGE_ARCH}")
set(WEBRTC_APM_ROOT "${CMAKE_SOURCE_DIR}/third_party/${WEBRTC_APM_NAME}")
set(WEBRTC_APM_ARCHIVE "${WEBRTC_APM_NAME}.${WEBRTC_ARCHIVE_EXT}")
set(WEBRTC_APM_URL "https://github.com/Sqhh99/webrtc-audio-processing/releases/download/${WEBRTC_APM_VERSION}/${WEBRTC_APM_ARCHIVE}")

# =============================================================================
# Download and Extract SDK if not present
# =============================================================================
if(NOT EXISTS "${WEBRTC_APM_ROOT}")
    message(STATUS "WebRTC Audio Processing not found at ${WEBRTC_APM_ROOT}")
    message(STATUS "Downloading WebRTC Audio Processing ${WEBRTC_APM_VERSION} (${WEBRTC_APM_NAME})...")

    set(WEBRTC_DOWNLOAD_PATH "${CMAKE_SOURCE_DIR}/third_party/${WEBRTC_APM_ARCHIVE}")

    file(DOWNLOAD
        "${WEBRTC_APM_URL}"
        "${WEBRTC_DOWNLOAD_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )

    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_ERROR_CODE)
    list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR_MESSAGE)

    if(NOT DOWNLOAD_ERROR_CODE EQUAL 0)
        file(REMOVE "${WEBRTC_DOWNLOAD_PATH}")
        message(FATAL_ERROR "Failed to download WebRTC Audio Processing from ${WEBRTC_APM_URL}: ${DOWNLOAD_ERROR_MESSAGE}")
    endif()

    message(STATUS "Extracting WebRTC Audio Processing...")

    # The archive has no top-level folder: bin/, include/, lib/ sit at its root
    file(MAKE_DIRECTORY "${WEBRTC_APM_ROOT}")
    file(ARCHIVE_EXTRACT
        INPUT "${WEBRTC_DOWNLOAD_PATH}"
        DESTINATION "${WEBRTC_APM_ROOT}"
    )

    file(REMOVE "${WEBRTC_DOWNLOAD_PATH}")

    message(STATUS "WebRTC Audio Processing ${WEBRTC_APM_VERSION} installed successfully")
else()
    message(STATUS "Found WebRTC Audio Processing at ${WEBRTC_APM_ROOT}")
endif()

# =============================================================================
# Configure SDK paths
# =============================================================================
set(WEBRTC_APM_INCLUDE_DIR "${WEBRTC_APM_ROOT}/include/webrtc-audio-processing-3")
set(WEBRTC_ABSEIL_INCLUDE_DIR "${WEBRTC_APM_ROOT}/include")

# Platform-specific library configuration
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WEBRTC_APM_LIBRARY "${WEBRTC_APM_ROOT}/lib/webrtc-audio-processing-3.lib")
    set(WEBRTC_APM_BIN_DIR "${WEBRTC_APM_ROOT}/bin")
    set(WEBRTC_APM_SHARED_LIB "${WEBRTC_APM_BIN_DIR}/webrtc-audio-processing-3-0.dll")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(WEBRTC_APM_LIBRARY "${WEBRTC_APM_ROOT}/lib/libwebrtc-audio-processing-3.so")
    set(WEBRTC_APM_BIN_DIR "${WEBRTC_APM_ROOT}/lib")
    set(WEBRTC_APM_SHARED_LIB "${WEBRTC_APM_ROOT}/lib/libwebrtc-audio-processing-3.so.0")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(WEBRTC_APM_BIN_DIR "${WEBRTC_APM_ROOT}/lib")
    set(WEBRTC_APM_SHARED_LIB "${WEBRTC_APM_ROOT}/lib/libwebrtc-audio-processing-3.0.dylib")
    set(WEBRTC_APM_LIBRARY "${WEBRTC_APM_SHARED_LIB}")
endif()

# Without pkg-config the headers need the platform define themselves
if(WIN32)
    set(WEBRTC_APM_DEFINITIONS "WEBRTC_WIN")
else()
    set(WEBRTC_APM_DEFINITIONS "WEBRTC_POSIX")
endif()

# The directory name carries no version, so an old or partial extract would
# otherwise surface later as a confusing compile or link error.
foreach(WEBRTC_APM_REQUIRED_PATH IN ITEMS
        "${WEBRTC_APM_INCLUDE_DIR}/api/audio/builtin_audio_processing_builder.h"
        "${WEBRTC_APM_LIBRARY}"
        "${WEBRTC_APM_SHARED_LIB}")
    if(NOT EXISTS "${WEBRTC_APM_REQUIRED_PATH}")
        message(FATAL_ERROR
            "WebRTC Audio Processing layout under ${WEBRTC_APM_ROOT} does not match "
            "${WEBRTC_APM_VERSION}: missing ${WEBRTC_APM_REQUIRED_PATH}. "
            "Delete ${WEBRTC_APM_ROOT} and re-run configure to download it again.")
    endif()
endforeach()

# The dylib's install name is the absolute path it was built at
if(APPLE)
    execute_process(
        COMMAND install_name_tool -id "@rpath/libwebrtc-audio-processing-3.0.dylib" "${WEBRTC_APM_SHARED_LIB}"
        RESULT_VARIABLE WEBRTC_APM_INSTALL_NAME_RESULT
        ERROR_VARIABLE WEBRTC_APM_INSTALL_NAME_ERROR
    )
    if(NOT WEBRTC_APM_INSTALL_NAME_RESULT EQUAL 0)
        message(WARNING "Failed to normalize install_name for WebRTC APM dylib: ${WEBRTC_APM_INSTALL_NAME_ERROR}")
    endif()
endif()

message(STATUS "WebRTC Audio Processing Configuration:")
message(STATUS "  Version: ${WEBRTC_APM_VERSION}")
message(STATUS "  Root: ${WEBRTC_APM_ROOT}")
message(STATUS "  Arch: ${WEBRTC_ARCH}")
message(STATUS "  Include: ${WEBRTC_APM_INCLUDE_DIR}")
message(STATUS "  Abseil Include: ${WEBRTC_ABSEIL_INCLUDE_DIR}")
message(STATUS "  Library: ${WEBRTC_APM_LIBRARY}")
