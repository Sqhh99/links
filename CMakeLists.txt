cmake_minimum_required(VERSION 3.16)
project(links LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(VCPKG_APPLOCAL_DEPS OFF)

# Testing options
option(LINKS_BUILD_TESTS "Build test suite" ON)

# Set Qt path (keep existing prefixes for vcpkg)
list(PREPEND CMAKE_PREFIX_PATH "D:/Qt/6.10.0/msvc2022_64")
set(LIVEKIT_SDK_ROOT "D:/workspace/cpp-workspace/links/extern/livekit-sdk-windows")
set(LIVEKIT_PATH "${LIVEKIT_SDK_ROOT}/lib")
# Set LiveKit SDK paths (external library)
# livekit.lib is the import library for livekit.dll
set(LIVEKIT_INCLUDE_DIR "${LIVEKIT_SDK_ROOT}/include")
set(LIVEKIT_LIBRARY "${LIVEKIT_PATH}/livekit.lib")
set(LIVEKIT_FFI_LIBRARY "${LIVEKIT_PATH}/livekit_ffi.dll.lib")

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Multimedia Quick QuickControls2 Qml Concurrent)

# Source files
set(SOURCES
    main.cpp
    # Core modules
    core/conference/conference_manager.cpp
    core/conference/participant_store.cpp
    core/conference/media_pipeline.cpp
    core/conference/device_controller.cpp
    core/conference/room_controller.cpp
    core/media_manager.cpp
    core/network_client.cpp
    core/camera_capturer.cpp
    core/microphone_capturer.cpp
    core/screen_capturer.cpp
    core/room_event_delegate.cpp
    core/platform_window_ops.cpp
    core/thumbnail_service.cpp
    # Desktop Capture module
    core/desktop_capture/desktop_frame.cpp
    core/desktop_capture/desktop_capturer.cpp
    # Utils
    utils/logger.cpp
    utils/settings.cpp
    # QML backends
    ui/backend/LoginBackend.cpp
    ui/backend/SettingsBackend.cpp
    ui/backend/ConferenceBackend.cpp
    ui/backend/VideoRenderer.cpp
    ui/backend/ScreenPickerBackend.cpp
    ui/backend/ShareModeManager.cpp
)

# Windows-specific desktop capture sources
if(WIN32)
    list(APPEND SOURCES
        core/desktop_capture/win/window_utils.cpp
        core/desktop_capture/win/platform_window_ops_win.cpp
        core/desktop_capture/win/wgc_capturer.cpp
        core/desktop_capture/win/dxgi_duplicator.cpp
        core/desktop_capture/win/gdi_capturer.cpp
    )
endif()

set(HEADERS
    # Core modules
    core/conference/conference_manager.h
    core/conference/conference_types.h
    core/conference/participant_store.h
    core/conference/media_pipeline.h
    core/conference/device_controller.h
    core/conference/room_controller.h
    core/media_manager.h
    core/network_client.h
    core/camera_capturer.h
    core/microphone_capturer.h
    core/screen_capturer.h
    core/room_event_delegate.h
    core/window_types.h
    core/platform_window_ops.h
    core/thumbnail_service.h
    # Utils
    utils/logger.h
    utils/settings.h
    # QML backends
    ui/backend/LoginBackend.h
    ui/backend/SettingsBackend.h
    ui/backend/ConferenceBackend.h
    ui/backend/VideoRenderer.h
    ui/backend/ScreenPickerBackend.h
    ui/backend/ShareModeManager.h
)

# Create executable
qt_add_executable(links ${SOURCES} ${HEADERS})

