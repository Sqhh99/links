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
    property string meetingNo: ""
    property string userName: ""
    property string userAuthToken: ""
    property bool isHost: false
    property bool isGuest: false
    property bool chromeAutoHideEnabled: AppearanceManager.autoHideConferenceChrome
    property bool topChromeVisible: true
    property bool bottomChromeVisible: true
    property int chromeAutoHideDelayMs: 3000
    property int topRevealZoneHeight: 48
    property int bottomRevealZoneHeight: 72
    property int topChromeHeight: 52
    property int bottomChromeHeight: 68

    // Backend
    ConferenceBackend {
        id: backend

        Component.onCompleted: {
            setConferenceWindow(root)
            if (root.serverUrl && root.token) {
                initialize(root.serverUrl, root.token, root.roomName, root.meetingNo, root.userName, root.isHost, root.userAuthToken)
            }
        }

        onLeaveRequested: Qt.callLater(function() { leaveDialog.open() })
        onShowSettings: settingsDialog.open()
        onMeetingEndedByHost: {
            leaveDialog.close()
            meetingEndedPopup.open()
            meetingEndedAutoCloseTimer.restart()
        }

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
            if (localGalleryThumbnail && typeof localGalleryThumbnail.clearFrame === 'function') {
                localGalleryThumbnail.clearFrame()
            }
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

    function restartChromeAutoHideTimer() {
        if (!chromeAutoHideEnabled || !visible) {
            return
        }
        chromeAutoHideTimer.restart()
    }

    function resetChromeVisibility() {
        topChromeVisible = true
        bottomChromeVisible = true
        restartChromeAutoHideTimer()
    }

    function maybeRevealChromeByPointer(pointerY) {
        if (!chromeAutoHideEnabled || !visible) {
            return
        }

        var nearTop = pointerY <= topRevealZoneHeight
        var nearBottom = pointerY >= (windowFrame.height - bottomRevealZoneHeight)
        var didReveal = false

        if (!topChromeVisible && nearTop) {
            topChromeVisible = true
            didReveal = true
        }
        if (!bottomChromeVisible && nearBottom) {
            bottomChromeVisible = true
            didReveal = true
        }

        if (didReveal || topChromeVisible || bottomChromeVisible) {
            restartChromeAutoHideTimer()
        }
    }

    Timer {
        id: chromeAutoHideTimer
        interval: root.chromeAutoHideDelayMs
        repeat: false
        running: false

        onTriggered: {
            if (!root.chromeAutoHideEnabled || !root.visible) {
                return
            }
            root.topChromeVisible = false
            root.bottomChromeVisible = false
        }
    }

    Component.onCompleted: resetChromeVisibility()

    onActiveChanged: {
        if (active) {
            resetChromeVisibility()
        } else {
            chromeAutoHideTimer.stop()
        }
    }

    onVisibleChanged: {
        if (visible) {
            resetChromeVisibility()
        } else {
            chromeAutoHideTimer.stop()
        }
    }

    onChromeAutoHideEnabledChanged: {
        if (!chromeAutoHideEnabled) {
            topChromeVisible = true
            bottomChromeVisible = true
            chromeAutoHideTimer.stop()
            return
        }
        restartChromeAutoHideTimer()
    }

    // Main content
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        anchors.margins: 1
        color: Theme.windowBackground
        radius: 16
        border.color: Theme.borderColor
        border.width: 1
        clip: true
        antialiasing: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Title bar
            Item {
                id: topBarHost
                Layout.fillWidth: true
                Layout.preferredHeight: root.topChromeVisible ? root.topChromeHeight : 0
                opacity: root.topChromeVisible ? 1 : 0
                visible: Layout.preferredHeight > 0 || opacity > 0
                clip: true

                Behavior on Layout.preferredHeight {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 160; easing.type: Easing.OutQuad }
                }

                ConferenceTitleBar {
                    id: titleBar
                    anchors.fill: parent
                    targetWindow: root
                    backend: backend
                    shareUrl: {
                        if (!backend.meetingNo || backend.meetingNo.length === 0)
                            return ""
                        var base = settingsBackendInstance.apiUrl.replace(/\/+$/, "")
                        return base + "/join?meetingNo=" + encodeURIComponent(backend.meetingNo)
                    }
                }
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
                        visible: Layout.preferredWidth > 0 || backend.sidebarVisible
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
                        color: Theme.cardBackground
                        radius: 16
                        border.color: Theme.borderColor
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
                            scale: sidebarToggleArea.pressed ? 0.94 : 1.0

                            Behavior on opacity { NumberAnimation { duration: 150 } }
                            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

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
                    color: Theme.windowBackground
                    radius: 16
                    border.color: Theme.borderColor
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
                                color: Theme.cardBackground
                                radius: 12
                                border.color: Theme.borderColor
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
                                    color: Theme.isDark ? Qt.rgba(30/255, 30/255, 40/255, 0.9) : "#FFFFFFEE"
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
                                            color: Theme.textPrimary
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
                                    color: Theme.cardBackground
                                    radius: 12
                                    // Speaker highlight with accent border
                                    border.color: modelData.identity === backend.mainParticipantId ? Theme.accentColor : Theme.borderColor
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
                                        border.color: Theme.isDark ? Qt.rgba(91/255, 141/255, 239/255, 0.2) : "#3B82F620"
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
                                        color: Theme.isDark ? Qt.rgba(30/255, 30/255, 40/255, 0.9) : "#FFFFFFEE"
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
                                                color: Theme.textPrimary
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
            Item {
                id: bottomBarHost
                Layout.fillWidth: true
                Layout.preferredHeight: root.bottomChromeVisible ? root.bottomChromeHeight : 0
                opacity: root.bottomChromeVisible ? 1 : 0
                visible: Layout.preferredHeight > 0 || opacity > 0
                clip: true

                Behavior on Layout.preferredHeight {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 160; easing.type: Easing.OutQuad }
                }

                ControlBar {
                    id: controlBar
                    anchors.fill: parent
                    backend: backend
                    settingsBackend: settingsBackendInstance
                    isGuest: root.isGuest

                    onScreenShareClicked: {
                        if (backend && backend.screenShareSupported) {
                            screenPickerDialog.open()
                        }
                    }
                }
            }
        }

        HoverHandler {
            id: activityHoverHandler

            onPointChanged: function(point) {
                if (!point) {
                    return
                }
                var yInWindowFrame = point.position.y
                if (point.scenePosition) {
                    yInWindowFrame = point.scenePosition.y - windowFrame.y
                }
                root.maybeRevealChromeByPointer(yInWindowFrame)
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.topRevealZoneHeight
            visible: root.chromeAutoHideEnabled && !root.topChromeVisible
            color: "transparent"
            z: 40

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                hoverEnabled: true

                onEntered: {
                    root.topChromeVisible = true
                    root.restartChromeAutoHideTimer()
                }

                onPositionChanged: function(mouse) {
                    root.maybeRevealChromeByPointer(mouse.y)
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.bottomRevealZoneHeight
            visible: root.chromeAutoHideEnabled && !root.bottomChromeVisible
            color: "transparent"
            z: 40

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                hoverEnabled: true

                onEntered: {
                    root.bottomChromeVisible = true
                    root.restartChromeAutoHideTimer()
                }

                onPositionChanged: function(mouse) {
                    root.maybeRevealChromeByPointer(parent.y + mouse.y)
                }
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

        // Right sidebar container with show/hide animation
        Item {
            id: rightSidebarHost
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: topBarHost.height
            anchors.bottom: parent.bottom
            anchors.bottomMargin: bottomBarHost.height
            width: (backend.isChatVisible || backend.isParticipantsVisible) ? 320 : 0
            opacity: (backend.isChatVisible || backend.isParticipantsVisible) ? 1 : 0
            visible: width > 0 || backend.isChatVisible || backend.isParticipantsVisible
            clip: true
            z: 10

            Behavior on width {
                NumberAnimation { duration: 240; easing.type: Easing.OutCubic }
            }
            Behavior on opacity {
                NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
            }

            RightSidebar {
                id: rightSidebar
                anchors.fill: parent
                backend: backend
                isGuest: root.isGuest
            }
        }

        // Settings dialog
        SettingsWindow {
            id: settingsDialog
            onSettingsSaved: {
                if (backend) backend.applyAudioSettings()
            }
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

    Popup {
        id: meetingEndedPopup
        modal: false
        focus: false
        closePolicy: Popup.NoAutoClose
        width: 320
        height: 120
        anchors.centerIn: parent

        background: Rectangle {
            radius: 12
            color: Theme.popupBackground
            border.color: Theme.popupBorder
            border.width: 1
        }

        contentItem: Column {
            anchors.centerIn: parent
            spacing: 8

            Text {
                text: "会议已结束"
                color: Theme.textPrimary
                font.pixelSize: 18
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                text: "主持人已离开会议，正在退出..."
                color: Theme.textMuted
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Timer {
        id: meetingEndedAutoCloseTimer
        interval: 1500
        repeat: false
        onTriggered: {
            backend.confirmLeave()
            if (meetingEndedPopup.opened) {
                meetingEndedPopup.close()
            }
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
            isGuest: root.isGuest
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
