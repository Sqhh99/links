import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Window {
    id: root
    
    width: 900
    height: 620
    visible: true
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Window
    title: "LiveKit Conference - Join Meeting"
    onClosing: Qt.quit()
    
    // Backend integration
    LoginBackend {
        id: backend
        
        onJoinConference: function(url, token, roomName, userName, isHost) {
            // This will be handled by C++ to create ConferenceWindow
            console.log("Join conference:", roomName)
            root.hide()  // Hide instead of close, so we can show it again after leaving
        }
        
        onSettingsRequested: {
            settingsDialog.open()
        }
    }
    
    // Clock timer
    Timer {
        id: clockTimer
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        
        onTriggered: {
            heroPanel.currentTime = backend.currentTime()
            heroPanel.currentDate = backend.currentDate()
        }
    }
    
    // Main content
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        color: "#F5F7FA"
        radius: 14
        border.color: "#E5E7EB"
        border.width: 1
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            
            // Title bar
            TitleBar {
                Layout.fillWidth: true
                targetWindow: root
                title: "LiveKit Conference"
                
                onSettingsClicked: backend.showSettings()
                onMinimizeClicked: root.showMinimized()
                onCloseClicked: root.close()
            }
            
            // Body
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 24
                Layout.topMargin: 12
                spacing: 24
                
                // Hero panel (left)
                HeroPanel {
                    id: heroPanel
                    Layout.fillHeight: true
                    Layout.preferredWidth: 300
                }
                
                // Login card (right)
                LoginCard {
                    id: loginCard
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    userName: backend.userName
                    roomName: backend.roomName
                    scheduledTime: backend.scheduledTime
                    micEnabled: backend.micEnabled
                    camEnabled: backend.camEnabled
                    loading: backend.loading
                    errorMessage: backend.errorMessage
                    
                    onUserNameChanged: backend.userName = userName
                    onRoomNameChanged: backend.roomName = roomName
                    onScheduledTimeChanged: backend.scheduledTime = scheduledTime
                    onMicEnabledChanged: backend.micEnabled = micEnabled
                    onCamEnabledChanged: backend.camEnabled = camEnabled
                    
                    onJoinClicked: backend.join()
                    onQuickJoinClicked: backend.quickJoin()
                    onCreateRoomClicked: backend.createScheduledRoom()
                }
            }
        }
    }
    
    // Settings dialog
    SettingsDialog {
        id: settingsDialog
    }
}
