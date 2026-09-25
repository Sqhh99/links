import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
// import QtQuick.Effects

Rectangle {
    id: root
    
    height: 52
    radius: 8
    color: hovered ? Theme.hoverBackground : "transparent"
    
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
            color: Theme.accentLight
            border.color: Theme.borderLight
            border.width: 1
            
            Text {
                anchors.centerIn: parent
                text: getInitials()
                color: Theme.accentColor
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
                color: Theme.textPrimary
                font.pixelSize: 13
                font.weight: Font.Medium
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            
            Text {
                visible: root.isLocal || root.isParticipantHost
                text: root.isParticipantHost ? "主持人" : "参会者"
                color: Theme.textMuted
                font.pixelSize: 10
            }
        }

        
        // Status icons (always visible when control buttons are NOT showing)
        RowLayout {
            spacing: 4
            // Hide only when control buttons would be visible
            visible: !(root.isLocalHost && !root.isLocal && !root.isParticipantHost && root.hovered)
            
            // Camera Status
            Icon {
                id: camIcon
                name: root.camEnabled ? "video" : "video-off"
                size: 15
                color: root.camEnabled ? Theme.iconSecondary : Theme.danger
            }
            
            // Mic Status
            Icon {
                id: micIcon
                name: root.micEnabled ? "mic" : "mic-off"
                size: 15
                color: root.micEnabled ? Theme.success : Theme.danger
            }
        }
        
        // Control buttons (host only, not for self) - On Hover
        Row {
            spacing: 2
            visible: root.isLocalHost && !root.isLocal && !root.isParticipantHost && root.hovered
            
            // Mic toggle
            MemberActionButton {
                iconName: root.micEnabled ? "mic" : "mic-off"
                iconColor: root.micEnabled ? Theme.iconPrimary : Theme.danger
                onClicked: root.micToggleClicked(root.identity)
            }
            
            // Camera toggle
            MemberActionButton {
                iconName: root.camEnabled ? "video" : "video-off"
                iconColor: root.camEnabled ? Theme.iconPrimary : Theme.danger
                onClicked: root.cameraToggleClicked(root.identity)
            }
            
            // Kick
            MemberActionButton {
                iconName: "user-x"
                iconColor: Theme.danger
                hoverColor: Theme.dangerSoft
                onClicked: root.kickClicked(root.identity)
            }
        }
    }
    
    component MemberActionButton: Button {
        id: btn
        property string iconName: ""
        property color iconColor: Theme.iconSecondary
        property color hoverColor: Theme.hoverBackground
        
        width: 28
        height: 28
        
        background: Rectangle {
            color: btn.hovered ? btn.hoverColor : "transparent"
            radius: 6
        }
        
        contentItem: Item {
            Icon {
                id: icon
                name: btn.iconName
                size: 15
                anchors.centerIn: parent
                color: btn.iconColor
            }
        }
    }
}
