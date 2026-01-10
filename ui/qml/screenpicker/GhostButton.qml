import QtQuick
import QtQuick.Controls

Button {
    id: root
    
    implicitHeight: 40
    leftPadding: 16
    rightPadding: 16
    
    background: Rectangle {
        color: "transparent"
        border.color: root.hovered ? "#3d4560" : "#2a3041"
        border.width: 1
        radius: 10
        
        Behavior on border.color {
            ColorAnimation { duration: 100 }
        }
    }
    
    contentItem: Text {
        text: root.text
        color: "#c4c7d3"
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
