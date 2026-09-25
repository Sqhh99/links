import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

// Segment of a segmented control. Place several in a RowLayout on a
// Theme.tabInactiveBg track (or use them standalone).
Button {
    id: root

    property bool active: false

    checkable: false
    checked: active

    implicitHeight: 30

    background: Rectangle {
        color: root.checked ? Theme.tabActiveBg : (root.hovered ? Theme.hoverBackground : Theme.tabInactiveBg)
        radius: 7
        border.color: root.checked && !Theme.isDark ? Theme.borderLight : "transparent"
        border.width: 1

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }

    contentItem: Text {
        text: root.text
        color: root.checked ? Theme.tabActiveText : Theme.tabInactiveText
        font.pixelSize: 12
        font.weight: root.checked ? Font.DemiBold : Font.Normal
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
