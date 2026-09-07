cmake_minimum_required(VERSION 3.16)
project(links LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(VCPKG_APPLOCAL_DEPS OFF)

set(LINKS_APP_ICON_PNG_16 "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/linux/icon_16x16.png")
set(LINKS_APP_ICON_PNG_32 "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/linux/icon_32x32.png")
set(LINKS_APP_ICON_PNG_48 "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/linux/icon_48x48.png")
set(LINKS_APP_ICON_PNG_64 "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/linux/icon_64x64.png")
set(LINKS_APP_ICON_PNG_128 "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/linux/icon_128x128.png")
set(LINKS_APP_ICON_PNG_256 "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/linux/icon_256x256.png")
set(LINKS_MAC_ICONSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/macos/AppIcon.iconset")
set(LINKS_WINDOWS_ICON_ICO "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/windows/icon.ico")

# Testing options
option(LINKS_BUILD_TESTS "Build test suite" ON)
if(WIN32)
    option(LINKS_ENABLE_LOCAL_RECORDING "Enable local recording feature" ON)
else()
    option(LINKS_ENABLE_LOCAL_RECORDING "Enable local recording feature" OFF)
endif()

# =============================================================================
# CMake Module Path
# =============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# =============================================================================
# Qt Configuration (platform-specific)
# =============================================================================
if(WIN32)
    list(PREPEND CMAKE_PREFIX_PATH "D:/Qt/6.10.0/msvc2022_64")
elseif(APPLE)
    # macOS: Adjust this path as needed
    # list(PREPEND CMAKE_PREFIX_PATH "/path/to/Qt/6.x.x/macos")
elseif(UNIX)
    # Linux: Qt is typically found via system paths or set externally
    # list(PREPEND CMAKE_PREFIX_PATH "/path/to/Qt/6.x.x/gcc_64")
endif()

# =============================================================================
# LiveKit SDK (auto-download based on platform)
# =============================================================================
# Set version before including the module (optional, defaults to 0.2.7)
# set(LIVEKIT_SDK_VERSION "0.2.7")
include(FetchLiveKitSDK)

# =============================================================================
# WebRTC Audio Processing (auto-download based on platform)
# =============================================================================
# Set version before including the module (optional, defaults to 2.1.0-mirror.1)
# set(WEBRTC_APM_VERSION "2.1.0-mirror.1")
include(FetchWebRTCAudioProcessing)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Multimedia Quick QuickControls2 Qml Concurrent)
qt_standard_project_setup(REQUIRES 6.8)

# =============================================================================
# links_core_base - Qt-free core
#
# This target deliberately links NO Qt6::* target. Qt headers reach the compiler
# only through Qt6::* usage requirements, so an accidental #include <QString> in
# one of these files is a hard compile error rather than a silent regression.
# AUTOMOC is off for the same reason: a stray Q_OBJECT fails at link time.
#
# Files graduate into this target as they are decoupled from Qt. It currently
# holds only the parts of core/ that were already Qt-free.
# =============================================================================

# Platform libraries for the desktop capture backends. Declared here, above the
# target that needs them; `links` picks them up transitively.
if(APPLE)
    find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(CORE_MEDIA_FRAMEWORK CoreMedia REQUIRED)
    find_library(CORE_VIDEO_FRAMEWORK CoreVideo REQUIRED)
    find_library(AVFOUNDATION_FRAMEWORK AVFoundation REQUIRED)
    find_library(SCREEN_CAPTURE_KIT_FRAMEWORK ScreenCaptureKit)
elseif(UNIX)
    find_package(X11 REQUIRED)
endif()

set(LINKS_CORE_BASE_SOURCES
    core/base/log.cpp
    core/base/strings.cpp
    core/media/video_frame.cpp
    core/platform_window_ops.cpp
    core/thumbnail_service.cpp
    core/desktop_capture/desktop_frame.cpp
    core/desktop_capture/desktop_capturer.cpp
    core/recording/recording_layout.cpp
    core/recording/frame_clock.cpp
)

set(LINKS_CORE_BASE_HEADERS
    core/base/log.h
    core/base/signal.h
    core/base/strings.h
    core/base/executor.h
    core/base/timer.h
    core/base/time.h
    core/media/video_frame.h
    core/image_types.h
    core/window_types.h
    core/platform_window_ops.h
    core/thumbnail_service.h
    core/desktop_capture/capture_options.h
    core/desktop_capture/desktop_geometry.h
    core/desktop_capture/desktop_frame.h
    core/desktop_capture/desktop_capturer.h
    core/recording/recording_layout.h
    core/recording/frame_clock.h
)

if(WIN32)
    list(APPEND LINKS_CORE_BASE_SOURCES
        core/desktop_capture/win/window_utils.cpp
        core/desktop_capture/win/platform_window_ops_win.cpp
        core/desktop_capture/win/wgc_capturer.cpp
        core/desktop_capture/win/dxgi_duplicator.cpp
        core/desktop_capture/win/gdi_capturer.cpp
    )
    list(APPEND LINKS_CORE_BASE_HEADERS
        core/desktop_capture/win/window_utils.h
        core/desktop_capture/win/platform_window_ops_win.h
        core/desktop_capture/win/wgc_capturer.h
        core/desktop_capture/win/dxgi_duplicator.h
        core/desktop_capture/win/gdi_capturer.h
    )
elseif(APPLE)
    list(APPEND LINKS_CORE_BASE_SOURCES
        core/desktop_capture/mac/platform_window_ops_mac.cpp
        core/desktop_capture/mac/screen_capture_kit_adapter.mm
        core/desktop_capture/mac/mac_capturer.cpp
    )
    list(APPEND LINKS_CORE_BASE_HEADERS
        core/desktop_capture/mac/platform_window_ops_mac.h
        core/desktop_capture/mac/screen_capture_kit_adapter.h
        core/desktop_capture/mac/mac_capturer.h
    )
elseif(UNIX)
    list(APPEND LINKS_CORE_BASE_SOURCES
        core/desktop_capture/linux/x11/platform_window_ops_linux_x11.cpp
        core/desktop_capture/linux/x11/x11_capturer.cpp
    )
    list(APPEND LINKS_CORE_BASE_HEADERS
        core/desktop_capture/linux/x11/platform_window_ops_linux_x11.h
        core/desktop_capture/linux/x11/x11_capturer.h
    )
endif()

add_library(links_core_base STATIC
    ${LINKS_CORE_BASE_SOURCES}
    ${LINKS_CORE_BASE_HEADERS}
)

# qt_standard_project_setup() turns CMAKE_AUTOMOC on globally; this target has no
# Q_OBJECT and must never be moc'd.
set_target_properties(links_core_base PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    POSITION_INDEPENDENT_CODE ON
)

target_compile_features(links_core_base PUBLIC cxx_std_20)

# core/screen_capturer.cpp holds a BOM-less CJK string literal. It survives today
# only because QString(const char*) is fromUtf8(); pin UTF-8 narrow literals now,
# before that file moves to std::string and loses the safety net.
if(MSVC)
    target_compile_options(links_core_base PUBLIC /utf-8)
endif()

target_include_directories(links_core_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(links_core_base PUBLIC nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(links_core_base PUBLIC d3d11 dxgi windowsapp dwmapi Shcore)
elseif(APPLE)
    target_link_libraries(links_core_base PUBLIC
        ${CORE_GRAPHICS_FRAMEWORK}
        ${CORE_FOUNDATION_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${CORE_MEDIA_FRAMEWORK}
        ${CORE_VIDEO_FRAMEWORK}
        ${AVFOUNDATION_FRAMEWORK}
    )
    if(SCREEN_CAPTURE_KIT_FRAMEWORK)
        target_link_libraries(links_core_base PUBLIC ${SCREEN_CAPTURE_KIT_FRAMEWORK})
    endif()
elseif(UNIX)
    target_link_libraries(links_core_base PUBLIC X11::X11)
endif()

# Source files
set(SOURCES
    main.cpp
    # Core modules
    core/conference/conference_manager.cpp
    core/conference/network_stats_aggregator.cpp
    core/conference/participant_metadata_parser.cpp
    core/conference/participant_store.cpp
    core/conference/media_pipeline.cpp
    core/conference/device_controller.cpp
    core/conference/room_controller.cpp
    core/camera_capturer.cpp
    core/microphone_capturer.cpp
    core/audio_processing_module.cpp
    core/screen_capturer.cpp
    core/room_event_delegate.cpp
    # Utils
    utils/logger.cpp
    utils/settings.cpp
    # Qt adapters
    ui/adapters/qt/qt_capture_adapter.cpp
    ui/adapters/qt/qt_task_runner.cpp
    ui/adapters/qt/qt_timer_factory.cpp
    ui/adapters/qt/qt_log_sink.cpp
    # QML backends
    ui/backend/network_client.cpp
    ui/backend/LoginBackend.cpp
    ui/backend/SettingsBackend.cpp
    ui/backend/ConferenceBackend.cpp
    ui/backend/VideoRenderer.cpp
    ui/backend/ScreenPickerBackend.cpp
    ui/backend/ShareModeManager.cpp
    ui/backend/AuthBackend.cpp
    ui/backend/ThemeManager.cpp
    ui/backend/AppearanceManager.cpp
    ui/backend/LocalRecordingManager.cpp
)

if(WIN32)
    file(TO_NATIVE_PATH "${LINKS_WINDOWS_ICON_ICO}" LINKS_WINDOWS_ICON_ICO_NATIVE)
    string(REPLACE "\\" "\\\\" LINKS_WINDOWS_ICON_ICO_ESCAPED "${LINKS_WINDOWS_ICON_ICO_NATIVE}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/windows/app_icon.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/links_app_icon.rc"
        @ONLY
    )
    list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/links_app_icon.rc")
endif()

set(HEADERS
    # Core modules
    core/conference/conference_manager.h
    core/conference/conference_types.h
    core/conference/network_stats_aggregator.h
    core/conference/participant_metadata_parser.h
    core/conference/participant_store.h
    core/conference/media_pipeline.h
    core/conference/device_controller.h
    core/conference/room_controller.h
    core/camera_capturer.h
    core/microphone_capturer.h
    core/screen_capturer.h
    core/room_event_delegate.h
    # Utils
    utils/logger.h
    utils/settings.h
    # Qt adapters
    ui/adapters/qt/qt_capture_adapter.h
    ui/adapters/qt/qt_task_runner.h
    ui/adapters/qt/qt_timer_factory.h
    ui/adapters/qt/qt_log_sink.h
    # QML backends
    ui/backend/network_client.h
    ui/backend/LoginBackend.h
    ui/backend/SettingsBackend.h
    ui/backend/ConferenceBackend.h
    ui/backend/VideoRenderer.h
    ui/backend/ScreenPickerBackend.h
    ui/backend/ShareModeManager.h
    ui/backend/AuthBackend.h
    ui/backend/ThemeManager.h
    ui/backend/AppearanceManager.h
    ui/backend/LocalRecordingManager.h
)

# Create executable
qt_add_executable(links ${SOURCES} ${HEADERS})

# Mark Theme.qml as a QML singleton (required for pragma Singleton in Qt 6)
set_source_files_properties(ui/qml/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

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
        ui/qml/components/SecondaryButton.qml
        ui/qml/components/LinkButton.qml
        ui/qml/components/TextField.qml
        ui/qml/components/PillToggle.qml
        ui/qml/components/TabButton.qml
        ui/qml/components/ComboBox.qml
        ui/qml/components/CheckBox.qml
        # Login (auth components)
        ui/qml/login/LoginCard.qml
        ui/qml/login/AuthTabs.qml
        ui/qml/login/AuthField.qml
        ui/qml/login/AuthTextField.qml
        ui/qml/login/LoginForm.qml
        ui/qml/login/RegisterForm.qml
        # Home
        ui/qml/home/HomeWindow.qml
        ui/qml/home/HomeSidebar.qml
        ui/qml/home/HomeUserCard.qml
        ui/qml/home/HomeNavButton.qml
        ui/qml/home/HomeMeetingPage.qml
        ui/qml/home/HomeRecordingPage.qml
        ui/qml/home/QuickActionCard.qml
        ui/qml/home/MeetingListPanel.qml
        ui/qml/home/HostMeetingListPanel.qml
        ui/qml/home/MeetingListItem.qml
        ui/qml/home/GuestPromptDialog.qml
        ui/qml/home/MeetingPasswordDialog.qml
        ui/qml/home/CancelMeetingDialog.qml
        ui/qml/home/AuthModal.qml
        ui/qml/home/ShareScreenForm.qml
        ui/qml/home/LoginPromptCard.qml
        ui/qml/home/JoinForm.qml
        ui/qml/home/QuickStartForm.qml
        ui/qml/home/ScheduleForm.qml
        ui/qml/home/ScheduleCalendar.qml
        ui/qml/home/ScheduleTimeSelect.qml
        ui/qml/home/ScheduleDurationSelect.qml
        # Settings
        ui/qml/settings/SettingsWindow.qml
        ui/qml/settings/AudioSettings.qml
        ui/qml/settings/VideoSettings.qml
        ui/qml/settings/NetworkSettings.qml
        ui/qml/settings/AppearanceSettings.qml
        # Theme
        ui/qml/theme/Theme.qml
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
        ui/qml/utils/DateUtils.js
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
        res/icon/panel-right-open.png
        res/icon/user-x.png
        res/icon/maximize_recovery.png
        res/icon/chevron-down.png
        res/icon/chevron-up.png
        res/icon/chevron-right.png
        res/icon/chevron-left.png 
        res/icon/network.png
        res/icon/screen_sharing_sidebar.png
        res/icon/screen-share-off.png
        res/icon/camera_staring_sidebar.png
        res/icon/copy.png
        res/icon/square-arrow-out-up-right.png
        res/icon/monitor.png
        res/icon/file-chart-column-increasing.png
        res/icon/disc.png
        res/icon/appIcon/linux/icon_16x16.png
        res/icon/appIcon/linux/icon_32x32.png
        res/icon/appIcon/linux/icon_48x48.png
        res/icon/appIcon/linux/icon_64x64.png
        res/icon/appIcon/linux/icon_128x128.png
        res/icon/appIcon/linux/icon_256x256.png
)

# Link libraries
target_link_libraries(links PRIVATE
    links_core_base
    ${LIVEKIT_LIBRARY}
    ${LIVEKIT_FFI_LIBRARY}
    ${WEBRTC_APM_LIBRARY}
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Multimedia
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
)

if(LINKS_ENABLE_LOCAL_RECORDING)
    target_compile_definitions(links PRIVATE LINKS_ENABLE_LOCAL_RECORDING=1)
else()
    target_compile_definitions(links PRIVATE LINKS_ENABLE_LOCAL_RECORDING=0)
endif()
message(STATUS "Local recording enabled: ${LINKS_ENABLE_LOCAL_RECORDING}")

if(APPLE)
    set_target_properties(links PROPERTIES
        MACOSX_BUNDLE TRUE
    )

    find_program(ICONUTIL_EXECUTABLE iconutil)
    if(ICONUTIL_EXECUTABLE)
        file(GLOB LINKS_MAC_ICONSET_FILES CONFIGURE_DEPENDS "${LINKS_MAC_ICONSET_DIR}/*.png")
        set(LINKS_MAC_ICON "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns")

        add_custom_command(
            OUTPUT "${LINKS_MAC_ICON}"
            COMMAND "${ICONUTIL_EXECUTABLE}" -c icns "${LINKS_MAC_ICONSET_DIR}" -o "${LINKS_MAC_ICON}"
            DEPENDS ${LINKS_MAC_ICONSET_FILES}
            COMMENT "Generating macOS application icon"
            VERBATIM
        )

        target_sources(links PRIVATE "${LINKS_MAC_ICON}")
        set_source_files_properties("${LINKS_MAC_ICON}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        set_target_properties(links PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
        )
    else()
        message(WARNING "iconutil not found; macOS bundle icon will not be generated")
    endif()
endif()

if(UNIX AND NOT APPLE)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/res/linux/links.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/links.desktop"
        @ONLY
    )

    install(TARGETS links
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/links.desktop"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    )
    foreach(icon_size IN ITEMS 16 32 48 64 128 256)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/icon/appIcon/linux/icon_${icon_size}x${icon_size}.png"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${icon_size}x${icon_size}/apps"
            RENAME links.png
        )
    endforeach()
endif()

# Include directories
target_include_directories(links PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${LIVEKIT_INCLUDE_DIR}
    ${WEBRTC_APM_INCLUDE_DIR}
    ${WEBRTC_ABSEIL_INCLUDE_DIR}
)

# Set output directory
set_target_properties(links PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Windows specific settings
if(WIN32)
    target_link_libraries(links PRIVATE 
        # Windows system libraries required by livekit_ffi
        # (d3d11/dxgi/windowsapp/dwmapi/Shcore arrive via links_core_base)
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
    add_custom_command(TARGET links POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIVEKIT_BIN_DIR}/livekit.dll"
            "$<TARGET_FILE_DIR:links>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIVEKIT_BIN_DIR}/livekit_ffi.dll"
            "$<TARGET_FILE_DIR:links>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WEBRTC_APM_SHARED_LIB}"
            "$<TARGET_FILE_DIR:links>/"
        COMMENT "Copying DLLs to output directory"
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
