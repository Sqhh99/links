import QtQuick
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root

    property string meetingTitle: ""
    property string meetingTime: ""
    property string meetingTag: ""

    radius: 10
    color: Theme.cardBackground
    border.color: Theme.borderLight
    border.width: 1

    implicitHeight: 64

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true

            Text {
                text: root.meetingTitle
                color: Theme.textPrimary
                font.pixelSize: 13
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                text: root.meetingTime
                color: Theme.textMuted
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        Rectangle {
            visible: root.meetingTag.length > 0
            radius: 8
            color: Theme.accentLight
            border.color: Theme.accentColor
            border.width: 1
            Layout.preferredHeight: 22
            Layout.preferredWidth: tagLabel.implicitWidth + 16

            Text {
                id: tagLabel
                anchors.centerIn: parent
                text: root.meetingTag
                color: Theme.accentColor
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
        }
    }
}
