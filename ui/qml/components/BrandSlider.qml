import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

Slider {
    id: control

    implicitHeight: 24

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 200
        width: control.availableWidth
        height: 4
        radius: 2
        color: Theme.switchTrackOff

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 2
            color: Theme.brand
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 16
        implicitHeight: 16
        radius: 8
        color: "#FFFFFF"
        border.color: Theme.brand
        border.width: control.pressed ? 5 : 4

        Behavior on border.width { NumberAnimation { duration: 100 } }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
