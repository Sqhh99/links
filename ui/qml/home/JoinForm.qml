import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

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
        text: root.guestMode ? "游客名称" : "显示名称"
        color: Theme.textSecondary
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
    
    TextField {
        id: userNameInput
        visible: !root.guestMode
        Layout.fillWidth: true
        placeholderText: "e.g. Alice Smith"
        readOnly: false
        
        Keys.onReturnPressed: root.joinClicked()
    }

    Rectangle {
        visible: root.guestMode
        Layout.fillWidth: true
        implicitHeight: 44
        radius: 10
        color: Theme.sidebarBackground
        border.color: Theme.borderLight
        border.width: 1

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
            text: root.userName
            color: Theme.textSecondary
            font.pixelSize: 13
            font.weight: Font.Medium
        }
    }
    
    Text {
        text: root.guestMode ? "会议号 / 分享链接 / 房间名称" : "会议号 / 分享链接"
        color: Theme.textSecondary
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
    
    TextField {
        id: roomNameInput
        Layout.fillWidth: true
        placeholderText: root.guestMode ? "如 123456789、分享链接，或普通房间名" : "如 123456789 或分享链接"
        
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
