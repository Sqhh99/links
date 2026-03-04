import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Popup {
    id: root
    
    width: 900
    height: 640
    modal: true
    closePolicy: Popup.CloseOnEscape
    
    anchors.centerIn: parent
    
    // Signals for selection results
    signal screenSelected(int screenIndex)
    signal windowSelected(var windowId)
    signal cancelled()
    
    // Backend integration
    ScreenPickerBackend {
        id: backend
        
        onAccepted: {
            if (currentTabIndex === 1 && windowShareSupported) {
                var windowInfo = windows[selectedWindowIndex]
                if (windowInfo) {
                    root.windowSelected(windowInfo.windowId)
                }
            } else {
                root.screenSelected(selectedScreenIndex)
            }
            root.close()
        }
        
        onRejected: {
            root.cancelled()
            root.close()
        }
    }
    
    // Expose backend for external access
    property alias pickerBackend: backend
    
    // Re-populate when opening
    onOpened: {
        backend.refreshScreens()
        if (backend.windowShareSupported) {
            backend.refreshWindows()
        }
    }
    
    // Cancel pending operations when closing
    onClosed: {
        backend.cancelPendingOperations()
    }
    
    background: Rectangle {
        color: "transparent"
    }
    
    contentItem: Rectangle {
        id: frame
        color: Theme.windowBackground
        radius: 12
        border.color: Theme.borderColor
        border.width: 1
        
        // Drag support
        property point dragStartPos
        property bool dragging: false
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12
            
            // Title bar
            ScreenPickerTitleBar {
                Layout.fillWidth: true
                onCloseClicked: backend.cancel()
            }
            
            // Tab bar
            ScreenPickerTabBar {
                id: tabBar
                Layout.fillWidth: true
                currentIndex: backend.currentTabIndex
                windowShareSupported: backend.windowShareSupported
                onCurrentIndexChanged: backend.currentTabIndex = currentIndex
            }
            
            // Content area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.cardBackground
                radius: 8
                
                StackLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    currentIndex: backend.currentTabIndex
                    
                    // Screen list
                    ScreenGrid {
                        id: screenGrid
                        items: backend.screens
                        selectedIndex: backend.selectedScreenIndex
                        onSelectedIndexChanged: backend.selectedScreenIndex = selectedIndex
                    }
                    
                    // Window list with refresh button
                    ColumnLayout {
                        spacing: 12
                        
                        RowLayout {
                            Layout.fillWidth: true
                            Item { Layout.fillWidth: true }
                            
                            Button {
                                id: refreshBtn
                                text: "刷新"
                                implicitWidth: 72
                                implicitHeight: 32
                                background: Rectangle { 
                                    color: refreshBtn.hovered ? Theme.hoverBackground : "transparent"
                                    radius: 6 
                                    border.color: Theme.borderColor 
                                }
                                contentItem: Text { 
                                    text: parent.text 
                                    color: Theme.textSecondary
                                    font.pixelSize: 12 
                                    horizontalAlignment: Text.AlignHCenter 
                                    verticalAlignment: Text.AlignVCenter 
                                }
                                onClicked: backend.refreshWindows()
                            }
                        }
                        
                        WindowGrid {
                            id: windowGrid
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            items: backend.windows
                            selectedIndex: backend.selectedWindowIndex
                            onSelectedIndexChanged: backend.selectedWindowIndex = selectedIndex
                        }
                    }
                }
            }
            
            // Button bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                
                Item { Layout.fillWidth: true }
                
                Button {
                    id: cancelBtn
                    text: "取消"
                    implicitWidth: 80
                    implicitHeight: 36
                    background: Rectangle { 
                        color: cancelBtn.hovered ? Theme.hoverBackground : "transparent"
                        radius: 8 
                        border.color: Theme.borderColor 
                    }
                    contentItem: Text { 
                        text: parent.text 
                        color: Theme.textPrimary
                        font.pixelSize: 13 
                        horizontalAlignment: Text.AlignHCenter 
                        verticalAlignment: Text.AlignVCenter 
                    }
                    onClicked: backend.cancel()
                }
                
                Button {
                    id: shareBtn
                    text: backend.shareButtonText
                    enabled: backend.hasSelection
                    implicitHeight: 36
                    implicitWidth: 100
                    
                    background: Rectangle {
                        color: enabled ? (shareBtn.hovered ? Theme.accentHover : Theme.accentColor) : Theme.disabledBackground
                        radius: 8
                    }
                    contentItem: Text {
                        text: parent.text
                        color: Theme.textOnAccent
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: backend.accept()
                }
            }
        }
    }
}
