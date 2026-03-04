import QtQuick
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property string actionText: ""
    property string iconSource: ""

    signal clicked()

    radius: 12
    color: Theme.cardBackground
    border.color: Theme.borderLight
    border.width: 1

    implicitHeight: 120

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        Image {
            source: root.iconSource
            sourceSize.width: 18
            sourceSize.height: 18
            opacity: 0.7
        }

        Text {
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        Text {
            text: root.subtitle
            color: Theme.textMuted
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }

        LinkButton {
            text: root.actionText
            onClicked: root.clicked()
        }
    }
}
