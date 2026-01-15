import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Popup {
    id: root
    
    width: 720
    height: 520
    modal: true
    closePolicy: Popup.CloseOnEscape

    property point dragLastGlobal: Qt.point(0, 0)
    property bool dragging: false
    
    anchors.centerIn: parent
    
    // Backend integration
    SettingsBackend {
        id: backend
        
        Component.onCompleted: {
            refreshDevices()
            loadSettings()
        }
        
        onAccepted: root.close()
        onRejected: root.close()
    }
    
    background: Rectangle {
        id: frame
        color: "#FFFFFF"
        radius: 16
        border.color: "#E5E7EB"
        border.width: 1
        antialiasing: true
    }
    
    contentItem: Item {
        anchors.fill: parent
        clip: true
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 0 // Zero margins, handle inside
            spacing: 0
            
            // Body container
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0
                
                // Left Sidebar
                Rectangle {
                    Layout.preferredWidth: 200
                    Layout.fillHeight: true
                    color: "#F9FAFB" // Light sidebar
                    
                    // Borders
                    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: "#E5E7EB" }
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 24
                        
                        Text {
                            text: "设置"
                            color: "#111827"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                        }
                        
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            
                            NavButton {
                                text: "音频"
                                iconSource: "qrc:/res/icon/Turn_on_the_microphone.png" // Use generic if available or tint
                                active: pageStack.currentIndex === 0
                                onClicked: pageStack.currentIndex = 0
                            }
                            
                            NavButton {
                                text: "视频"
                                iconSource: "qrc:/res/icon/video.png"
                                active: pageStack.currentIndex === 1
                                onClicked: pageStack.currentIndex = 1
                            }
                            
                            NavButton {
                                text: "网络"
                                iconSource: "qrc:/res/icon/network.png" // Placeholder
                                active: pageStack.currentIndex === 2
                                onClicked: pageStack.currentIndex = 2
                            }
                        }
                        
                        Item { Layout.fillHeight: true }
                    }
                }
                
                // Right Content
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#FFFFFF"
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 20
                        
                        // Header area with Close button
                        RowLayout {
                            Layout.fillWidth: true
                            Item {
                                Layout.fillWidth: true
                                height: 32
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.SizeAllCursor
                                    preventStealing: true
                                    onPressed: function(mouse) {
                                        root.dragging = true
                                        root.dragLastGlobal = Qt.point(root.x + mouse.x, root.y + mouse.y)
                                    }
                                    onPositionChanged: function(mouse) {
                                        if (!root.dragging) return
                                        var globalPos = Qt.point(root.x + mouse.x, root.y + mouse.y)
                                        root.x += globalPos.x - root.dragLastGlobal.x
                                        root.y += globalPos.y - root.dragLastGlobal.y
                                        root.dragLastGlobal = globalPos
                                    }
                                    onReleased: root.dragging = false
                                }
                            }
                            IconButton {
                                iconSource: "qrc:/res/icon/close.png"
                                iconColor: "#6B7280"
                                hoverColor: "#F3F4F6"
                                onClicked: backend.cancel()
                            }
                        }
                        
                        // Content pages
                        StackLayout {
                            id: pageStack
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: 0
                            
                            AudioSettings {
                                backend: backend
                            }
                            
                            VideoSettings {
                                backend: backend
                            }
                            
                            NetworkSettings {
                                backend: backend
                            }
                        }
                        
                        // Footer Buttons
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            
                            Item { Layout.fillWidth: true }
                            
                            Button {
                                text: "取消"
                                implicitWidth: 80
                                implicitHeight: 36
                                
                                background: Rectangle {
                                    color: parent.hovered ? "#F3F4F6" : "transparent"
                                    radius: 8
                                    border.color: "#D1D5DB"
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "#374151"
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: backend.cancel()
                            }
                            
                            Button {
                                text: "保存"
                                implicitWidth: 80
                                implicitHeight: 36
                                
                                background: Rectangle {
                                    color: parent.hovered ? "#1D4ED8" : "#2563EB"
                                    radius: 8
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: backend.save()
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Navigation button component
    component NavButton: Button {
        id: navBtn
        property bool active: false
        property string iconSource: ""
        
        Layout.fillWidth: true
        implicitHeight: 40
        checkable: true
        checked: active
        
        background: Rectangle {
            color: navBtn.checked ? "#EFF6FF" : (navBtn.hovered ? "#F3F4F6" : "transparent")
            radius: 8
        }
        
        contentItem: RowLayout {
            spacing: 12
            anchors.fill: parent
            anchors.leftMargin: 12
            
            // Icon Placeholder (Tinted)
            Rectangle {
                width: 20; height: 20
                color: "transparent"
                Image {
                    id: btnIcon
                    source: navBtn.iconSource
                    sourceSize.width: 18; sourceSize.height: 18
                    anchors.centerIn: parent
                    visible: true
                    opacity: 0.7
                }
                /*
                MultiEffect {
                    source: btnIcon; anchors.fill: btnIcon
                    colorization: 1.0
                    colorizationColor: navBtn.active ? "#2563EB" : "#6B7280"
                }
                */
            }
            
            Text {
                text: navBtn.text
                color: navBtn.active ? "#2563EB" : "#4B5563"
                font.pixelSize: 14
                font.weight: navBtn.active ? Font.Bold : Font.Medium
                Layout.fillWidth: true
            }
        }
        
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: function(mouse) { mouse.accepted = false }
        }
    }
    
    component IconButton: Button {
        id: btn
        property string iconSource: ""
        property color iconColor: "#6B7280"
        property color hoverColor: "#F3F4F6"
        
        width: 32
        height: 32
        
        background: Rectangle {
            color: btn.hovered ? btn.hoverColor : "transparent"
            radius: 6
        }
        
        contentItem: Item {
            Image {
                id: icon
                source: btn.iconSource
                sourceSize.width: 16
                sourceSize.height: 16
                anchors.centerIn: parent
                visible: true
                opacity: 0.7
            }
            /*
            MultiEffect {
                source: icon
                anchors.fill: icon
                colorization: 1.0
                colorizationColor: btn.iconColor
            }
            */
        }
        onClicked: btn.clicked()
    }
}
