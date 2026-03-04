import QtQuick
import QtQuick.Controls
import Links.Backend 1.0

Button {
    id: root
    
    property bool active: false
    
    checkable: false
    checked: active
    
    implicitHeight: 36
    
    background: Rectangle {
        color: root.checked ? Theme.tabActiveBg : Theme.tabInactiveBg
        border.color: root.checked ? Theme.tabActiveBg : Theme.tabInactiveBorder
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
        color: root.checked ? Theme.textOnAccent : Theme.tabInactiveText
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
