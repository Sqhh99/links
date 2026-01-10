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
        placeholderText: "e.g. Alice Smith"
        
        Keys.onReturnPressed: root.joinClicked()
    }
    
    Text {
        text: "会议号 / 房间名"
        color: "#374151"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
    
    TextField {
        id: roomNameInput
        Layout.fillWidth: true
        placeholderText: "如 daily-standup 或会议号"
        
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
