import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string meetingTitle: ""
    property string meetingTime: ""
    property string meetingTag: ""

    radius: 10
    color: "#FFFFFF"
    border.color: "#E5E7EB"
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
                color: "#111827"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                text: root.meetingTime
                color: "#6B7280"
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        Rectangle {
            visible: root.meetingTag.length > 0
            radius: 8
            color: "#EFF6FF"
            border.color: "#BFDBFE"
            border.width: 1
            Layout.preferredHeight: 22
            Layout.preferredWidth: tagLabel.implicitWidth + 16

            Text {
                id: tagLabel
                anchors.centerIn: parent
                text: root.meetingTag
                color: "#2563EB"
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
        }
    }
}