# Add QML module with all QML files and resources
qt_add_qml_module(links
    URI Links
    VERSION 1.0
    RESOURCE_PREFIX /
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        # Main entry
        ui/qml/main.qml
        # Components
        ui/qml/components/TitleBar.qml
        ui/qml/components/IconButton.qml
        ui/qml/components/PrimaryButton.qml
        ui/qml/components/TextField.qml
        ui/qml/components/PillToggle.qml
        ui/qml/components/TabButton.qml
        ui/qml/components/ComboBox.qml
        ui/qml/components/CheckBox.qml
        # Login
        ui/qml/login/LoginWindow.qml
        ui/qml/login/HeroPanel.qml
        ui/qml/login/LoginCard.qml
        ui/qml/login/JoinForm.qml
        ui/qml/login/QuickStartForm.qml
        ui/qml/login/ScheduleForm.qml
        # Settings
        ui/qml/settings/SettingsWindow.qml
        ui/qml/settings/AudioSettings.qml
        ui/qml/settings/VideoSettings.qml
        ui/qml/settings/NetworkSettings.qml
        # Conference
        ui/qml/conference/ConferenceWindow.qml
        ui/qml/conference/ConferenceTitleBar.qml
        ui/qml/conference/VideoSidebar.qml
        ui/qml/conference/VideoThumbnail.qml
        ui/qml/conference/MainVideoPanel.qml
        ui/qml/conference/ControlBar.qml
        ui/qml/conference/RightSidebar.qml
        ui/qml/conference/ParticipantItem.qml
        ui/qml/conference/ChatPanel.qml
        ui/qml/conference/ChatMessage.qml
        ui/qml/conference/LeaveDialog.qml
        ui/qml/conference/FloatingControlBar.qml
        ui/qml/conference/CameraThumbnail.qml
        # Screen Picker
        ui/qml/screenpicker/ScreenPickerDialog.qml
        ui/qml/screenpicker/ScreenPickerTitleBar.qml
        ui/qml/screenpicker/ScreenPickerTabBar.qml
        ui/qml/screenpicker/ScreenGrid.qml
        ui/qml/screenpicker/WindowGrid.qml
        ui/qml/screenpicker/ThumbnailItem.qml
        ui/qml/screenpicker/GhostButton.qml
        ui/qml/screenpicker/ShareButton.qml
    RESOURCES
        res/icon/close.png
        res/icon/minimize.png
        res/icon/maximize.png
        res/icon/video.png
        res/icon/close_video.png
        res/icon/mute_the_microphone.png
        res/icon/Turn_on_the_microphone.png
        res/icon/monitor-up.png
        res/icon/message.png
        res/icon/user.png
        res/icon/hang_up.png
        res/icon/pin-off.png
        res/icon/pin.png
        res/icon/set_up.png
        res/icon/panel-left-close.png
        res/icon/panel-left-open.png 
        res/icon/user-x.png
        res/icon/maximize_recovery.png
        res/icon/chevron-down.png
        res/icon/chevron-up.png
        res/icon/network.png
        res/icon/screen_sharing_sidebar.png
        res/icon/screen-share-off.png
        res/icon/camera_staring_sidebar.png
)

# Link libraries
target_link_libraries(links PRIVATE
    ${LIVEKIT_LIBRARY}
    ${LIVEKIT_FFI_LIBRARY}
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Multimedia
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
)

# Include directories
target_include_directories(links PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${LIVEKIT_INCLUDE_DIR}
)

# Set output directory
set_target_properties(links PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Windows specific settings
if(WIN32)
    target_link_libraries(links PRIVATE 
        d3d11 dxgi windowsapp
        dwmapi Shcore
        # Windows system libraries required by livekit_ffi
        ntdll            # NT kernel functions (NtReadFile, NtWriteFile, etc.)
        winmm            # Multimedia timer functions (timeSetEvent, timeKillEvent)
        msdmo            # DirectX Media Objects
        dmoguids         # DMO GUIDs and interfaces
        strmiids         # DirectShow GUIDs
        wmcodecdspuuid   # Windows Media Codec DSP UUIDs (CLSID_CWMAudioAEC)
    )
    set_target_properties(links PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    
    # Copy DLLs to output directory after build
    set(DLL_SOURCE_DIR "${LIVEKIT_PATH}")
    add_custom_command(TARGET links POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_SOURCE_DIR}/livekit.dll"
            "$<TARGET_FILE_DIR:links>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_SOURCE_DIR}/livekit_ffi.dll"
            "$<TARGET_FILE_DIR:links>/"
        COMMENT "Copying DLLs from client-sdk-cpp to output directory"
    )
endif()

# =============================================================================
# Testing Configuration
# =============================================================================
if(LINKS_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()
