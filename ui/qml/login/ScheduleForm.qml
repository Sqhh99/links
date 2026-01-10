import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

ColumnLayout {
    id: root
    
    property alias scheduledTime: timeInput.text
    property bool loading: false
    
    signal createRoomClicked()
    
    spacing: 12
    
    Text {
        text: "预定会议"
        color: "#374151"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
    
    TextField {
        id: timeInput
        Layout.fillWidth: true
        placeholderText: "填写预定时间说明，例如：明日 10:00"
    }
    
    Text {
        text: "预定后将生成会议号并保存到本地提醒。"
        color: "#6B7280"
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    
    PrimaryButton {
        Layout.fillWidth: true
        text: "预定并生成会议"
        loading: root.loading
        enabled: !root.loading
        onClicked: root.createRoomClicked()
    }
    
    Item { Layout.fillHeight: true }
}
