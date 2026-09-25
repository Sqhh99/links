import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

Button {
    id: root

    property color textColor: Theme.brand

    implicitHeight: 24
    leftPadding: 2
    rightPadding: 2

    background: Item {}

    contentItem: Text {
        text: root.text
        color: root.hovered ? Qt.lighter(root.textColor, 1.15) : root.textColor
        font.pixelSize: 12
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
