import QtQuick
import QtQuick.Controls

Button {
    id: root
    
    implicitHeight: 40
    leftPadding: 18
    rightPadding: 18
    
    background: Rectangle {
        color: {
            if (!root.enabled) return "#2f3a2f"
            if (root.pressed) return "#5aa830"
            if (root.hovered) return "#7bd44a"
            return "#6bbf3e"
        }
        radius: 10
        
        Behavior on color {
            ColorAnimation { duration: 100 }
        }
    }
    
    contentItem: Text {
        text: root.text
        color: root.enabled ? "#0c0f18" : "#6b6f7a"
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
