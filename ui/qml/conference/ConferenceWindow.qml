import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Window {
    id: root
    
    width: 1280
    height: 800
    // Hide main window when in Share Mode (overlay enabled)
    visible: !(backend.shareMode && backend.shareMode.isActive && backend.shareMode.overlayEnabled)
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Window
    title: backend.roomName ? "LiveKit Conference - " + backend.roomName : "Conference"
    
    // Connection parameters
    property string serverUrl: ""
    property string token: ""
    property string roomName: ""
    property string userName: ""
    property bool isHost: false
    
    // Backend
    ConferenceBackend {
        id: backend
        
        Component.onCompleted: {
            if (root.serverUrl && root.token) {
                initialize(root.serverUrl, root.token, root.roomName, root.userName, root.isHost)
            }
        }
        
        onLeaveRequested: Qt.callLater(function() { leaveDialog.open() })
        onShowSettings: settingsDialog.open()
        
        // Video frame routing
        onLocalVideoFrameReady: function(frame) {
            // Always route to sidebar for dual-stream mode (when camera shows in sidebar)
            if (videoSidebar) videoSidebar.updateLocalFrame(frame)
            
            // Show camera in main when:
            // - Single stream mode (camera only, no screen share), OR
            // - Dual stream mode with showScreenShareInMain = false (camera in main)
            var hasDualStreams = backend.camEnabled && backend.screenSharing
            var showCameraInMain = !hasDualStreams || !backend.showScreenShareInMain
            
            if (backend.mainParticipantId === "local" && showCameraInMain && mainVideoPanel) {
                mainVideoPanel.updateFrame(frame)
            }
            // Route to gallery view local thumbnail
            // Show camera frames when: showing camera in dual-stream OR single-stream camera only
            if (galleryView.visible && localGalleryThumbnail) {
                var hasDualStreams = backend.camEnabled && backend.screenSharing
                var shouldShowCamera = !hasDualStreams || !localGalleryCard.showingScreen
                if (shouldShowCamera && backend.camEnabled) {
                    localGalleryThumbnail.updateFrame(frame)
                }
            }
        }
        
        onLocalScreenFrameReady: function(frame) {
            // Route to sidebar for dual-stream mode (when screen shows in sidebar)
            if (videoSidebar) videoSidebar.updateLocalScreenFrame(frame)
            
            // Show screen in main when:
            // - mainParticipantId is "local", AND
            // - Single stream mode (screen only, no camera), OR
            // - Dual stream mode with showScreenShareInMain = true (screen in main)
            var hasDualStreams = backend.camEnabled && backend.screenSharing
            var showScreenInMain = !hasDualStreams || backend.showScreenShareInMain
            
            if (backend.mainParticipantId === "local" && backend.screenSharing && showScreenInMain && mainVideoPanel) {
                mainVideoPanel.updateFrame(frame)
            }
            
            // Route to gallery view local thumbnail
            // Show screen frames when: showing screen in dual-stream OR single-stream screen only
            if (galleryView.visible && localGalleryThumbnail) {
                var hasDualStreamsLocal = backend.camEnabled && backend.screenSharing
                var shouldShowScreen = !hasDualStreamsLocal || localGalleryCard.showingScreen
                if (shouldShowScreen && backend.screenSharing) {
                    localGalleryThumbnail.updateFrame(frame)
                }
            }
        }
        
        onRemoteVideoFrameReady: function(participantId, frame) {
            // Always route to sidebar for dual-stream mode (camera in sidebar when screen in main)
            if (videoSidebar) videoSidebar.updateRemoteFrame(participantId, frame)
            
            // Show camera in main when:
            // - Single stream mode (camera only, no screen share), OR
            // - Dual stream mode with showScreenInMain = false (camera in main)
            var hasScreenShare = backend.getRemoteScreenSharing(participantId)
            var hasCam = true  // If we're receiving camera frames, camera is enabled
            var hasDualStreams = hasCam && hasScreenShare
            var showCameraInMain = !hasDualStreams || !backend.getRemoteShowScreenInMain(participantId)
            
            if (backend.mainParticipantId === participantId && showCameraInMain && mainVideoPanel) {
                mainVideoPanel.updateFrame(frame)
            }
            // Also route to gallery view remote thumbnails (camera frames)
            if (galleryView.visible) {
                updateGalleryRemoteFrame(participantId, frame, false)
            }
        }
        
        onRemoteScreenFrameReady: function(participantId, frame) {
            // Route to sidebar for dual-stream mode (screen in sidebar when camera in main)
            if (videoSidebar) videoSidebar.updateRemoteScreenFrame(participantId, frame)
            
            // Show screen in main when:
            // - Single stream mode (screen only, no camera), OR
            // - Dual stream mode with showScreenInMain = true (screen in main)
            var hasCam = true  // Assume camera is enabled if participant has known cam state
            var hasDualStreams = hasCam && true  // Screen share is always enabled if receiving frames
            var showScreenInMain = !hasDualStreams || backend.getRemoteShowScreenInMain(participantId)
            
            if (backend.mainParticipantId === participantId && showScreenInMain && mainVideoPanel) {
                mainVideoPanel.updateFrame(frame)
            }
            
            // Also route to gallery view remote thumbnails (screen frames)
            if (galleryView.visible) {
                updateGalleryRemoteFrame(participantId, frame, true)
            }
        }
        
        onFullscreenChanged: {
            if (backend.isFullscreen) root.showFullScreen()
            else root.showNormal()
        }
        
        onAlwaysOnTopChanged: {
            root.flags = backend.alwaysOnTop 
                ? (Qt.FramelessWindowHint | Qt.Window | Qt.WindowStaysOnTopHint)
                : (Qt.FramelessWindowHint | Qt.Window)
            root.show()
        }
        
        // Handle local camera ended - clear frames
        onLocalCameraEnded: {
            if (localGalleryThumbnail && typeof localGalleryThumbnail.clearFrame === 'function') {
                localGalleryThumbnail.clearFrame()
            }
            if (backend.mainParticipantId === "local" && mainVideoPanel) {
                mainVideoPanel.clearFrame()
            }
        }
        
        // Handle local screen share ended - clear frames
        onLocalScreenShareEnded: {
            if (backend.mainParticipantId === "local" && mainVideoPanel) {
                mainVideoPanel.clearFrame()
            }
        }
        
        // Handle remote track ended - clear frames
        onRemoteTrackEnded: function(participantId, isScreenShare) {
            // Clear main panel if this participant is displayed
            if (backend.mainParticipantId === participantId && mainVideoPanel) {
                mainVideoPanel.clearFrame()
            }
            // Clear gallery view thumbnails
            clearGalleryRemoteFrame(participantId)
        }
    }
    
    // Dynamic grid column calculation
    function getGridColumns(count) {
        if (count <= 2) return 2
        if (count <= 4) return 2
        if (count <= 9) return 3
        if (count <= 16) return 4
        return 5
    }
    
    // Gallery view remote frame routing
    function updateGalleryRemoteFrame(participantId, frame, isScreenFrame) {
        for (var i = 0; i < galleryRemoteRepeater.count; i++) {
            var item = galleryRemoteRepeater.itemAt(i)
            if (item && item.children) {
                // Find the VideoThumbnail in the Rectangle's children
                for (var j = 0; j < item.children.length; j++) {
                    var child = item.children[j]
                    if (child.participantId && child.participantId === participantId) {
                        // Check if this card should receive this frame type
                        var hasDualStreams = item.hasDualStreams || false
                        var showingScreen = item.showingScreen || false
                        
                        // Determine if we should route this frame
                        var shouldRoute = false
                        if (hasDualStreams) {
                            // In dual-stream mode, route based on which stream is being shown
                            shouldRoute = (isScreenFrame === showingScreen)
                        } else {
                            // In single-stream mode, route if this is the only active stream type
                            shouldRoute = true
                        }
                        
                        if (shouldRoute) {
                            child.updateFrame(frame)
                        }
                        return
                    }
                }
            }
        }
    }
    
    // Avatar color palette
    function getAvatarColor(index) {
        var colors = ["#3B82F6", "#10B981", "#8B5CF6", "#F59E0B", "#EC4899", "#06B6D4", "#EF4444", "#6366F1"]
        return colors[index % colors.length]
    }
    
    // Clear gallery view remote frame when track ends
    function clearGalleryRemoteFrame(participantId) {
        for (var i = 0; i < galleryRemoteRepeater.count; i++) {
            var item = galleryRemoteRepeater.itemAt(i)
            if (item && item.children) {
                for (var j = 0; j < item.children.length; j++) {
                    var child = item.children[j]
                    if (child.participantId && child.participantId === participantId) {
                        if (typeof child.clearFrame === 'function') {
                            child.clearFrame()
                        }
                        return
                    }
                }
            }
        }
    }
    
    // Main content
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        anchors.margins: 1
        color: "#F5F7FA"
        radius: 16
        border.color: "#E5E7EB"
        border.width: 1
        clip: true
        antialiasing: true
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            
            // Title bar
            ConferenceTitleBar {
                id: titleBar
                Layout.fillWidth: true
                targetWindow: root
                backend: backend
            }
            
            // Main content area
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 12
                
                // Speaker View
                RowLayout {
                    id: speakerView
                    anchors.fill: parent
                    spacing: 12
                    visible: backend.viewMode !== "gallery"
                    
                    // Left sidebar (collapsible)
                    VideoSidebar {
                        id: videoSidebar
                        Layout.fillHeight: true
                        Layout.preferredWidth: backend.sidebarVisible ? 224 : 0
                        visible: backend.sidebarVisible
                        opacity: backend.sidebarVisible ? 1 : 0
                        backend: backend
                        
                        onThumbnailClicked: function(participantId) {
                            backend.pinParticipant(participantId)
                        }
                        
                        Behavior on Layout.preferredWidth {
                            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
                        }
                        Behavior on opacity {
                            NumberAnimation { duration: 200 }
                        }
                    }
                    
                    // Main video panel
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#F9FAFB"
                        radius: 16
                        border.color: "#E5E7EB"
                        clip: true
                        
                        MainVideoPanel {
                            id: mainVideoPanel
                            anchors.fill: parent
                            backend: backend
                        }
                        
                        // Sidebar toggle button
                        Rectangle {
                            id: sidebarToggle
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.leftMargin: 12
                            anchors.topMargin: 12
                            width: 36
                            height: 36
                            radius: 8
                            color: sidebarToggleArea.containsMouse ? "#00000080" : "#00000040"
                            opacity: sidebarToggleArea.containsMouse || !backend.sidebarVisible ? 1 : 0
                            
                            Behavior on opacity { NumberAnimation { duration: 150 } }
                            
                            Image {
                                anchors.centerIn: parent
                                source: backend.sidebarVisible ? "qrc:/res/icon/panel-left-close.png" : "qrc:/res/icon/panel-left-open.png"
                                sourceSize.width: 18
                                sourceSize.height: 18
                            }
                            
                            MouseArea {
                                id: sidebarToggleArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.sidebarVisible = !backend.sidebarVisible
                            }
                            
                            ToolTip.visible: sidebarToggleArea.containsMouse
                            ToolTip.text: backend.sidebarVisible ? "收起左侧列表" : "展开左侧列表"
                            ToolTip.delay: 500
                        }
                    }
                }
                
                // Gallery View - Dynamic Grid
                Rectangle {
                    id: galleryView
                    anchors.fill: parent
                    color: "#FFFFFF"
                    radius: 16
                    border.color: "#E5E7EB"
                    visible: backend.viewMode === "gallery"
                    clip: true
                    
                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 16
                        contentWidth: availableWidth
                        
                        GridLayout {
                            id: galleryGrid
                            width: parent.width
                            columns: getGridColumns(backend.participants.length + 1)
                            columnSpacing: 12
                            rowSpacing: 12
                            
                            // Local participant card with actual video
                            Rectangle {
                                id: localGalleryCard
                                Layout.fillWidth: true
                                Layout.preferredHeight: width * 9 / 16 // 16:9 aspect ratio
                                color: "#FFFFFF"
                                radius: 12
                                border.color: "#E5E7EB"
                                border.width: 1
                                clip: true
                                
                                // Dual-stream state
                                property bool showingScreen: false
                                property bool hasDualStreams: backend.camEnabled && backend.screenSharing
                                
                                // Video thumbnail for local camera/screen
                                VideoThumbnail {
                                    id: localGalleryThumbnail
                                    anchors.fill: parent
                                    participantId: "local"
                                    participantName: backend.userName + " (You)"
                                    micEnabled: backend.micEnabled
                                    camEnabled: localGalleryCard.showingScreen ? true : (backend.camEnabled || backend.screenSharing)
                                    mirrored: !localGalleryCard.showingScreen
                                    showStatus: false // We use custom name label below
                                }
                                
                                // Left chevron button (switch to camera)
                                Rectangle {
                                    id: localLeftChevron
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 8
                                    width: 28
                                    height: 28
                                    radius: 14
                                    color: localLeftArea.containsMouse ? "#00000080" : "#00000050"
                                    border.color: "#6010B981"
                                    border.width: 1
                                    visible: localGalleryCard.hasDualStreams && localGalleryCard.showingScreen
                                    z: 20
                                    
                                    Image {
                                        anchors.centerIn: parent
                                        source: "qrc:/res/icon/chevron-left.png"
                                        sourceSize.width: 14
                                        sourceSize.height: 14
                                    }
                                    
                                    MouseArea {
                                        id: localLeftArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: localGalleryCard.showingScreen = false
                                    }
                                }
                                
                                // Right chevron button (switch to screen)
                                Rectangle {
                                    id: localRightChevron
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.rightMargin: 8
                                    width: 28
                                    height: 28
                                    radius: 14
                                    color: localRightArea.containsMouse ? "#00000080" : "#00000050"
                                    border.color: "#6010B981"
                                    border.width: 1
                                    visible: localGalleryCard.hasDualStreams && !localGalleryCard.showingScreen
                                    z: 20
                                    
                                    Image {
                                        anchors.centerIn: parent
                                        source: "qrc:/res/icon/chevron-right.png"
                                        sourceSize.width: 14
                                        sourceSize.height: 14
                                    }
                                    
                                    MouseArea {
                                        id: localRightArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: localGalleryCard.showingScreen = true
                                    }
                                }
                                
                                // Name label with mic status (bottom-left, semi-transparent)
                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    anchors.margins: 8
                                    height: 24
                                    width: localNameRow.width + 16
                                    color: "#FFFFFFEE"
                                    radius: 6
                                    z: 10
                                    
                                    Row {
                                        id: localNameRow
                                        anchors.centerIn: parent
                                        spacing: 4
                                        
                                        // Mic status indicator only
                                        Rectangle {
                                            width: backend.micEnabled ? 4 : 10
                                            height: backend.micEnabled ? 8 : 10
                                            radius: backend.micEnabled ? 2 : 5
                                            color: backend.micEnabled ? "#10B981" : "transparent"
                                            anchors.verticalCenter: parent.verticalCenter
                                            
                                            Image {
                                                anchors.centerIn: parent
                                                source: "qrc:/res/icon/mute_the_microphone.png"
                                                sourceSize.width: 10
                                                sourceSize.height: 10
                                                visible: !backend.micEnabled
                                            }
                                        }
                                        
                                        Text {
                                            text: localGalleryCard.showingScreen ? "我 (屏幕)" : "我 (You)"
                                            color: "#374151"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                        }
                                    }
                                }
                                
                                // Click to pin
                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: backend.pinParticipant("local")
                                    z: 1
                                }
                            }
                            
                            // Remote participants
                            Repeater {
                                id: galleryRemoteRepeater
                                model: backend.participants.filter(function(p) { return p.identity !== "local" })
                                
                                Rectangle {
                                    id: remoteCard
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: width * 9 / 16 // 16:9 aspect ratio
                                    color: "#FFFFFF"
                                    radius: 12
                                    // Speaker highlight with blue border
                                    border.color: modelData.identity === backend.mainParticipantId ? "#3B82F6" : "#E5E7EB"
                                    border.width: modelData.identity === backend.mainParticipantId ? 2 : 1
                                    clip: true
                                    
                                    // Dual-stream state
                                    property bool showingScreen: false
                                    property bool hasDualStreams: modelData.camEnabled && modelData.screenSharing
                                    
                                    // Video thumbnail for remote participant
                                    VideoThumbnail {
                                        id: remoteGalleryThumbnail
                                        anchors.fill: parent
                                        participantId: modelData.identity
                                        participantName: modelData.name || modelData.identity
                                        micEnabled: modelData.micEnabled
                                        camEnabled: remoteCard.showingScreen ? true : (modelData.camEnabled || modelData.screenSharing)
                                        mirrored: false
                                        showStatus: false // We use custom name label below
                                    }
                                    
                                    // Left chevron button (switch to camera)
                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: 8
                                        width: 28
                                        height: 28
                                        radius: 14
                                        color: remoteLeftArea.containsMouse ? "#00000080" : "#00000050"
                                        border.color: "#6010B981"
                                        border.width: 1
                                        visible: remoteCard.hasDualStreams && remoteCard.showingScreen
                                        z: 20
                                        
                                        Image {
                                            anchors.centerIn: parent
                                            source: "qrc:/res/icon/chevron-left.png"
                                            sourceSize.width: 14
                                            sourceSize.height: 14
                                        }
                                        
                                        MouseArea {
                                            id: remoteLeftArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: remoteCard.showingScreen = false
                                        }
                                    }
                                    
                                    // Right chevron button (switch to screen)
                                    Rectangle {
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.rightMargin: 8
                                        width: 28
                                        height: 28
                                        radius: 14
                                        color: remoteRightArea.containsMouse ? "#00000080" : "#00000050"
                                        border.color: "#6010B981"
                                        border.width: 1
                                        visible: remoteCard.hasDualStreams && !remoteCard.showingScreen
                                        z: 20
                                        
                                        Image {
                                            anchors.centerIn: parent
                                            source: "qrc:/res/icon/chevron-right.png"
                                            sourceSize.width: 14
                                            sourceSize.height: 14
                                        }
                                        
                                        MouseArea {
                                            id: remoteRightArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: remoteCard.showingScreen = true
                                        }
                                    }
                                    
                                    // Speaker ring effect
                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 12
                                        color: "transparent"
                                        border.color: "#3B82F620"
                                        border.width: 4
                                        visible: modelData.identity === backend.mainParticipantId
                                        z: 5
                                    }
                                    
                                    // Name label with mic status (bottom-left)
                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.bottom: parent.bottom
                                        anchors.margins: 8
                                        height: 24
                                        width: remoteNameRow.width + 16
                                        color: "#FFFFFFEE"
                                        radius: 6
                                        z: 10
                                        
                                        Row {
                                            id: remoteNameRow
                                            anchors.centerIn: parent
                                            spacing: 4
                                            
                                            // Mic status indicator only
                                            Rectangle {
                                                width: modelData.micEnabled ? 4 : 10
                                                height: modelData.micEnabled ? 8 : 10
                                                radius: modelData.micEnabled ? 2 : 5
                                                color: modelData.micEnabled ? "#10B981" : "transparent"
                                                anchors.verticalCenter: parent.verticalCenter
                                                
                                                Image {
                                                    anchors.centerIn: parent
                                                    source: "qrc:/res/icon/mute_the_microphone.png"
                                                    sourceSize.width: 10
                                                    sourceSize.height: 10
                                                    visible: !modelData.micEnabled
                                                }
                                            }
                                            
                                            Text {
                                                text: remoteCard.showingScreen ? (modelData.name || modelData.identity) + " (屏幕)" : (modelData.name || modelData.identity)
                                                color: "#374151"
                                                font.pixelSize: 11
                                                font.weight: Font.Bold
                                            }
                                        }
                                    }
                                    
                                    // Click to pin
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: backend.pinParticipant(modelData.identity)
                                        z: 1
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Fixed bottom control bar
            ControlBar {
                id: controlBar
                Layout.fillWidth: true
                backend: backend
                settingsBackend: settingsBackendInstance
                
                onScreenShareClicked: screenPickerDialog.open()
            }
        }
        
        // SettingsBackend for device selection in ControlBar
        SettingsBackend {
            id: settingsBackendInstance
            Component.onCompleted: {
                loadSettings()
                refreshDevices()
            }
        }
        
        // Screen picker dialog
        ScreenPickerDialog {
            id: screenPickerDialog
            onScreenSelected: function(screenIndex) { backend.startScreenShare(screenIndex) }
            onWindowSelected: function(windowId) { backend.startWindowShare(windowId) }
        }
        
        // Right sidebar
        RightSidebar {
            id: rightSidebar
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 52
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 68
            width: 320
            visible: backend.isChatVisible || backend.isParticipantsVisible
            backend: backend
            z: 10
        }
        
        // Settings dialog
        SettingsWindow {
            id: settingsDialog
        }
    }
    
    // Leave dialog component
    LeaveDialog {
        id: leaveDialog
        anchors.centerIn: parent
        onConfirmClicked: {
            backend.confirmLeave()
            root.close()
        }
    }
    
    // ==========================================
    // Share Mode: Floating overlay windows
    // ==========================================
    
    // Floating Control Bar (always-on-top, capture-excluded)
    Loader {
        id: floatingBarLoader
        active: backend.shareMode && backend.shareMode.isActive && backend.shareMode.overlayEnabled
        sourceComponent: FloatingControlBar {
            backend: root.conferenceBackend
            settingsBackend: settingsBackendInstance
            visible: true
            
            onStopSharingClicked: {
                root.conferenceBackend.stopScreenShare()
            }
        }
    }

    // Camera Thumbnail (when overlay is disabled, keep it in the main window)
    Loader {
        id: cameraThumbnailLoader
        active: backend.shareMode && backend.shareMode.isActive &&
                !backend.shareMode.overlayEnabled && backend.camEnabled
        sourceComponent: CameraThumbnail {
            backend: root.conferenceBackend
            visible: true
        }
    }
    
    // Alias for easier access in loader components
    property alias conferenceBackend: backend
}
