# =============================================================================
# FetchWebRTCAudioProcessing.cmake
# Automatically download and configure WebRTC Audio Processing based on platform
# =============================================================================

# Version configuration (can be overridden before including this module)
if(NOT DEFINED WEBRTC_APM_VERSION)
    set(WEBRTC_APM_VERSION "2.1.0-mirror.1")
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
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WEBRTC_PLATFORM "windows")
    set(WEBRTC_ARCHIVE_EXT "zip")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(WEBRTC_PLATFORM "linux")
    set(WEBRTC_ARCHIVE_EXT "tar.gz")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(WEBRTC_PLATFORM "macos")
    set(WEBRTC_ARCHIVE_EXT "tar.gz")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# =============================================================================
# SDK Paths
# =============================================================================
set(WEBRTC_APM_NAME "webrtc-audio-processing-${WEBRTC_PLATFORM}-${WEBRTC_ARCH}")
set(WEBRTC_APM_ROOT "${CMAKE_SOURCE_DIR}/third_party/${WEBRTC_APM_NAME}")
set(WEBRTC_APM_ARCHIVE "${WEBRTC_APM_NAME}.${WEBRTC_ARCHIVE_EXT}")
set(WEBRTC_APM_URL "https://github.com/Sqhh99/webrtc-audio-processing-mirror/releases/download/v${WEBRTC_APM_VERSION}/${WEBRTC_APM_ARCHIVE}")

# =============================================================================
# Download and Extract SDK if not present
# =============================================================================
if(NOT EXISTS "${WEBRTC_APM_ROOT}")
    message(STATUS "WebRTC Audio Processing not found at ${WEBRTC_APM_ROOT}")
    message(STATUS "Downloading WebRTC Audio Processing v${WEBRTC_APM_VERSION} for ${WEBRTC_PLATFORM}-${WEBRTC_ARCH}...")
    
    set(WEBRTC_DOWNLOAD_PATH "${CMAKE_SOURCE_DIR}/third_party/${WEBRTC_APM_ARCHIVE}")
    
    # Download the archive
    file(DOWNLOAD
        "${WEBRTC_APM_URL}"
        "${WEBRTC_DOWNLOAD_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_ERROR_CODE)
    list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR_MESSAGE)
    
    if(NOT DOWNLOAD_ERROR_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download WebRTC Audio Processing: ${DOWNLOAD_ERROR_MESSAGE}")
    endif()
    
    message(STATUS "Extracting WebRTC Audio Processing...")
    
    # Create target directory first (archive doesn't contain parent folder)
    file(MAKE_DIRECTORY "${WEBRTC_APM_ROOT}")
    
    # Extract the archive directly into the target directory
    file(ARCHIVE_EXTRACT
        INPUT "${WEBRTC_DOWNLOAD_PATH}"
        DESTINATION "${WEBRTC_APM_ROOT}"
    )
    
    # Check if extraction created the expected content
    if(NOT EXISTS "${WEBRTC_APM_ROOT}/include")
        message(FATAL_ERROR "Failed to extract WebRTC Audio Processing to ${WEBRTC_APM_ROOT}")
    endif()
    
    # Clean up the archive
    file(REMOVE "${WEBRTC_DOWNLOAD_PATH}")
    
    message(STATUS "WebRTC Audio Processing v${WEBRTC_APM_VERSION} installed successfully")
else()
    message(STATUS "Found WebRTC Audio Processing at ${WEBRTC_APM_ROOT}")
endif()

# =============================================================================
# Configure SDK paths
# =============================================================================
set(WEBRTC_APM_INCLUDE_DIR "${WEBRTC_APM_ROOT}/include/webrtc-audio-processing-2")
set(WEBRTC_ABSEIL_INCLUDE_DIR "${WEBRTC_APM_ROOT}/include")

# Platform-specific library configuration
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WEBRTC_APM_LIBRARY "${WEBRTC_APM_ROOT}/lib/webrtc-audio-processing-2.lib")
    set(WEBRTC_APM_BIN_DIR "${WEBRTC_APM_ROOT}/bin")
    set(WEBRTC_APM_SHARED_LIB "${WEBRTC_APM_BIN_DIR}/webrtc-audio-processing-2-1.dll")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(WEBRTC_APM_LIBRARY "${WEBRTC_APM_ROOT}/lib/libwebrtc-audio-processing-2.so")
    set(WEBRTC_APM_BIN_DIR "${WEBRTC_APM_ROOT}/lib")
    set(WEBRTC_APM_SHARED_LIB "${WEBRTC_APM_ROOT}/lib/libwebrtc-audio-processing-2.so.1")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(WEBRTC_APM_LIBRARY "${WEBRTC_APM_ROOT}/lib/libwebrtc-audio-processing-2.dylib")
    set(WEBRTC_APM_BIN_DIR "${WEBRTC_APM_ROOT}/lib")
    set(WEBRTC_APM_SHARED_LIB "${WEBRTC_APM_ROOT}/lib/libwebrtc-audio-processing-2.1.dylib")
endif()

message(STATUS "WebRTC Audio Processing Configuration:")
message(STATUS "  Root: ${WEBRTC_APM_ROOT}")
message(STATUS "  Arch: ${WEBRTC_ARCH}")
message(STATUS "  Include: ${WEBRTC_APM_INCLUDE_DIR}")
message(STATUS "  Abseil Include: ${WEBRTC_ABSEIL_INCLUDE_DIR}")
message(STATUS "  Library: ${WEBRTC_APM_LIBRARY}")
