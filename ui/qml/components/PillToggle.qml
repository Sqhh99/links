import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

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
        color: root.checked ? Theme.pillActiveBg : Theme.pillInactiveBg
        border.color: root.checked ? Theme.pillActiveBg : Theme.pillInactiveBorder
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
        color: root.checked ? "white" : Theme.pillInactiveText
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
