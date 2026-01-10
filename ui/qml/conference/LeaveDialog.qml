import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    
    modal: true
    width: 360
    // Height will be determined by contentItem's implicitHeight + padding
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 24 // Use padding on Popup instead of margins on layout
    
    signal confirmClicked()
    signal cancelClicked()
    
    background: Rectangle {
        color: "#FFFFFF"
        radius: 16
        border.color: "#E5E7EB"
        border.width: 1
        
        // Shadow effect
        layer.enabled: true
        layer.effect: null // Placeholder for shadow if needed, but keeping it clean
    }
    
    contentItem: ColumnLayout {
        spacing: 32 // More space between sections
        
        // Text content
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12
            
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: "结束会议"
                color: "#111827"
                font.pixelSize: 18
                font.weight: Font.Bold
                horizontalAlignment: Text.AlignHCenter
            }
            
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: "您确定要离开当前会议吗？"
                color: "#6B7280"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
        
        // Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            
            Button {
                id: cancelButton
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                
                background: Rectangle {
                    color: cancelButton.down ? "#E5E7EB" : (cancelButton.hovered ? "#F3F4F6" : "#FFFFFF")
                    radius: 8
                    border.color: "#D1D5DB"
                    border.width: 1
                }
                
                contentItem: Text {
                    text: "取消"
                    color: "#374151"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    root.cancelClicked()
                    root.close()
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: function(mouse) { mouse.accepted = false }
                }
            }
            
            Button {
                id: confirmButton
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                
                background: Rectangle {
                    color: confirmButton.down ? "#991B1B" : (confirmButton.hovered ? "#B91C1C" : "#DC2626")
                    radius: 8
                }
                
                contentItem: Text {
                    text: "退出"
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    root.confirmClicked()
                    root.close()
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: function(mouse) { mouse.accepted = false }
                }
            }
        }
    }
}