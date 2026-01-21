import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

ColumnLayout {
    id: root
    
    property bool loading: false
    
    signal quickJoinClicked()
    
    spacing: 12
    
    Text {
        text: "一键创建临时会议"
        color: "#374151"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
    
    Text {
        text: "将自动生成房间名，分享给参会者即可入会。"
        color: "#6B7280"
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    
    Item { height: 8 }
    
    PrimaryButton {
        Layout.fillWidth: true
        text: "立即创建并进入"
        loading: root.loading
        enabled: !root.loading
        onClicked: root.quickJoinClicked()
    }
    
    Item { Layout.fillHeight: true }
}
