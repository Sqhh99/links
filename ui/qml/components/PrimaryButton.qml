import QtQuick
import QtQuick.Controls

Button {
    id: root

    property bool loading: false

    implicitHeight: 46
    leftPadding: 16
    rightPadding: 16

    background: Rectangle {
        color: {
            if (!root.enabled) return "#E5E7EB"
            if (root.pressed) return "#1E40AF"
            if (root.hovered) return "#1D4ED8"
            return "#2563EB"
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
            color: root.enabled ? "#ffffff" : "#9CA3AF"
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
