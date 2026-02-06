# =============================================================================
# FetchLiveKitSDK.cmake
# Automatically download and configure LiveKit SDK based on platform
# =============================================================================

# Version configuration (can be overridden before including this module)
if(NOT DEFINED LIVEKIT_SDK_VERSION)
    set(LIVEKIT_SDK_VERSION "0.2.7")
endif()

# Shared SDK architecture selector across third-party fetch modules.
# Supported values: x64, arm64. Default is x64.
if(NOT DEFINED LINKS_SDK_ARCH)
    set(LINKS_SDK_ARCH "x64")
endif()
string(TOLOWER "${LINKS_SDK_ARCH}" LIVEKIT_ARCH)
if(NOT LIVEKIT_ARCH MATCHES "^(x64|arm64)$")
    message(FATAL_ERROR "Unsupported LINKS_SDK_ARCH: ${LINKS_SDK_ARCH}. Expected x64 or arm64.")
endif()

# =============================================================================
# Platform Detection
# =============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(LIVEKIT_PLATFORM "windows")
    set(LIVEKIT_ARCHIVE_EXT "zip")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LIVEKIT_PLATFORM "linux")
    set(LIVEKIT_ARCHIVE_EXT "tar.gz")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(LIVEKIT_PLATFORM "macos")
    set(LIVEKIT_ARCHIVE_EXT "tar.gz")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# =============================================================================
# SDK Paths
# =============================================================================
set(LIVEKIT_SDK_NAME "livekit-sdk-${LIVEKIT_PLATFORM}-${LIVEKIT_ARCH}-${LIVEKIT_SDK_VERSION}")
set(LIVEKIT_SDK_ROOT "${CMAKE_SOURCE_DIR}/third_party/${LIVEKIT_SDK_NAME}")
set(LIVEKIT_SDK_ARCHIVE "${LIVEKIT_SDK_NAME}.${LIVEKIT_ARCHIVE_EXT}")
set(LIVEKIT_SDK_URL "https://github.com/livekit/client-sdk-cpp/releases/download/v${LIVEKIT_SDK_VERSION}/${LIVEKIT_SDK_ARCHIVE}")

# =============================================================================
# Download and Extract SDK if not present
# =============================================================================
if(NOT EXISTS "${LIVEKIT_SDK_ROOT}")
    message(STATUS "LiveKit SDK not found at ${LIVEKIT_SDK_ROOT}")
    message(STATUS "Downloading LiveKit SDK v${LIVEKIT_SDK_VERSION} for ${LIVEKIT_PLATFORM}-${LIVEKIT_ARCH}...")
    
    set(LIVEKIT_DOWNLOAD_PATH "${CMAKE_SOURCE_DIR}/third_party/${LIVEKIT_SDK_ARCHIVE}")
    
    # Download the archive
    file(DOWNLOAD
        "${LIVEKIT_SDK_URL}"
        "${LIVEKIT_DOWNLOAD_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_ERROR_CODE)
    list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR_MESSAGE)
    
    if(NOT DOWNLOAD_ERROR_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download LiveKit SDK: ${DOWNLOAD_ERROR_MESSAGE}")
    endif()
    
    message(STATUS "Extracting LiveKit SDK...")
    
    # Extract the archive
    file(ARCHIVE_EXTRACT
        INPUT "${LIVEKIT_DOWNLOAD_PATH}"
        DESTINATION "${CMAKE_SOURCE_DIR}/third_party"
    )
    
    # The archive extracts to a subdirectory with the same name
    # Check if extraction created the expected directory
    if(NOT EXISTS "${LIVEKIT_SDK_ROOT}")
        message(FATAL_ERROR "Failed to extract LiveKit SDK to ${LIVEKIT_SDK_ROOT}")
    endif()
    
    # Clean up the archive
    file(REMOVE "${LIVEKIT_DOWNLOAD_PATH}")
    
    message(STATUS "LiveKit SDK v${LIVEKIT_SDK_VERSION} installed successfully")
else()
    message(STATUS "Found LiveKit SDK at ${LIVEKIT_SDK_ROOT}")
endif()

# =============================================================================
# Configure SDK paths for find_package
# =============================================================================
# Add SDK's CMake config directory to CMAKE_PREFIX_PATH
list(APPEND CMAKE_PREFIX_PATH "${LIVEKIT_SDK_ROOT}/lib/cmake/LiveKit")

# Export variables for manual configuration (if find_package doesn't work)
set(LIVEKIT_INCLUDE_DIR "${LIVEKIT_SDK_ROOT}/include")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(LIVEKIT_LIBRARY "${LIVEKIT_SDK_ROOT}/lib/livekit.lib")
    set(LIVEKIT_FFI_LIBRARY "${LIVEKIT_SDK_ROOT}/lib/livekit_ffi.dll.lib")
    set(LIVEKIT_BIN_DIR "${LIVEKIT_SDK_ROOT}/bin")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LIVEKIT_LIBRARY "${LIVEKIT_SDK_ROOT}/lib/liblivekit.so")
    set(LIVEKIT_FFI_LIBRARY "${LIVEKIT_SDK_ROOT}/lib/liblivekit_ffi.so")
    set(LIVEKIT_BIN_DIR "${LIVEKIT_SDK_ROOT}/lib")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(LIVEKIT_LIBRARY "${LIVEKIT_SDK_ROOT}/lib/liblivekit.dylib")
    set(LIVEKIT_FFI_LIBRARY "${LIVEKIT_SDK_ROOT}/lib/liblivekit_ffi.dylib")
    set(LIVEKIT_BIN_DIR "${LIVEKIT_SDK_ROOT}/lib")
endif()

message(STATUS "LiveKit SDK Configuration:")
message(STATUS "  Root: ${LIVEKIT_SDK_ROOT}")
message(STATUS "  Arch: ${LIVEKIT_ARCH}")
message(STATUS "  Include: ${LIVEKIT_INCLUDE_DIR}")
message(STATUS "  Library: ${LIVEKIT_LIBRARY}")
message(STATUS "  FFI Library: ${LIVEKIT_FFI_LIBRARY}")
