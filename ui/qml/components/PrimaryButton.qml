import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

Button {
    id: root

    property bool loading: false

    implicitHeight: 46
    leftPadding: 16
    rightPadding: 16

    background: Rectangle {
        color: {
            if (!root.enabled) return Theme.disabledBg
            if (root.pressed) return Theme.accentPressed
            if (root.hovered) return Theme.accentHover
            return Theme.accentColor
        }
        radius: 10

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    contentItem: Item {
        BusyIndicator {
            id: spinner
            visible: root.loading
            running: root.loading
            width: root.loading ? 18 : 0
            height: root.loading ? 18 : 0
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
            font.pixelSize: 15
            font.weight: Font.Bold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
