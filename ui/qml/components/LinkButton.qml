import QtQuick
import QtQuick.Controls

Button {
    id: root

    implicitHeight: 26
    leftPadding: 4
    rightPadding: 4

    background: Rectangle {
        color: "transparent"
    }

    contentItem: Text {
        text: root.text
        color: root.hovered ? "#1D4ED8" : "#2563EB"
        font.pixelSize: 12
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
