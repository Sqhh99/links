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
            if (currentTabIndex === 0) {
                root.screenSelected(selectedScreenIndex)
            } else {
                var windowInfo = windows[selectedWindowIndex]
                if (windowInfo) {
                    root.windowSelected(windowInfo.windowId)
                }
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
        backend.refreshWindows()
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
        color: "#FFFFFF"
        radius: 12
        border.color: "#E5E7EB"
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
                onCurrentIndexChanged: backend.currentTabIndex = currentIndex
            }
            
            // Content area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#F9FAFB" // Gray-50
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
                                text: "刷新"
                                implicitWidth: 72
                                implicitHeight: 32
                                background: Rectangle { color: "white"; radius: 6; border.color: "#E5E7EB" }
                                contentItem: Text { text: parent.text; color: "#4B5563"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
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
                    text: "取消"
                    implicitWidth: 80
                    implicitHeight: 36
                    background: Rectangle { color: parent.hovered ? "#F3F4F6" : "white"; radius: 8; border.color: "#D1D5DB" }
                    contentItem: Text { text: parent.text; color: "#374151"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: backend.cancel()
                }
                
                Button {
                    text: backend.shareButtonText
                    enabled: backend.hasSelection
                    implicitHeight: 36
                    implicitWidth: 100
                    
                    background: Rectangle {
                        color: enabled ? "#2563EB" : "#93C5FD"
                        radius: 8
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
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
