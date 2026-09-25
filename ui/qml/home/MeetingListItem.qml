import QtQuick
import Links
import QtQuick.Layouts
import Links.Backend 1.0

// Flat meeting row: title + status pill, then "time · meeting no."
Rectangle {
    id: root

    property string meetingTitle: ""
    property string meetingTime: ""
    property string meetingTag: ""
    property string meetingNo: ""

    readonly property bool live: meetingTag === "进行中"
    readonly property bool upcoming: meetingTag === "待开始"

    radius: Theme.radiusMd
    color: hover.hovered ? Theme.hoverBackground : Theme.hoverClear
    implicitHeight: 58

    Behavior on color { ColorAnimation { duration: 100 } }

    HoverHandler { id: hover }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: root.meetingTitle
                color: Theme.textPrimary
                font.pixelSize: 14
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth
            }

            StatusPill {
                visible: root.meetingTag.length > 0
                text: root.meetingTag
                live: root.live
                upcoming: root.upcoming
            }

            Item { Layout.fillWidth: true }
        }

        Text {
            text: root.meetingNo.length > 0
                  ? root.meetingTime + "  ·  " + root.meetingNo
                  : root.meetingTime
            color: Theme.textMuted
            font.pixelSize: 12
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
