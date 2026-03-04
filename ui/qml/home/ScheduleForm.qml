import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0
import "../utils/DateUtils.js" as DateUtils

ColumnLayout {
    id: root

    property bool loading: false
    property date minimumDate: new Date(new Date().getFullYear(), new Date().getMonth(), new Date().getDate())
    property date selectedDate: root.defaultSelectedDate()
    property string selectedTime: root.defaultSelectedTime()
    property int noJoinMinutes: 15
    property int emptyRoomMinutes: 10

    signal createRoomClicked(string topic,
                             string localDate,
                             int hour,
                             int minute,
                             bool allowGuestJoin,
                             string meetingPassword,
                             int noJoinAutoEndMinutes,
                             int emptyAutoEndMinutes)



    function defaultSelectedDate() {
        var d = new Date(Date.now() + 30 * 60 * 1000)
        return new Date(d.getFullYear(), d.getMonth(), d.getDate())
    }

    function defaultSelectedTime() {
        var d = new Date(Date.now() + 30 * 60 * 1000)
        var rounded = Math.ceil(d.getMinutes() / 15) * 15
        if (rounded >= 60) {
            d.setHours(d.getHours() + 1)
            rounded = 0
        }
        return DateUtils.pad2(d.getHours()) + ":" + DateUtils.pad2(rounded)
    }

    function selectedDateString() {
        return selectedDate.getFullYear() + "-"
                + DateUtils.pad2(selectedDate.getMonth() + 1) + "-"
                + DateUtils.pad2(selectedDate.getDate())
    }

    function selectedHour() {
        if (selectedTime.indexOf(":") < 0) {
            return 0
        }
        var value = parseInt(selectedTime.split(":")[0], 10)
        return isNaN(value) ? 0 : value
    }

    function selectedMinute() {
        if (selectedTime.indexOf(":") < 0) {
            return 0
        }
        var value = parseInt(selectedTime.split(":")[1], 10)
        return isNaN(value) ? 0 : value
    }

    spacing: 10

    Flickable {
        id: formScroll
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: formColumn.implicitHeight

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: formColumn
            width: formScroll.width
            spacing: 0

            // ── 基本信息 ──
            TextField {
                id: topicInput
                Layout.fillWidth: true
                placeholderText: "会议主题（可选）"
                enabled: !root.loading
            }

            // ── 分割线 ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.topMargin: 16
                Layout.bottomMargin: 16
                color: Theme.borderLight
            }

            // ── 日期与时间 ──
            Text {
                text: "预定日期"
                color: Theme.textSecondary
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }

            Rectangle {
                id: datePickerField
                Layout.fillWidth: true
                Layout.topMargin: 6
                implicitHeight: 40
                radius: 10
                color: datePickerMa.containsMouse ? Theme.hoverBackground : Theme.cardBackground
                border.color: calendarPopup.visible ? Theme.accentColor : Theme.borderColor
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        Layout.fillWidth: true
                        text: root.selectedDateString()
                        color: Theme.textPrimary
                        font.pixelSize: 13
                    }

                    Text {
                        text: calendarPopup.visible ? "▲" : "▼"
                        color: Theme.textTertiary
                        font.pixelSize: 10
                    }
                }

                MouseArea {
                    id: datePickerMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (calendarPopup.visible)
                            calendarPopup.close()
                        else
                            calendarPopup.open()
                    }
                }

                Popup {
                    id: calendarPopup
                    y: datePickerField.height + 4
                    width: datePickerField.width
                    height: 300
                    padding: 0
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

                    background: Rectangle {
                        color: "transparent"
                    }

                    contentItem: ScheduleCalendar {
                        selectedDate: root.selectedDate
                        minDate: root.minimumDate
                        onDateSelected: function(value) {
                            root.selectedDate = value
                            calendarPopup.close()
                        }
                    }
                }
            }

            ScheduleTimeSelect {
                Layout.fillWidth: true
                Layout.topMargin: 12
                selectedTime: root.selectedTime
                enabled: !root.loading
                onTimeSelected: function(value) {
                    root.selectedTime = value
                }
            }

            // ── 分割线 ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.topMargin: 16
                Layout.bottomMargin: 16
                color: "#E5E7EB"
            }

            // ── 安全与策略 ──
            Text {
                text: "安全与策略"
                color: Theme.textSecondary
                font.pixelSize: 13
                font.weight: Font.DemiBold
                Layout.bottomMargin: 6
            }

            TextField {
                id: passwordInput
                Layout.fillWidth: true
                placeholderText: "会议密码（可选，6-32位）"
                enabled: !root.loading
                echoMode: TextInput.Password
            }

            CheckBox {
                id: allowGuestJoinCheck
                Layout.topMargin: 4
                text: "允许游客通过会议号加入"
                enabled: !root.loading
            }

            // ── 两个时长选择器横排 ──
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 10
                spacing: 12

                ScheduleDurationSelect {
                    Layout.fillWidth: true
                    enabled: !root.loading
                    title: "无人加入自动结束"
                    hintText: "开始后无人加入，自动结束"
                    selectedMinutes: root.noJoinMinutes
                    onMinutesSelected: function(value) {
                        root.noJoinMinutes = value
                    }
                }

                ScheduleDurationSelect {
                    Layout.fillWidth: true
                    enabled: !root.loading
                    title: "空房自动结束"
                    hintText: "房间持续为空，自动结束"
                    selectedMinutes: root.emptyRoomMinutes
                    onMinutesSelected: function(value) {
                        root.emptyRoomMinutes = value
                    }
                }
            }

            // ── 分割线 ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.topMargin: 18
                Layout.bottomMargin: 14
                color: "#E5E7EB"
            }

            PrimaryButton {
                Layout.fillWidth: true
                text: "创建预定会议"
                loading: root.loading
                enabled: !root.loading
                onClicked: {
                    root.createRoomClicked(topicInput.text,
                                           root.selectedDateString(),
                                           root.selectedHour(),
                                           root.selectedMinute(),
                                           allowGuestJoinCheck.checked,
                                           passwordInput.text,
                                           root.noJoinMinutes,
                                           root.emptyRoomMinutes)
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 12
            }
        }
    }
}
