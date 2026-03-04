import QtQuick
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Rectangle {
    id: root

    property bool isGuest: true
    property var meetingsModel: null
    property string headerTitle: "最近会议记录"
    property string headerTag: ""
    property string actionText: "会议记录 >"

    signal actionClicked()
    signal quickMeetingClicked()
    signal joinMeetingClicked()

    radius: 14
    color: Theme.cardBackground
    border.color: Theme.borderLight
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: root.headerTitle
                color: Theme.textPrimary
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }

            Rectangle {
                visible: root.headerTag.length > 0
                radius: 8
                color: Theme.hoverBackground
                border.color: Theme.borderLight
                border.width: 1
                Layout.preferredHeight: 20
                Layout.preferredWidth: tagLabel.implicitWidth + 14

                Text {
                    id: tagLabel
                    anchors.centerIn: parent
                    text: root.headerTag
                    color: Theme.textMuted
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                }
            }

            Item { Layout.fillWidth: true }

            LinkButton {
                text: root.actionText
                visible: meetingList.visible
                enabled: meetingList.visible
                onClicked: root.actionClicked()
            }
        }

        ListView {
            id: meetingList
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10
            clip: true
            model: root.meetingsModel
            visible: model && model.count > 0

            delegate: MeetingListItem {
                width: meetingList.width
                meetingTitle: title
                meetingTime: time
                meetingTag: tag
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !meetingList.visible

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10

                Image {
                    source: "qrc:/res/icon/panel-right-open.png"
                    sourceSize.width: 48
                    sourceSize.height: 48
                    opacity: 0.28
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: root.isGuest ? "暂无本地会议记录" : "暂无会议记录"
                    color: Theme.textSecondary
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: "发起会议或加入会议后将自动记录"
                    color: Theme.textTertiary
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: root.isGuest ? "登录后可跨设备同步记录" : ""
                    color: "#CBD5F5"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    visible: root.isGuest
                }
            }
        }
    }
}
