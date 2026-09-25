import QtQuick
import QtQuick.Controls
import Links

// Compact brand-colored toggle switch
Switch {
    id: control

    implicitHeight: 22
    padding: 0
    spacing: 8

    indicator: Rectangle {
        implicitWidth: 36
        implicitHeight: 20
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: height / 2
        color: control.checked ? Theme.brand : Theme.switchTrackOff
        opacity: control.enabled ? 1.0 : 0.45

        Behavior on color { ColorAnimation { duration: 150 } }

        Rectangle {
            width: 16
            height: 16
            radius: 8
            y: 2
            x: control.checked ? parent.width - width - 2 : 2
            color: "#FFFFFF"

            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        }
    }

    contentItem: Text {
        text: control.text
        visible: text.length > 0
        font.pixelSize: 13
        color: Theme.textSecondary
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
