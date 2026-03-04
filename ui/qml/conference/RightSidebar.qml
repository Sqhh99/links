import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root
    
    color: "#FFFFFF"
    radius: 12
    clip: true
    
    property ConferenceBackend backend
    property bool isGuest: false
    property bool chatActive: backend && !root.isGuest && backend.isChatVisible
    
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
                    text: root.chatActive ? "消息讨论" : "参会成员"
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
                        text: backend ? (root.chatActive ? backend.chatMessages.length : backend.participants.length) : "0"
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
        
        // Content area with animated panel switching
        Item {
            id: contentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            states: [
                State {
                    name: "participants"
                    when: !root.chatActive
                    PropertyChanges { target: participantsPanel; x: 0; opacity: 1; enabled: true }
                    PropertyChanges { target: chatPanel; x: 24; opacity: 0; enabled: false }
                },
                State {
                    name: "chat"
                    when: root.chatActive
                    PropertyChanges { target: participantsPanel; x: -24; opacity: 0; enabled: false }
                    PropertyChanges { target: chatPanel; x: 0; opacity: 1; enabled: true }
                }
            ]

            transitions: Transition {
                NumberAnimation {
                    targets: [participantsPanel, chatPanel]
                    properties: "x,opacity"
                    duration: 220
                    easing.type: Easing.OutCubic
                }
            }

            Item {
                id: participantsPanel
                anchors.fill: parent
                x: 0
                opacity: 1

                ScrollView {
                    anchors.fill: parent
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    
                    ListView {
                        id: participantsList
                        spacing: 2
                        anchors.fill: parent
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
            }
            
            Rectangle {
                id: chatPanel
                anchors.fill: parent
                color: "#F9FAFB" // Gray-50
                x: 24
                opacity: 0
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    
                    // Messages list
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.topMargin: 2
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                        
                        ListView {
                            id: messagesList
                            spacing: 10
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
                        Layout.preferredHeight: 74
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
                            spacing: 10
                            
                            TextField {
                                id: messageInput
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                placeholderText: "输入消息..."
                                placeholderTextColor: "#9CA3AF"
                                
                                background: Rectangle {
                                    color: "#FFFFFF"
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
                                id: sendButton
                                implicitWidth: 40
                                implicitHeight: 40
                                
                                background: Rectangle {
                                    color: sendButton.down ? "#1E40AF" : (sendButton.hovered ? "#1D4ED8" : "#2563EB")
                                    radius: 8
                                }
                                
                                contentItem: Text {
                                    text: "→" // Should use send icon
                                    color: "white"
                                    font.pixelSize: 16
                                    font.weight: Font.DemiBold
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
