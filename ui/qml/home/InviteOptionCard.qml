import QtQuick
import QtQuick.Layouts
import Links

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property string actionText: ""
    property string iconSource: ""

    signal clicked()

    radius: 12
    color: "#FFFFFF"
    border.color: "#E5E7EB"
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
            color: "#111827"
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        Text {
            text: root.subtitle
            color: "#6B7280"
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
