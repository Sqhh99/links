import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

// Fixed footer control bar with split button device controls
Rectangle {
    id: root
    
    height: 68
    color: "#FFFFFF"
    radius: 12
    clip: true
    
    // Top border
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: "#E5E7EB"
    }
    
    property ConferenceBackend backend
    property var settingsBackend: null  // SettingsBackend for device lists
    
    signal screenShareClicked()
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 0
        
        // --- Left Group: AV Controls with Split Button ---
        RowLayout {
            spacing: 12
            Layout.alignment: Qt.AlignVCenter
            
            // Microphone Split Button
            SplitDeviceButton {
                id: micButton
                isActive: backend ? backend.micEnabled : false
                iconOn: "qrc:/res/icon/Turn_on_the_microphone.png"
                iconOff: "qrc:/res/icon/mute_the_microphone.png"
                toolTipOn: "静音"
                toolTipOff: "解除静音"
                deviceLabel: "麦克风"
                devices: settingsBackend ? settingsBackend.microphones : []
                selectedDeviceId: settingsBackend ? settingsBackend.selectedMicId : ""
                onToggle: if (backend) backend.toggleMicrophone()
                onDeviceSelected: function(deviceId) { 
                    if (backend) {
                        backend.switchMicrophone(deviceId)
                    }
                }
                onOpenSettings: if (backend) backend.showSettings()
            }
            
            // Camera Split Button
            SplitDeviceButton {
                id: camButton
                isActive: backend ? backend.camEnabled : false
                iconOn: "qrc:/res/icon/video.png"
                iconOff: "qrc:/res/icon/close_video.png"
                toolTipOn: "关闭摄像头"
                toolTipOff: "开启摄像头"
                deviceLabel: "摄像头"
                devices: settingsBackend ? settingsBackend.cameras : []
                selectedDeviceId: settingsBackend ? settingsBackend.selectedCameraId : ""
                onToggle: if (backend) backend.toggleCamera()
                onDeviceSelected: function(deviceId) { 
                    if (backend) {
                        backend.switchCamera(deviceId)
                    }
                }
                onOpenSettings: if (backend) backend.showSettings()
            }
        }
        
        // Center spacer
        Item { Layout.fillWidth: true }
        
        // --- Center Group: Collaboration ---
        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter
            
            IconOnlyButton {
                iconSource: "qrc:/res/icon/monitor-up.png"
                isActive: backend ? backend.screenSharing : false
                activeColor: "#D1FAE5" // emerald-100
                activeIconColor: "#059669" // emerald-600
                toolTip: "共享屏幕"
                onClicked: {
                    if (backend) {
                        if (backend.screenSharing) backend.stopScreenShare()
                        else root.screenShareClicked()
                    }
                }
            }
            
            IconOnlyButton {
                iconSource: "qrc:/res/icon/message.png"
                isActive: backend ? backend.isChatVisible : false
                toolTip: "聊天"
                onClicked: if (backend) backend.isChatVisible = !backend.isChatVisible
            }
            
            IconOnlyButton {
                iconSource: "qrc:/res/icon/user.png"
                isActive: backend ? backend.isParticipantsVisible : false
                toolTip: "成员"
                badgeCount: backend ? backend.participants.length : 1
                onClicked: if (backend) backend.isParticipantsVisible = !backend.isParticipantsVisible
            }
            
            IconOnlyButton {
                iconSource: "qrc:/res/icon/set_up.png"
                toolTip: "设置"
                onClicked: if (backend) backend.showSettings()
            }
        }
        
        // Center spacer
        Item { Layout.fillWidth: true }
        
        // --- Right Group: Leave ---
        IconOnlyButton {
            iconSource: "qrc:/res/icon/hang_up.png"
            toolTip: "结束会议"
            onClicked: if (backend) backend.leave()
        }
    }
    
    // --- Split Device Button Component ---
    component SplitDeviceButton: Item {
        id: splitBtn
        
        property bool isActive: true
        property string iconOn: ""
        property string iconOff: ""
        property string toolTipOn: ""
        property string toolTipOff: ""
        property string deviceLabel: ""
        property var devices: []
        property string selectedDeviceId: ""
        
        signal toggle()
        signal deviceSelected(string deviceId)
        signal openSettings()
        
        implicitWidth: 68  // 42 + 26 seamless
        implicitHeight: 42
        
        // Container with capsule shape
        Rectangle {
            id: container
            anchors.fill: parent
            radius: 12
            color: splitBtn.isActive ? "#FFFFFF" : "#FEF2F2"
            border.width: 1
            border.color: splitBtn.isActive ? "#E5E7EB" : "#FEE2E2"
            clip: true
            
            Row {
                anchors.fill: parent
                spacing: 0
                
                // Left: Main toggle button
                Rectangle {
                    id: mainButton
                    width: 42
                    height: parent.height
                    color: {
                        if (mainButtonArea.containsMouse) {
                            return splitBtn.isActive ? "#F3F4F6" : "#FEE2E2"
                        }
                        return "transparent"
                    }
                    
                    Image {
                        anchors.centerIn: parent
                        source: splitBtn.isActive ? splitBtn.iconOn : splitBtn.iconOff
                        sourceSize.width: 20
                        sourceSize.height: 20
                    }
                    
                    MouseArea {
                        id: mainButtonArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: splitBtn.toggle()
                    }
                    
                    ToolTip.visible: mainButtonArea.containsMouse && !menuPopup.visible
                    ToolTip.text: splitBtn.isActive ? splitBtn.toolTipOn : splitBtn.toolTipOff
                    ToolTip.delay: 500
                }
                
                // Separator line
                Rectangle {
                    width: 1
                    height: parent.height - 12
                    anchors.verticalCenter: parent.verticalCenter
                    color: splitBtn.isActive ? "#E5E7EB" : "#FECACA"
                }
                
                // Right: Dropdown button with arrow
                Rectangle {
                    id: dropdownButton
                    width: 25
                    height: parent.height
                    color: {
                        if (dropdownArea.containsMouse) {
                            return splitBtn.isActive ? "#F3F4F6" : "#FEE2E2"
                        }
                        return "transparent"
                    }
                    
                    Image {
                        anchors.centerIn: parent
                        source: menuPopup.visible ? "qrc:/res/icon/chevron-down.png" : "qrc:/res/icon/chevron-up.png"
                        sourceSize.width: 10
                        sourceSize.height: 10
                        opacity: splitBtn.isActive ? 0.5 : 0.7
                    }
                    
                    MouseArea {
                        id: dropdownArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: function() {
                            if (menuPopup.visible) {
                                menuPopup.close()
                                return
                            }
                            menuPopup.open()
                        }
                    }
                }
            }
        }
        
        // Device selection popup - positioned above the button, left-aligned with arrow
        Popup {
            id: menuPopup
            y: -height - 8
            x: 0  // Left edge aligns with dropdown arrow button (after main button + separator)
            width: 260
            padding: 0
            closePolicy: Popup.CloseOnEscape
            
            background: Rectangle {
                color: "#FFFFFF"
                radius: 12
                border.width: 1
                border.color: "#E5E7EB"
                
                // Shadow effect
                layer.enabled: true
                layer.effect: null
            }
            
            contentItem: Column {
                id: menuContent
                padding: 8
                spacing: 4
                
                // Header
                Text {
                    text: "选择设备"
                    color: "#9CA3AF"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 0.5
                    leftPadding: 8
                    topPadding: 4
                }
                
                // Device list
                Repeater {
                    model: splitBtn.devices
                    
                    Rectangle {
                        width: 244
                        height: 36
                        radius: 8
                        color: modelData.id === splitBtn.selectedDeviceId ? "#EFF6FF" : (deviceItemArea.containsMouse ? "#F9FAFB" : "transparent")
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8
                            
                            Text {
                                text: modelData.name || modelData.label || modelData.id
                                color: modelData.id === splitBtn.selectedDeviceId ? "#2563EB" : "#374151"
                                font.pixelSize: 12
                                font.weight: modelData.id === splitBtn.selectedDeviceId ? Font.Bold : Font.Normal
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            
                            // Checkmark for selected
                            Text {
                                visible: modelData.id === splitBtn.selectedDeviceId
                                text: "✓"
                                color: "#2563EB"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                            }
                        }
                        
                        MouseArea {
                            id: deviceItemArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                splitBtn.deviceSelected(modelData.id)
                                menuPopup.close()
                            }
                        }
                    }
                }
                
                // Separator
                Rectangle {
                    width: 244
                    height: 1
                    color: "#F3F4F6"
                }
                
                // Settings button
                Rectangle {
                    width: 244
                    height: 36
                    radius: 8
                    color: settingsArea.containsMouse ? "#F9FAFB" : "transparent"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        spacing: 8
                        
                        Image {
                            source: "qrc:/res/icon/set_up.png"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            opacity: 0.6
                        }
                        
                        Text {
                            text: "音视频设置..."
                            color: "#6B7280"
                            font.pixelSize: 12
                        }
                    }
                    
                    MouseArea {
                        id: settingsArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            splitBtn.openSettings()
                            menuPopup.close()
                        }
                    }
                }
            }
        }
    }

    // --- Icon Only Button Component ---
    component IconOnlyButton: Button {
        id: iBtn
        property string iconSource: ""
        property bool isActive: false
        property color activeColor: "#EFF6FF"
        property color activeIconColor: "#2563EB"
        property string toolTip: ""
        property int badgeCount: 0
        
        implicitWidth: 48
        implicitHeight: 40
        
        background: Rectangle {
            color: iBtn.isActive ? iBtn.activeColor : "transparent"
            radius: 10
        }
        
        contentItem: Item {
            Image {
                anchors.centerIn: parent
                source: iBtn.iconSource
                sourceSize.width: 22
                sourceSize.height: 22
            }
            
            // Badge
            Rectangle {
                visible: iBtn.badgeCount > 0
                width: 16
                height: 16
                radius: 8
                color: "#EF4444"
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: -2
                anchors.rightMargin: -2
                border.width: 1
                border.color: "white"
                
                Text {
                    anchors.centerIn: parent
                    text: iBtn.badgeCount > 99 ? "99+" : iBtn.badgeCount.toString()
                    color: "white"
                    font.pixelSize: 9
                    font.bold: true
                }
            }
        }
        
        // Hover
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "#000000"
            opacity: (!iBtn.isActive && iBtn.hovered) ? 0.05 : 0
        }
        
        ToolTip.visible: toolTip.length > 0 && hovered
        ToolTip.text: toolTip
        ToolTip.delay: 500
        
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: function(mouse) { mouse.accepted = false }
        }
    }
}
