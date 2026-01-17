import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root
    
    color: "#F5F7FA" // Light theme background
    
    property ConferenceBackend backend
    property var remoteRenderers: ({})
    property int remoteViewRefreshCounter: 0  // Increments to force re-evaluation of remote showScreenInMain
    
    // Listen for remote view state changes
    Connections {
        target: backend
        function onRemoteViewStateChanged(participantId) {
            root.remoteViewRefreshCounter++
        }
        
        // Handle local camera ended - clear local camera frame
        function onLocalCameraEnded() {
            localVideoThumbnail.clearFrame()
        }
        
        // Handle local screen share ended - clear local screen frame
        function onLocalScreenShareEnded() {
            localScreenThumbnail.clearFrame()
        }
        
        // Handle track ended - clear the video frame to show placeholder
        function onRemoteTrackEnded(participantId, isScreenShare) {
            // Find the corresponding VideoThumbnail and clear its frame
            for (var i = 0; i < remoteVideosList.count; i++) {
                var item = remoteVideosList.itemAtIndex(i)
                if (item) {
                    if (isScreenShare) {
                        // Clear screen share thumbnail
                        var screenThumbnail = item.children[1]
                        if (screenThumbnail && screenThumbnail.participantId === participantId + "_screen" 
                            && typeof screenThumbnail.clearFrame === 'function') {
                            screenThumbnail.clearFrame()
                        }
                    } else {
                        // Clear camera thumbnail
                        var camThumbnail = item.children[0]
                        if (camThumbnail && camThumbnail.participantId === participantId
                            && typeof camThumbnail.clearFrame === 'function') {
                            camThumbnail.clearFrame()
                        }
                    }
                }
            }
            // Force re-evaluation of UI state
            root.remoteViewRefreshCounter++
        }
    }
    
    signal thumbnailClicked(string participantId)
    
    function updateLocalFrame(frame) {
        localVideoThumbnail.updateFrame(frame)
    }
    
    function updateLocalScreenFrame(frame) {
        localScreenThumbnail.updateFrame(frame)
    }
    
    function updateRemoteFrame(participantId, frame) {
        // Find and update the remote camera thumbnail in the merged card
        for (var i = 0; i < remoteVideosList.count; i++) {
            var item = remoteVideosList.itemAtIndex(i)
            // The delegate is a Rectangle (remoteCard), find the camera thumbnail inside
            if (item) {
                var camThumbnail = item.children[0]  // First child is remoteCamThumbnail
                if (camThumbnail && camThumbnail.participantId === participantId) {
                    camThumbnail.updateFrame(frame)
                    break
                }
            }
        }
    }
    
    function updateRemoteScreenFrame(participantId, frame) {
        // Find and update the remote screen thumbnail in the merged card
        for (var i = 0; i < remoteVideosList.count; i++) {
            var item = remoteVideosList.itemAtIndex(i)
            // The delegate is a Rectangle (remoteCard), find the screen thumbnail inside
            if (item) {
                var screenThumbnail = item.children[1]  // Second child is remoteScreenThumbnail
                if (screenThumbnail && screenThumbnail.participantId === participantId + "_screen") {
                    screenThumbnail.updateFrame(frame)
                    break
                }
            }
        }
    }
    
    // Right border
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#E5E7EB" // Light gray border
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        
        // Merged local video card (camera + screen share)
        Rectangle {
            id: localCard
            Layout.fillWidth: true
            Layout.preferredHeight: 135
            radius: 8
            color: "#F3F4F6"
            clip: true
            
            // Helper properties
            property bool hasDualStreams: backend ? (backend.camEnabled && backend.screenSharing) : false
            property bool hasSingleStream: backend ? (backend.camEnabled || backend.screenSharing) : false
            
            // Camera video thumbnail - visible only when dual stream AND showing screen in main (camera in sidebar)
            VideoThumbnail {
                id: localVideoThumbnail
                anchors.fill: parent
                visible: localCard.hasDualStreams && backend.showScreenShareInMain
                participantId: "local"
                participantName: backend ? backend.userName + " (You)" : "You"
                micEnabled: backend ? backend.micEnabled : false
                camEnabled: backend ? backend.camEnabled : false
                mirrored: true
                
                
                // Removed: clicking thumbnail should not toggle source, only buttons do
            }
            
            // Screen video thumbnail - visible only when dual stream AND showing camera in main (screen in sidebar)
            VideoThumbnail {
                id: localScreenThumbnail
                anchors.fill: parent
                visible: localCard.hasDualStreams && !backend.showScreenShareInMain
                participantId: "local_screen"
                participantName: "Screen"
                showStatus: false
                
                
                // Removed: clicking thumbnail should not toggle source, only buttons do
            }
            
            // Placeholder - visible when single stream (no video in sidebar)
            Rectangle {
                anchors.fill: parent
                visible: !localCard.hasDualStreams
                color: "#F3F4F6"
                
                Text {
                    anchors.centerIn: parent
                    text: {
                        if (!backend) return "?"
                        var name = backend.userName || ""
                        if (!name) return "?"
                        var parts = name.split(' ')
                        var initials = ""
                        for (var i = 0; i < Math.min(2, parts.length); i++) {
                            if (parts[i].length > 0) initials += parts[i][0].toUpperCase()
                        }
                        return initials || "?"
                    }
                    color: "#6B7280"
                    font.pixelSize: 28
                    font.weight: Font.Bold
                }
            }
            
            // Name overlay at bottom
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 36
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: "#80000000" }
                }
                
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    spacing: 6
                    
                    Text {
                        text: backend ? backend.userName + " (You)" : "You"
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    
                    Image {
                        visible: backend ? backend.micEnabled : false
                        source: "qrc:/res/icon/Turn_on_the_microphone.png"
                        sourceSize.width: 14
                        sourceSize.height: 14
                    }
                }
            }
            
            // Toggle/Mode button (top-right) - always visible when has any stream
            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 6
                width: 28
                height: 28
                radius: 14
                color: toggleArea.containsMouse ? "#A0000000" : "#80000000"
                visible: localCard.hasSingleStream
                
                Image {
                    anchors.centerIn: parent
                    // Single stream: show current mode icon
                    // Dual stream: show icon of what's currently in MAIN VIEW (click to switch to the other)
                    source: {
                        if (!backend) return ""
                        if (localCard.hasDualStreams) {
                            // Dual mode: show what's in main (click switches to sidebar content)
                            return backend.showScreenShareInMain 
                                ? "qrc:/res/icon/camera_staring_sidebar.png"   // Screen in main, click for camera
                                : "qrc:/res/icon/screen_sharing_sidebar.png"   // Camera in main, click for screen
                        } else {
                            // Single mode: show current active source
                            return backend.screenSharing 
                                ? "qrc:/res/icon/screen_sharing_sidebar.png" 
                                : "qrc:/res/icon/camera_staring_sidebar.png"
                        }
                    }
                    sourceSize.width: 16
                    sourceSize.height: 16
                }
                
                MouseArea {
                    id: toggleArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // Always toggle - single stream: confirms view, dual stream: switches
                        if (backend) backend.toggleMainViewSource()
                    }
                }
                
                ToolTip {
                    visible: toggleArea.containsMouse
                    text: {
                        if (!backend) return ""
                        if (localCard.hasDualStreams) {
                            return backend.showScreenShareInMain ? "切换到摄像头" : "切换到屏幕共享"
                        } else {
                            // Single stream: show what's being displayed
                            return backend.screenSharing ? "屏幕共享中" : "摄像头捕获中"
                        }
                    }
                    delay: 500
                }
            }
        }

        
        // "EVERYONE" header
        Text {
            text: "EVERYONE"
            color: "#6B7280" // Dark gray for text
            font.pixelSize: 11
            font.weight: Font.Bold
            font.letterSpacing: 0.5
            Layout.topMargin: 8
        }
        
        // Remote videos
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            
            ListView {
                id: remoteVideosList
                spacing: 12
                
                model: backend ? backend.participants.filter(function(p) { return p.identity !== "local" }) : []
                
                delegate: Rectangle {
                    id: remoteCard
                    width: ListView.view.width
                    height: 135
                    radius: 8
                    color: "#F3F4F6"
                    clip: true
                    
                    // Helper properties - use backend getters
                    property bool hasDualStreams: modelData.camEnabled && modelData.screenSharing
                    property bool hasSingleStream: modelData.camEnabled || modelData.screenSharing
                    property bool hasNoStream: !modelData.camEnabled && !modelData.screenSharing
                    // Include remoteViewRefreshCounter in binding to force re-evaluation on toggle
                    property bool showScreenInMain: {
                        var _ = root.remoteViewRefreshCounter  // Dependency for reactivity
                        return backend ? backend.getRemoteShowScreenInMain(modelData.identity) : true
                    }
                    
                    // Camera video thumbnail - visible only in dual stream when showing screen in main (camera in sidebar)
                    VideoThumbnail {
                        id: remoteCamThumbnail
                        anchors.fill: parent
                        visible: remoteCard.hasDualStreams && remoteCard.showScreenInMain
                        participantId: modelData.identity
                        participantName: modelData.name || modelData.identity
                        micEnabled: modelData.micEnabled
                        camEnabled: modelData.camEnabled
                        
                        // Removed: clicking thumbnail should not toggle source, only buttons do
                    }
                    
                    // Screen video thumbnail - visible only in dual stream when showing camera in main (screen in sidebar)
                    VideoThumbnail {
                        id: remoteScreenThumbnail
                        anchors.fill: parent
                        visible: remoteCard.hasDualStreams && !remoteCard.showScreenInMain
                        participantId: modelData.identity + "_screen"
                        participantName: modelData.name + " (Screen)"
                        showStatus: false
                        
                        // Removed: clicking thumbnail should not toggle source, only buttons do
                    }
                    
                    // Placeholder - visible when single stream mode (not dual)
                    Rectangle {
                        anchors.fill: parent
                        visible: !remoteCard.hasDualStreams
                        color: "#F3F4F6"
                        
                        Text {
                            anchors.centerIn: parent
                            text: {
                                var name = modelData.name || modelData.identity || ""
                                if (!name) return "?"
                                var parts = name.split(' ')
                                var initials = ""
                                for (var i = 0; i < Math.min(2, parts.length); i++) {
                                    if (parts[i].length > 0) initials += parts[i][0].toUpperCase()
                                }
                                return initials || "?"
                            }
                            color: "#6B7280"
                            font.pixelSize: 28
                            font.weight: Font.Bold
                        }
                    }
                    
                    // Name overlay at bottom
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 36
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 1.0; color: "#80000000" }
                        }
                        
                        RowLayout {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 8
                            spacing: 6
                            
                            Text {
                                text: modelData.name || modelData.identity
                                color: "#ffffff"
                                font.pixelSize: 12
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            
                            Image {
                                visible: modelData.micEnabled
                                source: "qrc:/res/icon/Turn_on_the_microphone.png"
                                sourceSize.width: 14
                                sourceSize.height: 14
                            }
                        }
                    }
                    
                    // Toggle/Mode button (top-right) - visible when has any stream
                    Rectangle {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 6
                        width: 28
                        height: 28
                        radius: 14
                        color: remoteToggleArea.containsMouse ? "#A0000000" : "#80000000"
                        visible: remoteCard.hasSingleStream
                        
                        Image {
                            anchors.centerIn: parent
                            source: {
                                if (remoteCard.hasDualStreams) {
                                    // Dual mode: show icon of what clicking will switch TO (same as local)
                                    return remoteCard.showScreenInMain 
                                        ? "qrc:/res/icon/camera_staring_sidebar.png"  // Screen in main, click for camera
                                        : "qrc:/res/icon/screen_sharing_sidebar.png"  // Camera in main, click for screen
                                } else {
                                    // Single mode: show current active source
                                    return modelData.screenSharing 
                                        ? "qrc:/res/icon/screen_sharing_sidebar.png" 
                                        : "qrc:/res/icon/camera_staring_sidebar.png"
                                }
                            }
                            sourceSize.width: 16
                            sourceSize.height: 16
                        }
                        
                        MouseArea {
                            id: remoteToggleArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (backend) backend.toggleRemoteMainViewSource(modelData.identity)
                            }
                        }
                        
                        ToolTip {
                            visible: remoteToggleArea.containsMouse
                            text: {
                                if (remoteCard.hasDualStreams) {
                                    return remoteCard.showScreenInMain ? "切换到摄像头" : "切换到屏幕共享"
                                } else {
                                    return modelData.screenSharing ? "屏幕共享中" : "摄像头捕获中"
                                }
                            }
                            delay: 500
                        }
                    }
                }
            }
        }
    }
}
