import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

Button {
    id: root

    implicitHeight: 46
    leftPadding: 16
    rightPadding: 16

    background: Rectangle {
        color: root.enabled ? (root.hovered ? Theme.secondaryHoverBg : Theme.secondaryBg) : Theme.hoverBackground
        border.color: root.enabled ? Theme.secondaryBorder : Theme.borderLight
        border.width: 1
        radius: 10

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 120 }
        }
    }

    contentItem: Text {
        text: root.text
        color: root.enabled ? Theme.secondaryText : Theme.disabledText
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
