import QtQuick
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Item {
    id: root

    property bool isGuest: true
    property var meetingsModel: null

    signal quickMeetingClicked()
    signal joinMeetingClicked()

    ListView {
        id: meetingList
        anchors.fill: parent
        spacing: 2
        clip: true
        model: root.meetingsModel
        visible: model && model.count > 0
        boundsBehavior: Flickable.StopAtBounds

        delegate: MeetingListItem {
            width: meetingList.width
            meetingTitle: title
            meetingTime: time
            meetingTag: tag
            meetingNo: model.meetingNo !== undefined ? model.meetingNo : ""
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: !meetingList.visible
        spacing: 8

        Icon {
            name: "inbox"
            size: 36
            color: Theme.iconMuted
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 4
        }

        Text {
            text: root.isGuest ? "暂无本地会议记录" : "暂无会议记录"
            color: Theme.textSecondary
            font.pixelSize: 13
            font.weight: Font.Medium
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: root.isGuest ? "登录后可跨设备同步记录" : "发起或加入会议后将自动记录"
            color: Theme.textMuted
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
