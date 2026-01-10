import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

Item {
    id: root
    
    property alias userName: joinForm.userName
    property alias roomName: joinForm.roomName
    property alias scheduledTime: scheduleForm.scheduledTime
    property alias micEnabled: joinForm.micEnabled
    property alias camEnabled: joinForm.camEnabled
    property bool loading: false
    property string errorMessage: ""
    
    signal joinClicked()
    signal quickJoinClicked()
    signal createRoomClicked()
    
    Rectangle {
        anchors.fill: parent
        radius: 14
        color: "#FFFFFF"
        border.color: "#E5E7EB"
        border.width: 1
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        anchors.bottomMargin: 24
        spacing: 16
        
        // Tabs
        RowLayout {
            spacing: 10
            
            TabButton {
                text: "加入会议"
                active: tabGroup.currentIndex === 0
                onClicked: tabGroup.currentIndex = 0
            }
            
            TabButton {
                text: "快速会议"
                active: tabGroup.currentIndex === 1
                onClicked: tabGroup.currentIndex = 1
            }
            
            TabButton {
                text: "预定会议"
                active: tabGroup.currentIndex === 2
                onClicked: tabGroup.currentIndex = 2
            }
            
            Item { Layout.fillWidth: true }
        }
        
        // Form stack
        StackLayout {
            id: tabGroup
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0
            
            JoinForm {
                id: joinForm
                loading: root.loading
                onJoinClicked: root.joinClicked()
            }
            
            QuickStartForm {
                id: quickStartForm
                loading: root.loading
                onQuickJoinClicked: root.quickJoinClicked()
            }
            
            ScheduleForm {
                id: scheduleForm
                loading: root.loading
                onCreateRoomClicked: root.createRoomClicked()
            }
        }
        
        // Error / Status
        Text {
            visible: root.errorMessage.length > 0
            text: root.errorMessage
            color: "#EF4444"
            font.pixelSize: 13
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        
        // Loading indicator
        RowLayout {
            visible: root.loading
            spacing: 8
            
            BusyIndicator {
                running: root.loading
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                palette.dark: "#2563EB"
            }
            
            Text {
                text: "正在连接 LiveKit..."
                color: "#2563EB"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
        }
    }
}
