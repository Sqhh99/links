import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Links
import Links.Backend 1.0

Item {
    id: root

    property var meetingsModel: null
    property bool loading: false
    property string errorMessage: ""

    signal joinMeeting(string meetingNo)
    signal cancelMeeting(string meetingNo)

    TextEdit {
        id: copyHelper
        visible: false
    }

    function copyText(text) {
        copyHelper.text = text
        copyHelper.selectAll()
        copyHelper.copy()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        ListView {
            id: meetingList
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 2
            clip: true
            model: root.meetingsModel
            visible: model && model.count > 0
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                id: row
                width: meetingList.width
                radius: Theme.radiusMd
                color: rowHover.hovered ? Theme.hoverBackground : Theme.hoverClear
                implicitHeight: contentLayout.implicitHeight + 16

                Behavior on color { ColorAnimation { duration: 100 } }

                HoverHandler { id: rowHover }

                ColumnLayout {
                    id: contentLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: title
                            color: Theme.textPrimary
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            Layout.maximumWidth: implicitWidth
                        }

                        StatusPill {
                            visible: tag.length > 0
                            text: tag
                            live: status === "open" || status === "active"
                            upcoming: status === "scheduled"
                        }

                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: time + "  ·  " + meetingNo
                            color: Theme.textMuted
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            Layout.maximumWidth: implicitWidth
                        }

                        Icon {
                            id: copyIcon
                            name: copied ? "check" : "copy"
                            size: 13
                            color: copied ? Theme.success
                                          : (copyHover.hovered ? Theme.brand : Theme.iconMuted)
                            property bool copied: false

                            HoverHandler {
                                id: copyHover
                                cursorShape: Qt.PointingHandCursor
                            }
                            TapHandler {
                                onTapped: {
                                    root.copyText(meetingNo)
                                    copyIcon.copied = true
                                    copyResetTimer.restart()
                                }
                            }
                            Timer {
                                id: copyResetTimer
                                interval: 1500
                                onTriggered: copyIcon.copied = false
                            }

                            ToolTip.visible: copyHover.hovered
                            ToolTip.text: copied ? "已复制" : "复制会议号"
                            ToolTip.delay: 300
                        }

                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 8
                        visible: (canJoin || canCancel)

                        PrimaryButton {
                            text: "加入"
                            implicitHeight: 28
                            leftPadding: 14
                            rightPadding: 14
                            enabled: canJoin && !root.loading
                            visible: canJoin
                            onClicked: root.joinMeeting(meetingNo)
                        }

                        SecondaryButton {
                            text: "取消预定"
                            implicitHeight: 28
                            leftPadding: 12
                            rightPadding: 12
                            enabled: canCancel && !root.loading
                            visible: canCancel
                            onClicked: root.cancelMeeting(meetingNo)
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !meetingList.visible

            // The spacers also fill width; otherwise this column's maximum width
            // is its widest child and the content would sit left-aligned.
            Item { Layout.fillWidth: true; Layout.fillHeight: true }

            BusyIndicator {
                running: root.loading
                visible: root.loading
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
            }

            Icon {
                visible: !root.loading
                name: "calendar"
                size: 36
                color: Theme.iconMuted
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 4
            }

            Text {
                visible: !root.loading
                text: "暂无预定会议"
                color: Theme.textSecondary
                font.pixelSize: 13
                font.weight: Font.Medium
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                visible: !root.loading
                text: "创建预定会议后会显示在这里"
                color: Theme.textMuted
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
            }

            Item { Layout.fillWidth: true; Layout.fillHeight: true }
        }

        Text {
            visible: root.errorMessage.length > 0
            text: root.errorMessage
            color: Theme.danger
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
