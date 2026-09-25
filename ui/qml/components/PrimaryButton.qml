import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

Button {
    id: root

    property bool loading: false
    property bool danger: false

    implicitHeight: 40
    leftPadding: 18
    rightPadding: 18

    background: Rectangle {
        color: {
            if (!root.enabled) return Theme.disabledBg
            if (root.danger) return root.pressed || root.hovered ? Theme.dangerHover : Theme.danger
            if (root.pressed) return Theme.brandPressed
            if (root.hovered) return Theme.brandHover
            return Theme.brand
        }
        radius: 8

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }

    contentItem: Item {
        implicitWidth: label.implicitWidth + (root.loading ? 26 : 0)
        implicitHeight: label.implicitHeight

        BusyIndicator {
            id: spinner
            visible: root.loading
            running: root.loading
            width: root.loading ? 16 : 0
            height: root.loading ? 16 : 0
            anchors.verticalCenter: label.verticalCenter
            anchors.right: label.left
            anchors.rightMargin: root.loading ? 8 : 0
            palette.dark: "white"
        }

        Text {
            id: label
            anchors.centerIn: parent
            text: root.text
            color: root.enabled ? Theme.textOnAccent : Theme.disabledText
            font.pixelSize: 14
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
