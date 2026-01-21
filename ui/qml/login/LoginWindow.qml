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
    title: "Links - 登录"
    onClosing: Qt.quit()
    
    // Backend integration
    LoginBackend {
        id: backend

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
                title: "Links"
                
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
                    Layout.preferredWidth: 320
                }
                
                // Auth card (right)
                LoginCard {
                    id: loginCard
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }
    
    // Settings dialog
    SettingsWindow {
        id: settingsDialog
    }
}
