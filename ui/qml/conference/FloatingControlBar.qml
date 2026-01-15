import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Links.Backend 1.0

/*
 * FloatingControlBar - Always-on-top control bar during screen sharing
 * Features: Mic/Camera toggles with device selection, share timer, end share button
 * This window uses WDA_EXCLUDEFROMCAPTURE to avoid being captured in screen share
 */
Window {
    id: root
    
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    width: 400
    height: 48
    color: "transparent"
    
    // Position at top center of primary screen
    x: Screen.width / 2 - width / 2
    y: 16
    
    property ConferenceBackend backend
    property var settingsBackend: null
    
    signal stopSharingClicked()
    
    // Apply capture exclusion when window is shown
    Component.onCompleted: {
        if (backend && backend.shareMode) {
            backend.shareMode.excludeFromCapture(root)
        }
    }
    
    // Dragging support
    property point dragStart: Qt.point(0, 0)
    
    // Main container with frosted glass effect
    Rectangle {
        id: container
        anchors.fill: parent
        radius: 24
        color: Qt.rgba(255, 255, 255, 0.95)
        border.width: 1
        border.color: "#E5E7EB"
        
        // Shadow effect
        layer.enabled: true
        layer.effect: null  // No built-in shadow, use border for now
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8
            
            // --- Left: Share status and timer ---
            Item {
                Layout.preferredWidth: statusRow.width
                Layout.fillHeight: true
                
                RowLayout {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: 6
                    
                    // Red recording dot
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#EF4444"
                        
                        SequentialAnimation on opacity {
                            running: true
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 800 }
                            NumberAnimation { to: 1.0; duration: 800 }
                        }
                    }
                    
                    Text {
                        text: "正在共享"
                        color: "#374151"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                    
                    Text {
                        text: backend && backend.shareMode ? backend.shareMode.formattedTime : "00:00"
                        color: "#6B7280"
                        font.pixelSize: 12
                        font.family: "Consolas"
                    }
                }
                
                // Drag handler for moving the window
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SizeAllCursor
                    
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
            
            // Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                color: "#E5E7EB"
            }
            
            // --- Center: Media controls ---
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 6
                
                // Microphone button
                ControlButton {
                    id: micButton
                    isActive: backend ? backend.micEnabled : false
                    iconOn: "qrc:/res/icon/Turn_on_the_microphone.png"
                    iconOff: "qrc:/res/icon/mute_the_microphone.png"
                    toolTip: isActive ? "静音" : "解除静音"
                    onClicked: if (backend) backend.toggleMicrophone()
                }
                
                // Camera button
                ControlButton {
                    id: camButton
                    isActive: backend ? backend.camEnabled : false
                    iconOn: "qrc:/res/icon/video.png"
                    iconOff: "qrc:/res/icon/close_video.png"
                    toolTip: isActive ? "关闭摄像头" : "开启摄像头"
                    onClicked: if (backend) backend.toggleCamera()
                }
                
                // Separator
                Rectangle {
                    width: 1
                    height: 20
                    color: "#E5E7EB"
                }
                
                // More button (placeholder for V2)
                ControlButton {
                    iconOn: "qrc:/res/icon/set_up.png"
                    iconOff: "qrc:/res/icon/set_up.png"
                    toolTip: "更多"
                    enabled: false
                    opacity: 0.4
                }
            }
            
            // Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                color: "#E5E7EB"
            }
            
            // --- Right: End Share button ---
            Button {
                id: endShareButton
                Layout.preferredWidth: 80
                Layout.preferredHeight: 32
                
                contentItem: Text {
                    text: "结束共享"
                    color: "white"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                background: Rectangle {
                    radius: 16
                    color: endShareButton.hovered ? "#BE123C" : "#E11D48"
                }
                
                onClicked: root.stopSharingClicked()
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: function(mouse) { mouse.accepted = false }
                }
            }
        }
    }
    
    // --- Control Button Component ---
    component ControlButton: Button {
        id: ctrlBtn
        property bool isActive: false
        property string iconOn: ""
        property string iconOff: ""
        property string toolTip: ""
        
        implicitWidth: 36
        implicitHeight: 36
        
        contentItem: Image {
            source: ctrlBtn.isActive ? ctrlBtn.iconOn : ctrlBtn.iconOff
            sourceSize.width: 18
            sourceSize.height: 18
            anchors.centerIn: parent
        }
        
        background: Rectangle {
            radius: 18
            color: {
                if (!ctrlBtn.enabled) return "transparent"
                if (ctrlBtn.hovered) return ctrlBtn.isActive ? "#F3F4F6" : "#FEE2E2"
                return ctrlBtn.isActive ? "transparent" : "#FEF2F2"
            }
            border.width: ctrlBtn.isActive ? 0 : 1
            border.color: "#FEE2E2"
        }
        
        ToolTip.visible: toolTip.length > 0 && hovered
        ToolTip.text: toolTip
        ToolTip.delay: 500
        
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: function(mouse) { mouse.accepted = false }
        }
    }
}
