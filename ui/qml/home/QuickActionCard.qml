import QtQuick
import Links
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property string iconSource: ""
    property color accentColor: "#2563EB"
    property real accentOpacity: 0.18
    property real iconOpacity: 0.9

    signal clicked()

    radius: 14
    color: pressed ? Theme.pressedBackground : (hovered ? Theme.hoverBackground : Theme.cardBackground)
    border.color: hovered ? accentColor : Theme.borderLight
    border.width: 1

    implicitHeight: 152
    implicitWidth: 240

    property bool hovered: false
    property bool pressed: false

    Behavior on border.color {
        ColorAnimation { duration: 120 }
    }
    Behavior on color {
        ColorAnimation { duration: 120 }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 8

        Rectangle {
            width: 40
            height: 40
            radius: 12
            color: accentColor
            opacity: root.accentOpacity

            Image {
                source: root.iconSource
                sourceSize.width: 20
                sourceSize.height: 20
                anchors.centerIn: parent
                opacity: root.iconOpacity
            }
        }

        Text {
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        Text {
            text: root.subtitle
            color: Theme.textMuted
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: root.hovered = true
        onExited: root.hovered = false
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: root.clicked()
    }
}
