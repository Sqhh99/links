import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Links
import Links.Backend 1.0

Rectangle {
    id: root

    property var meetingsModel: null
    property bool loading: false
    property string errorMessage: ""

    signal joinMeeting(string meetingNo)
    signal cancelMeeting(string meetingNo)

    radius: 14
    color: Theme.cardBackground
    border.color: Theme.borderLight
    border.width: 1

    TextEdit {
        id: copyHelper
        visible: false
    }

    function tagBackground(status) {
        if (status === "scheduled") return "#FFF7ED"
        if (status === "open") return "#EFF6FF"
        if (status === "ended") return "#F3F4F6"
        if (status === "cancelled") return "#FEF2F2"
        return "#F3F4F6"
    }

    function tagBorder(status) {
        if (status === "scheduled") return "#FCD34D"
        if (status === "open") return "#BFDBFE"
        if (status === "ended") return "#D1D5DB"
        if (status === "cancelled") return "#FECACA"
        return "#D1D5DB"
    }

    function tagColor(status) {
        if (status === "scheduled") return "#B45309"
        if (status === "open") return "#2563EB"
        if (status === "ended") return "#4B5563"
        if (status === "cancelled") return "#B91C1C"
        return "#4B5563"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "我的预定"
                color: Theme.textPrimary
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            BusyIndicator {
                running: root.loading
                visible: root.loading
                width: 18
                height: 18
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

            delegate: Rectangle {
                width: meetingList.width
                radius: 10
                color: Theme.cardBackground
                border.color: Theme.borderLight
                border.width: 1
                implicitHeight: contentLayout.implicitHeight + 20

                ColumnLayout {
                    id: contentLayout
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8



                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            Layout.fillWidth: true
                            text: title
                            color: Theme.textPrimary
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            radius: 8
                            color: root.tagBackground(status)
                            border.color: root.tagBorder(status)
                            border.width: 1
                            Layout.preferredHeight: 22
                            Layout.preferredWidth: statusLabel.implicitWidth + 14

                            Text {
                                id: statusLabel
                                anchors.centerIn: parent
                                text: tag
                                color: root.tagColor(status)
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "会议号: " + meetingNo
                            color: Theme.textMuted
                            font.pixelSize: 11
                        }

                        Image {
                            source: "qrc:/res/icon/copy.png"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            opacity: copyMa.containsMouse ? 1.0 : 0.6

                            MouseArea {
                                id: copyMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    copyHelper.text = meetingNo
                                    copyHelper.selectAll()
                                    copyHelper.copy()
                                    copyTooltip.show()
                                }
                            }
                        }

                        Rectangle {
                            id: copyTooltip
                            visible: false
                            radius: 4
                            color: "#111827"
                            implicitWidth: tooltipText.implicitWidth + 12
                            implicitHeight: 20

                            function show() {
                                visible = true
                                hideTimer.restart()
                            }

                            Text {
                                id: tooltipText
                                anchors.centerIn: parent
                                text: "已复制"
                                color: "#FFFFFF"
                                font.pixelSize: 10
                            }

                            Timer {
                                id: hideTimer
                                interval: 1500
                                onTriggered: copyTooltip.visible = false
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: time
                        color: "#6B7280"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        SecondaryButton {
                            text: "加入"
                            Layout.fillWidth: true
                            enabled: canJoin && !root.loading
                            visible: canJoin
                            onClicked: root.joinMeeting(meetingNo)
                        }

                        SecondaryButton {
                            text: "取消预定"
                            Layout.fillWidth: true
                            enabled: canCancel && !root.loading
                            visible: canCancel
                            onClicked: root.cancelMeeting(meetingNo)
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !meetingList.visible

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: "暂无可管理的预定会议"
                    color: Theme.textSecondary
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: "创建预定会议后会显示在这里"
                    color: Theme.textTertiary
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Text {
            visible: root.errorMessage.length > 0
            text: root.errorMessage
            color: "#DC2626"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
