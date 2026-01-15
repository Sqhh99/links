import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtMultimedia
import Links.Backend 1.0

/*
 * CameraThumbnail - Floating draggable local camera preview during screen sharing
 * Features: Draggable, minimize to dot, close, local video display
 * This window uses WDA_EXCLUDEFROMCAPTURE to avoid being captured in screen share
 */
Window {
    id: root
    
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    visible: true
    transientParent: null
    
    // Initial position: bottom-right corner of screen
    x: Screen.width - width - 32
    y: Screen.height - height - 100
    
    property ConferenceBackend backend
    property bool minimized: false
    
    // Apply capture exclusion when window is shown
    Component.onCompleted: {
        if (backend && backend.shareMode) {
            backend.shareMode.excludeFromCapture(root)
        }
    }
    
    // Dragging support
    property point dragStart: Qt.point(0, 0)
    
    // State for showing/hiding controls on hover
    property bool hovered: false
    
    // Normal view
    Rectangle {
        id: normalView
        anchors.fill: parent
        visible: !root.minimized
        radius: 12
        color: "#1F2937"
        clip: true
        border.width: 2
        border.color: root.hovered ? "#3B82F6" : "#374151"
        
        // Video renderer (C++ backend - non-visual data provider)
        VideoRenderer {
            id: videoRenderer
            videoSink: videoOutput.videoSink
            mirrored: true  // Mirror local camera
        }
        
        // Connect to backend video frame signal
        Connections {
            target: root.backend
            function onLocalVideoFrameReady(frame) {
                videoRenderer.updateFrame(frame)
            }
        }
        
        // Video output display
        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            anchors.margins: 2
            visible: videoRenderer.hasFrame
        }
        
        // Placeholder when no video
        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            color: "#374151"
            visible: !backend || !backend.camEnabled
            
            Text {
                anchors.centerIn: parent
                text: "📷"
                font.pixelSize: 32
                opacity: 0.5
            }
        }
        
        // Control overlay (visible on hover)
        Rectangle {
            id: controlOverlay
            anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.3)
            opacity: root.hovered ? 1 : 0
            
            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
            
            // Top row: Close and Minimize buttons
            Row {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 6
                spacing: 4
                
                // Minimize button
                Rectangle {
                    width: 24
                    height: 24
                    radius: 12
                    color: minimizeArea.containsMouse ? "#374151" : Qt.rgba(0, 0, 0, 0.4)
                    
                    Text {
                        anchors.centerIn: parent
                        text: "−"
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    
                    MouseArea {
                        id: minimizeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.minimized = true
                    }
                }
            }
        }
        
        // Drag handler
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeAllCursor
            
            onEntered: root.hovered = true
            onExited: root.hovered = false
            
            onPressed: function(mouse) {
                root.dragStart = Qt.point(mouse.x, mouse.y)
            }
            
            onPositionChanged: function(mouse) {
                if (pressed) {
                    root.x += mouse.x - root.dragStart.x
                    root.y += mouse.y - root.dragStart.y
                }
            }
        }
    }
    
    // Minimized view (small dot)
    Rectangle {
        id: minimizedView
        anchors.centerIn: parent
        width: 40
        height: 40
        radius: 20
        visible: root.minimized
        color: minimizedArea.containsMouse ? "#3B82F6" : "#1F2937"
        border.width: 2
        border.color: "#374151"
        
        Text {
            anchors.centerIn: parent
            text: "📷"
            font.pixelSize: 18
        }
        
        MouseArea {
            id: minimizedArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            
            onClicked: root.minimized = false
            
            // Drag support
            onPressed: function(mouse) {
                root.dragStart = Qt.point(mouse.x, mouse.y)
            }
            
            onPositionChanged: function(mouse) {
                if (pressed) {
                    root.x += mouse.x - root.dragStart.x
                    root.y += mouse.y - root.dragStart.y
                }
            }
        }
        
        ToolTip.visible: minimizedArea.containsMouse
        ToolTip.text: "展开摄像头"
        ToolTip.delay: 500
    }
    
    // Adjust window size when minimized (using bindings instead of states for Window)
    width: minimized ? 44 : 180
    height: minimized ? 44 : 120
    
    // Note: Window doesn't support Behavior animations on size, so size change is instant
}
