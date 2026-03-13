import QtQuick
import Links
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

Button {
    id: root

    property bool active: false
    property string iconSource: ""

    Layout.fillWidth: true
    implicitHeight: 42
    checkable: true
    checked: active

    background: Rectangle {
        color: "transparent"
        radius: 10

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Theme.activeBackground
            opacity: root.checked ? 1 : 0

            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Theme.hoverBackground
            opacity: root.checked ? 0 : (root.hovered ? 1 : 0)

            Behavior on opacity { NumberAnimation { duration: 100 } }
        }
    }

    contentItem: RowLayout {
        spacing: 10
        anchors.fill: parent
        anchors.leftMargin: 12

        Rectangle {
            width: 22
            height: 22
            radius: 6
            color: "transparent"

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: Theme.accentLight
                opacity: root.active ? 1 : 0

                Behavior on opacity { NumberAnimation { duration: 120 } }
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: Theme.hoverBackground
                opacity: (!root.active && root.hovered) ? 1 : 0

                Behavior on opacity { NumberAnimation { duration: 100 } }
            }

            Image {
                source: root.iconSource
                sourceSize.width: 14
                sourceSize.height: 14
                anchors.centerIn: parent
                opacity: root.active ? 0.9 : (root.hovered ? 0.75 : 0.6)
            }
        }

        Text {
            text: root.text
            color: root.active ? Theme.accentHover : Theme.textTertiary
            font.pixelSize: 13
            font.weight: root.active ? Font.DemiBold : Font.Medium
            Layout.fillWidth: true

            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
