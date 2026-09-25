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

    implicitHeight: 36

    onCheckedChanged: active = checked

    background: Rectangle {
        color: root.checked ? Theme.brandSoft : Theme.pillInactiveBg
        border.color: root.checked ? Theme.brand : Theme.pillInactiveBorder
        border.width: 1
        radius: height / 2

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
    }

    contentItem: Text {
        text: root.text
        color: root.checked ? Theme.secondaryText : Theme.pillInactiveText
        font.pixelSize: 13
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
