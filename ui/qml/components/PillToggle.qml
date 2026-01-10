import QtQuick
import QtQuick.Controls

Button {
    id: root
    
    property bool active: false
    property string activeText: ""
    property string inactiveText: ""
    
    text: active ? activeText : inactiveText
    checkable: true
    checked: active
    
    implicitHeight: 42
    
    onCheckedChanged: active = checked
    
    background: Rectangle {
        color: root.checked ? "#10B981" : "#F3F4F6"
        border.color: root.checked ? "#10B981" : "#D1D5DB"
        border.width: 1
        radius: 999  // Pill shape
        
        Behavior on color {
            ColorAnimation { duration: 200 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 200 }
        }
    }
    
    contentItem: Text {
        text: root.text
        color: root.checked ? "white" : "#6B7280"
        font.pixelSize: 13
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
