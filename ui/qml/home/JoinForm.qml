import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

ColumnLayout {
    id: root
    
    property alias userName: userNameInput.text
    property alias roomName: roomNameInput.text
    property alias micEnabled: micToggle.active
    property alias camEnabled: camToggle.active
    property bool loading: false
    property bool guestMode: false
    
    signal joinClicked()
    
    spacing: 12
    
    Text {
        text: "显示名称"
        color: "#374151"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
    
    TextField {
        id: userNameInput
        Layout.fillWidth: true
        placeholderText: root.guestMode ? "Guest-XXXX" : "e.g. Alice Smith"
        readOnly: root.guestMode
        
        Keys.onReturnPressed: root.joinClicked()
    }

    Text {
        visible: root.guestMode
        text: "游客模式将使用固定名称格式：Guest-XXXX"
        color: "#6B7280"
        font.pixelSize: 12
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }
    
    Text {
        text: "会议号 / 分享链接"
        color: "#374151"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
    
    TextField {
        id: roomNameInput
        Layout.fillWidth: true
        placeholderText: "如 123456789 或分享链接"
        
        Keys.onReturnPressed: root.joinClicked()
    }
    
    RowLayout {
        spacing: 10
        Layout.fillWidth: true
        
        PillToggle {
            id: micToggle
            Layout.fillWidth: true
            activeText: "麦克风开"
            inactiveText: "麦克风关"
        }
        
        PillToggle {
            id: camToggle
            Layout.fillWidth: true
            activeText: "摄像头开"
            inactiveText: "摄像头关"
        }
    }
    
    PrimaryButton {
        Layout.fillWidth: true
        text: "进入会议"
        loading: root.loading
        enabled: !root.loading
        onClicked: root.joinClicked()
    }
    
    Item { Layout.fillHeight: true }
}
