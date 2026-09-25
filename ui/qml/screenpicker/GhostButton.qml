import QtQuick
import QtQuick.Controls
import Links

Button {
    id: root
    
    implicitHeight: 40
    leftPadding: 16
    rightPadding: 16
    
    background: Rectangle {
        color: "transparent"
        border.color: root.hovered ? Theme.borderColor : Theme.borderLight
        border.width: 1
        radius: 10
        
        Behavior on border.color {
            ColorAnimation { duration: 100 }
        }
    }
    
    contentItem: Text {
        text: root.text
        color: Theme.textSecondary
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
