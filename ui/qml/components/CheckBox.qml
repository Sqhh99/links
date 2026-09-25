import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

CheckBox {
    id: control

    implicitHeight: 24

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 5
        border.color: control.checked ? Theme.checkboxCheckedBorder : (control.hovered ? Theme.brand : Theme.checkboxBorder)
        border.width: 1.5
        color: control.checked ? Theme.checkboxCheckedBg : Theme.checkboxBg

        Behavior on color { ColorAnimation { duration: 100 } }
        Behavior on border.color { ColorAnimation { duration: 100 } }

        Icon {
            anchors.centerIn: parent
            name: "check"
            size: 12
            color: "#FFFFFF"
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        font.pixelSize: 13
        opacity: enabled ? 1.0 : 0.4
        color: Theme.textSecondary
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
