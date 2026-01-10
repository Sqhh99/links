import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root
    
    color: "#FFFFFF"
    
    property ConferenceBackend backend
    
    // Shadow for sidebar
    // layer.enabled: true
    /*
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "#0A000000"
        shadowBlur: 16
        shadowHorizontalOffset: -2
    }
    */
    
    // Left border
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#E5E7EB"
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "transparent"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                
                Text {
                    text: backend && backend.isChatVisible ? "消息讨论" : "参会成员"
                    color: "#111827" // Gray-900
                    font.pixelSize: 14
                    font.weight: Font.Bold
                }
                
                // Badge
                Rectangle {
                    visible: true
                    width: countText.implicitWidth + 12
                    height: 20
                    radius: 4
                    color: "#F3F4F6"
                    
                    Text {
                        id: countText
                        anchors.centerIn: parent
                        text: backend ? (backend.isChatVisible ? backend.chatMessages.length : backend.participants.length) : "0"
                        color: "#6B7280"
                        font.pixelSize: 11
                    }
                }
                
                Item { Layout.fillWidth: true }
                
                // Close button for sidebar
                MouseArea {
                    width: 24; height: 24
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (backend) {
                            if (backend.isChatVisible) backend.isChatVisible = false
                            if (backend.isParticipantsVisible) backend.isParticipantsVisible = false
                        }
                    }
                    Text { anchors.centerIn: parent; text: "×"; color: "#9CA3AF"; font.pixelSize: 20 }
                }
            }
            
            // Bottom border
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#E5E7EB"
            }
        }
        
        // Content area - use StackLayout to switch between panels
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: backend && backend.isChatVisible ? 1 : 0
            
            // Index 0: Participants panel
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                
                ListView {
                    id: participantsList
                    spacing: 2
                    anchors.margins: 8
                    
                    model: backend ? backend.participants : []
                    
                    delegate: ParticipantItem {
                        width: ListView.view.width - 16
                        x: 8
                        identity: modelData.identity
                        name: modelData.name
                        micEnabled: modelData.micEnabled
                        camEnabled: modelData.camEnabled
                        isLocal: modelData.isLocal
                        isLocalHost: backend ? backend.isHost : false
                        isParticipantHost: modelData.isHost || false
                        
                        onMicToggleClicked: function(identity) {
                            if (backend) backend.muteParticipant(identity)
                        }
                        onCameraToggleClicked: function(identity) {
                            if (backend) backend.hideParticipantVideo(identity)
                        }
                        onKickClicked: function(identity) {
                            if (backend) backend.kickParticipant(identity)
                        }
                    }
                }
            }
            
            // Index 1: Chat panel
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#F9FAFB" // Gray-50
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    
                    // Messages list
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                        
                        ListView {
                            id: messagesList
                            spacing: 12
                            verticalLayoutDirection: ListView.BottomToTop
                            anchors.margins: 12
                            
                            model: backend ? backend.chatMessages : []
                            
                            delegate: ChatMessage {
                                width: ListView.view.width - 24
                                x: 12
                                senderName: modelData.sender
                                message: modelData.message
                                timestamp: modelData.timestamp
                                isLocal: modelData.isLocal
                            }
                            
                            onCountChanged: {
                                positionViewAtEnd()
                            }
                        }
                    }
                    
                    // Input area
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 72
                        color: "#FFFFFF"
                        
                        // Top border
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: 1
                            color: "#E5E7EB"
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
                            
                            TextField {
                                id: messageInput
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                placeholderText: "输入消息..."
                                placeholderTextColor: "#9CA3AF"
                                
                                background: Rectangle {
                                    color: "#F9FAFB"
                                    border.color: messageInput.activeFocus ? "#2563EB" : "#E5E7EB"
                                    border.width: 1
                                    radius: 8
                                }
                                
                                color: "#111827"
                                font.pixelSize: 14
                                leftPadding: 12
                                rightPadding: 12
                                
                                Keys.onReturnPressed: sendMessage()
                                Keys.onEnterPressed: sendMessage()
                                
                                function sendMessage() {
                                    if (text.trim().length > 0 && backend) {
                                        backend.sendChatMessage(text.trim())
                                        text = ""
                                    }
                                }
                            }
                            
                            Button {
                                implicitWidth: 40
                                implicitHeight: 40
                                
                                background: Rectangle {
                                    color: parent.hovered ? "#1D4ED8" : "#2563EB"
                                    radius: 8
                                }
                                
                                contentItem: Text {
                                    text: "→" // Should use send icon
                                    color: "white"
                                    font.pixelSize: 18
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: messageInput.sendMessage()
                            }
                        }
                    }
                }
            }
        }
    }
}
