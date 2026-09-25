import QtQuick
import QtQuick.Layouts
import Links

// Titled group of SettingsRows on a rounded card
ColumnLayout {
    id: root

    property string title: ""
    default property alias rows: body.data

    Layout.fillWidth: true
    spacing: 8

    Text {
        visible: root.title.length > 0
        text: root.title
        color: Theme.textTertiary
        font.pixelSize: 12
        font.weight: Font.DemiBold
        Layout.leftMargin: 4
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: body.implicitHeight
        radius: Theme.radiusMd
        color: Theme.cardBackground
        border.color: Theme.borderLight
        border.width: 1

        ColumnLayout {
            id: body
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            spacing: 0
        }
    }
}
