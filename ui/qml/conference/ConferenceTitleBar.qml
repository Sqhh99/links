import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root
    
    height: 52
    color: "#FFFFFF" // Light theme
    
    property var targetWindow
    property ConferenceBackend backend
    
    // Drag support
    property point dragStartPos
    property bool dragging: false
    
    MouseArea {
        anchors.fill: parent
        
        onPressed: function(mouse) {
            root.dragging = true
            root.dragStartPos = Qt.point(mouse.x, mouse.y)
        }
        
        onPositionChanged: function(mouse) {
            if (root.dragging && root.targetWindow) {
                root.targetWindow.x += mouse.x - root.dragStartPos.x
                root.targetWindow.y += mouse.y - root.dragStartPos.y
            }
        }
        
        onReleased: root.dragging = false
        
        onDoubleClicked: {
            if (backend) {
                backend.isFullscreen = !backend.isFullscreen
            }
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 0
        
        // --- Left Section: Room Name, ID (no icon) ---
        RowLayout {
            spacing: 12
            Layout.alignment: Qt.AlignVCenter
            
            // Room info
            ColumnLayout {
                spacing: 0
                
                Text {
                    text: backend ? backend.roomName : "产品设计评审会"
                    color: "#111827" // Gray-900
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }
                
                Text {
                    text: "ID: 884-291" // Mock/Placeholder
                    color: "#9CA3AF" // Gray-400
                    font.pixelSize: 10
                }
            }
        }
        
        // Center spacer
        Item { Layout.fillWidth: true }
        
        // --- Center Section: Network Status + View Mode Toggle ---
        RowLayout {
            spacing: 16
            Layout.alignment: Qt.AlignVCenter
            
            // Network Status (Mock)
            Rectangle {
                height: 24
                width: 80
                radius: 12
                color: "#ECFDF5" // Emerald-50
                
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Rectangle {
                        width: 6; height: 6; radius: 3
                        color: "#10B981" // Emerald-500
                    }
                    Text {
                        text: "连接稳定"
                        color: "#059669" // Emerald-600
                        font.pixelSize: 11
                        font.weight: Font.Medium
                    }
                }
            }
            
            // View Mode Toggle (Gallery / Speaker)
            Rectangle {
                height: 28
                width: 130
                color: "#F3F4F6" // Gray-100
                radius: 6
                border.color: "#E5E7EB"
                border.width: 1
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 0
                    
                    // Gallery Mode Button
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: (backend && backend.viewMode === "gallery") ? "#FFFFFF" : "transparent"
                        radius: 4
                        
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            
                            Image {
                                source: "qrc:/res/icon/member.png" // Using as grid icon
                                sourceSize.width: 12
                                sourceSize.height: 12
                            }
                            
                            Text {
                                text: "画廊"
                                color: (backend && backend.viewMode === "gallery") ? "#111827" : "#6B7280"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (backend) backend.viewMode = "gallery"
                        }
                    }
                    
                    // Speaker Mode Button
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: (!backend || backend.viewMode === "speaker") ? "#FFFFFF" : "transparent"
                        radius: 4
                        
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            
                            Image {
                                source: "qrc:/res/icon/maximize.png"
                                sourceSize.width: 12
                                sourceSize.height: 12
                            }
                            
                            Text {
                                text: "演讲者"
                                color: (!backend || backend.viewMode === "speaker") ? "#111827" : "#6B7280"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (backend) backend.viewMode = "speaker"
                        }
                    }
                }
            }
        }
        
        // Center spacer
        Item { Layout.fillWidth: true }
        
        // --- Right Section: Window Controls ---
        RowLayout {
            spacing: 4
            Layout.alignment: Qt.AlignVCenter
            
            IconButton {
                iconSource: "qrc:/res/icon/minimize.png"
                iconColor: "#9CA3AF"
                hoverColor: "#F3F4F6"
                onClicked: if (root.targetWindow) root.targetWindow.showMinimized()
            }
            
            IconButton {
                iconSource: (backend && backend.isFullscreen) ? "qrc:/res/icon/maximize_recovery.png" : "qrc:/res/icon/maximize.png"
                iconColor: "#9CA3AF"
                hoverColor: "#F3F4F6"
                onClicked: if (backend) backend.isFullscreen = !backend.isFullscreen
            }
            
            IconButton {
                iconSource: "qrc:/res/icon/close.png"
                iconColor: "#9CA3AF"
                hoverColor: "#FEE2E2"
                hoverIconColor: "#EF4444"
                onClicked: if (backend) backend.leave()
            }
        }
    }
    
    // Bottom border
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#E5E7EB"
    }
    
    // Icon Button Component
    component IconButton: Rectangle {
        id: btn
        property string iconSource: ""
        property string toolTipText: ""
        property color iconColor: "#6B7280" // Gray-500
        property color hoverColor: "#F3F4F6" // Gray-100
        property color hoverIconColor: btn.iconColor
        property bool checkable: false
        property bool checked: false
        
        signal clicked()
        
        implicitWidth: 32
        implicitHeight: 32
        radius: 6
        color: mouseArea.containsMouse ? btn.hoverColor : "transparent"
        
        Image {
            id: icon
            source: btn.iconSource
            sourceSize.width: 16
            sourceSize.height: 16
            anchors.centerIn: parent
            visible: true
            opacity: 0.8
        }
        
        ToolTip.visible: toolTipText.length > 0 && mouseArea.containsMouse
        ToolTip.text: toolTipText
        ToolTip.delay: 500
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: btn.clicked()
        }
    }
}
