import QtQuick
import QtQuick.Controls

Button {
    id: root

    implicitHeight: 46
    leftPadding: 16
    rightPadding: 16

    background: Rectangle {
        color: root.enabled ? (root.hovered ? "#EEF2FF" : "#F8FAFC") : "#F3F4F6"
        border.color: root.enabled ? "#CBD5F5" : "#E5E7EB"
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
        color: root.enabled ? "#1D4ED8" : "#9CA3AF"
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
