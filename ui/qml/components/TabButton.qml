import QtQuick
import QtQuick.Controls

Button {
    id: root
    
    property bool active: false
    
    checkable: true
    checked: active
    
    implicitHeight: 36
    
    onCheckedChanged: active = checked
    
    background: Rectangle {
        color: root.checked ? "#2563EB" : "#F3F4F6"
        border.color: root.checked ? "#2563EB" : "#E5E7EB"
        border.width: 1
        radius: 10
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
    }
    
    contentItem: Text {
        text: root.text
        color: root.checked ? "white" : "#6B7280"
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
