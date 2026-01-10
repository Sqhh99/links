import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root
    
    color: "#13131a"  // Background for visibility
    
    property ConferenceBackend backend
    
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
                spacing: 8
                verticalLayoutDirection: ListView.BottomToTop
                
                model: backend ? backend.chatMessages : []
                
                delegate: ChatMessage {
                    width: ListView.view.width - 24
                    x: 12
                    senderName: modelData.sender
                    message: modelData.message
                    timestamp: modelData.timestamp
                    isLocal: modelData.isLocal
                }
                
                // Auto-scroll
                onCountChanged: {
                    positionViewAtEnd()
                }
            }
        }
        
        // Input area
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: "#13131a"
            
            // Top border
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: "#2a2a35"
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8
                
                TextField {
                    id: messageInput
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: "Type a message..."
                    
                    background: Rectangle {
                        color: "#0e0e14"
                        border.color: messageInput.activeFocus ? "#5865f2" : "#2a2a35"
                        border.width: 1
                        radius: 8
                    }
                    
                    color: "#ffffff"
                    font.pixelSize: 13
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
                        color: parent.hovered ? "#4752c4" : "#5865f2"
                        radius: 8
                    }
                    
                    contentItem: Text {
                        text: "→"
                        color: "white"
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: messageInput.sendMessage()
                }
            }
        }
    }
}
