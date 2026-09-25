import QtQuick
import QtQuick.Controls
import Links

Button {
    id: root
    
    implicitHeight: 40
    leftPadding: 18
    rightPadding: 18
    
    background: Rectangle {
        color: {
            if (!root.enabled) return Theme.disabledBg
            if (root.pressed) return Theme.brandPressed
            if (root.hovered) return Theme.brandHover
            return Theme.brand
        }
        radius: 10
        
        Behavior on color {
            ColorAnimation { duration: 100 }
        }
    }
    
    contentItem: Text {
        text: root.text
        color: root.enabled ? Theme.textOnAccent : Theme.disabledText
        font.pixelSize: 14
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
