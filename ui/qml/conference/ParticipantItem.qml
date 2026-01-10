import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// import QtQuick.Effects

Rectangle {
    id: root
    
    height: 52
    radius: 8
    color: hovered ? "#F3F4F6" : "transparent"
    
    property string identity: ""
    property string name: ""
    property bool micEnabled: false
    property bool camEnabled: false
    property bool isLocal: false
    property bool isLocalHost: false      // Is the local user the host?
    property bool isParticipantHost: false  // Is this participant the host?
    
    property bool hovered: hoverHandler.hovered
    
    signal micToggleClicked(string identity)
    signal cameraToggleClicked(string identity)
    signal kickClicked(string identity)
    
    // Use HoverHandler for stable hover detection
    // This doesn't get "interrupted" when hovering over child buttons
    HoverHandler {
        id: hoverHandler
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 12
        
        // Avatar
        Rectangle {
            width: 36
            height: 36
            radius: 18
            color: "#EFF6FF" // Blue-50
            border.color: "#BFDBFE"
            border.width: 1
            
            Text {
                anchors.centerIn: parent
                text: getInitials()
                color: "#3B82F6" // Blue-500
                font.pixelSize: 13
                font.weight: Font.Bold
                
                function getInitials() {
                    if (!root.name) return "?"
                    var parts = root.name.split(' ')
                    var initials = ""
                    for (var i = 0; i < Math.min(2, parts.length); i++) {
                        if (parts[i].length > 0) {
                            initials += parts[i][0].toUpperCase()
                        }
                    }
                    return initials || "?"
                }
            }
        }
        
        // Name
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            
            Text {
                text: root.name + (root.isLocal ? " (我)" : "")
                color: "#1F2937" // Gray-800
                font.pixelSize: 13
                font.weight: Font.Medium
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            
            Text {
                visible: root.isLocal || root.isParticipantHost
                text: root.isParticipantHost ? "主持人" : "参会者"
                color: "#9CA3AF"
                font.pixelSize: 10
            }
        }

        
        // Status icons (always visible when control buttons are NOT showing)
        RowLayout {
            spacing: 4
            // Hide only when control buttons would be visible
            visible: !(root.isLocalHost && !root.isLocal && !root.isParticipantHost && root.hovered)
            
            // Camera Status
            Image {
                source: root.camEnabled ? "qrc:/res/icon/video.png" : "qrc:/res/icon/close_video.png"
                sourceSize.width: 14; sourceSize.height: 14
                visible: true // restored visibility
                id: camIcon
                opacity: 0.6 // Adding opacity since we can't tint easily without MultiEffect
            }
            /*
            MultiEffect {
                source: camIcon; width: 14; height: 14
                colorization: 1.0
                colorizationColor: root.camEnabled ? "#6B7280" : "#EF4444"
            }
            */
            
            // Mic Status
            Image {
                source: root.micEnabled ? "qrc:/res/icon/Turn_on_the_microphone.png" : "qrc:/res/icon/mute_the_microphone.png"
                sourceSize.width: 14; sourceSize.height: 14
                visible: true // restored visibility
                id: micIcon
                opacity: 0.6
            }
            /*
            MultiEffect {
                source: micIcon; width: 14; height: 14
                colorization: 1.0
                colorizationColor: root.micEnabled ? "#10B981" : "#EF4444"
            }
            */
        }
        
        // Control buttons (host only, not for self) - On Hover
        Row {
            spacing: 2
            visible: root.isLocalHost && !root.isLocal && !root.isParticipantHost && root.hovered
            
            // Mic toggle
            IconButton {
                iconSource: root.micEnabled ? "qrc:/res/icon/Turn_on_the_microphone.png" : "qrc:/res/icon/mute_the_microphone.png"
                iconColor: root.micEnabled ? "#374151" : "#EF4444"
                onClicked: root.micToggleClicked(root.identity)
            }
            
            // Camera toggle
            IconButton {
                iconSource: root.camEnabled ? "qrc:/res/icon/video.png" : "qrc:/res/icon/close_video.png"
                iconColor: root.camEnabled ? "#374151" : "#EF4444"
                onClicked: root.cameraToggleClicked(root.identity)
            }
            
            // Kick
            IconButton {
                iconSource: "qrc:/res/icon/close.png"
                iconColor: "#EF4444"
                hoverColor: "#FEE2E2"
                onClicked: root.kickClicked(root.identity)
            }
        }
    }
    
    component IconButton: Button {
        id: btn
        property string iconSource: ""
        property color iconColor: "#6B7280"
        property color hoverColor: "#E5E7EB"
        
        width: 28
        height: 28
        
        background: Rectangle {
            color: btn.hovered ? btn.hoverColor : "transparent"
            radius: 4
        }
        
        contentItem: Item {
            Image {
                id: icon
                source: btn.iconSource
                sourceSize.width: 14
                sourceSize.height: 14
                anchors.centerIn: parent
                visible: true // restored visibility
                opacity: 0.7
            }
            /*
            MultiEffect {
                source: icon
                anchors.fill: icon
                colorization: 1.0
                colorizationColor: btn.iconColor
            }
            */
        }
    }
}
