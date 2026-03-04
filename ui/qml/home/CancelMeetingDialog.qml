import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Popup {
    id: root

    property string meetingNo: ""
    property string meetingTitle: ""

    signal confirmed(string meetingNo)
    signal cancelled()

    modal: true
    focus: true
    width: 360
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    Overlay.modal: Rectangle {
        color: "#00000066"
    }

    background: Rectangle {
        color: "transparent"
    }

    contentItem: Rectangle {
        radius: 16
        color: Theme.windowBackground
        border.color: Theme.borderLight
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            Text {
                text: "取消预定会议"
                color: Theme.textPrimary
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Text {
                text: root.meetingTitle.length > 0
                    ? "确认取消预定：" + root.meetingTitle
                    : "确认取消该预定会议？"
                color: Theme.textMuted
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                visible: root.meetingNo.length > 0
                text: "会议号: " + root.meetingNo
                color: Theme.textMuted
                font.pixelSize: 11
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                SecondaryButton {
                    text: "返回"
                    Layout.fillWidth: true
                    onClicked: {
                        root.cancelled()
                        root.close()
                    }
                }

                PrimaryButton {
                    text: "确认取消"
                    Layout.fillWidth: true
                    onClicked: {
                        root.confirmed(root.meetingNo)
                        root.close()
                    }
                }
            }
        }
    }
}
