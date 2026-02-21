import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

ColumnLayout {
    id: root

    property bool loading: false

    signal createRoomClicked(string topic,
                             string localDate,
                             int hour,
                             int minute,
                             bool allowGuestJoin,
                             string meetingPassword,
                             int noJoinAutoEndMinutes,
                             int emptyAutoEndMinutes)

    function pad2(value) {
        return value < 10 ? "0" + value : "" + value
    }

    function selectedDateString() {
        return yearBox.value + "-" + pad2(monthBox.value) + "-" + pad2(dayBox.value)
    }

    Component.onCompleted: {
        var dt = new Date(Date.now() + 30 * 60 * 1000)
        yearBox.value = dt.getFullYear()
        monthBox.value = dt.getMonth() + 1
        dayBox.value = dt.getDate()
        hourBox.value = dt.getHours()
        minuteBox.value = Math.floor(dt.getMinutes() / 5) * 5
    }

    spacing: 10

    Text {
        text: "预定会议"
        color: "#374151"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }

    TextField {
        id: topicInput
        Layout.fillWidth: true
        placeholderText: "会议主题（可选）"
        enabled: !root.loading
    }

    Text {
        text: "预定日期"
        color: "#374151"
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6

        SpinBox {
            id: yearBox
            from: 2024
            to: 2099
            value: 2026
            editable: true
            enabled: !root.loading
            Layout.fillWidth: true
        }

        SpinBox {
            id: monthBox
            from: 1
            to: 12
            value: 1
            editable: true
            enabled: !root.loading
            Layout.fillWidth: true
        }

        SpinBox {
            id: dayBox
            from: 1
            to: 31
            value: 1
            editable: true
            enabled: !root.loading
            Layout.fillWidth: true
        }
    }

    Text {
        text: "预定时间"
        color: "#374151"
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6

        SpinBox {
            id: hourBox
            from: 0
            to: 23
            value: 9
            editable: true
            enabled: !root.loading
            Layout.fillWidth: true
        }

        SpinBox {
            id: minuteBox
            from: 0
            to: 55
            stepSize: 5
            value: 0
            editable: true
            enabled: !root.loading
            Layout.fillWidth: true
        }
    }

    CheckBox {
        id: allowGuestJoinCheck
        text: "允许游客通过会议号加入"
        enabled: !root.loading
    }

    TextField {
        id: passwordInput
        Layout.fillWidth: true
        placeholderText: "会议密码（可选，6-32位）"
        enabled: !root.loading
        echoMode: TextInput.Password
    }

    Text {
        text: "开始后无人入会自动结束（分钟）"
        color: "#374151"
        font.pixelSize: 12
    }

    SpinBox {
        id: noJoinBox
        Layout.fillWidth: true
        from: 1
        to: 180
        value: 15
        editable: true
        enabled: !root.loading
    }

    Text {
        text: "开启后空房自动结束（分钟）"
        color: "#374151"
        font.pixelSize: 12
    }

    SpinBox {
        id: emptyRoomBox
        Layout.fillWidth: true
        from: 1
        to: 180
        value: 10
        editable: true
        enabled: !root.loading
    }

    PrimaryButton {
        Layout.fillWidth: true
        text: "创建预定会议"
        loading: root.loading
        enabled: !root.loading
        onClicked: {
            root.createRoomClicked(topicInput.text,
                                   root.selectedDateString(),
                                   hourBox.value,
                                   minuteBox.value,
                                   allowGuestJoinCheck.checked,
                                   passwordInput.text,
                                   noJoinBox.value,
                                   emptyRoomBox.value)
        }
    }

    Item { Layout.fillHeight: true }
}
