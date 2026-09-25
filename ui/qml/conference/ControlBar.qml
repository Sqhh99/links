import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Links
import Links.Backend 1.0

// Fixed footer control bar with split button device controls
Rectangle {
    id: root
    
    height: 60
    color: Theme.windowBackground
    radius: 12
    clip: true
    
    // Top border
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: Theme.separatorColor
    }
    
    property ConferenceBackend backend
    property var settingsBackend: null  // SettingsBackend for device lists
    property bool isGuest: false
    
    signal screenShareClicked()
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 0
        
        // --- Left Group: AV Controls with Split Button ---
        RowLayout {
            visible: !root.isGuest
            Layout.preferredWidth: root.isGuest ? 0 : implicitWidth
            spacing: 12
            Layout.alignment: Qt.AlignVCenter
            
            // Microphone Split Button
            SplitDeviceButton {
                id: micButton
                isActive: backend ? backend.micEnabled : false
                iconOn: "mic"
                iconOff: "mic-off"
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
                iconOn: "video"
                iconOff: "video-off"
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
                visible: !root.isGuest
                iconName: "monitor-up"
                isActive: backend ? backend.screenSharing : false
                enabled: backend ? backend.screenShareSupported : false
                activeColor: Theme.accentLight
                activeIconColor: Theme.accentColor
                toolTip: backend && !backend.screenShareSupported ? "当前平台暂不支持屏幕共享" : "共享屏幕"
                onClicked: {
                    if (backend) {
                        if (backend.screenSharing) backend.stopScreenShare()
                        else root.screenShareClicked()
                    }
                }
            }
            
            IconOnlyButton {
                visible: !root.isGuest
                iconName: "message-square"
                isActive: backend ? backend.isChatVisible : false
                activeColor: Theme.accentLight
                activeIconColor: Theme.accentColor
                toolTip: "聊天"
                onClicked: if (backend) backend.isChatVisible = !backend.isChatVisible
            }
            
            IconOnlyButton {
                iconName: "users"
                isActive: backend ? backend.isParticipantsVisible : false
                activeColor: Theme.accentLight
                activeIconColor: Theme.accentColor
                toolTip: "成员"
                badgeCount: backend ? backend.participants.length : 1
                onClicked: if (backend) backend.isParticipantsVisible = !backend.isParticipantsVisible
            }

            IconOnlyButton {
                iconName: "circle-dot"
                isActive: backend ? backend.recording : false
                enabled: backend ? backend.recordingAvailable : false
                activeColor: Theme.dangerSoft
                activeIconColor: Theme.danger
                toolTip: backend && !backend.recordingAvailable
                         ? "当前构建未启用本地录制"
                         : (backend && backend.recording ? "停止录制" : "开始录制")
                onClicked: if (backend) backend.toggleRecording()
            }
            
            IconOnlyButton {
                iconName: "settings"
                toolTip: "设置"
                onClicked: if (backend) backend.showSettings()
            }
        }
        
        // Center spacer
        Item { Layout.fillWidth: true }
        
        // --- Right Group: Leave ---
        IconOnlyButton {
            iconName: "phone-off"
            danger: true
            implicitWidth: 56
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
        
        implicitWidth: 66  // 40 + 26 seamless
        implicitHeight: 40
        
        // Container with capsule shape
        Rectangle {
            id: container
            anchors.fill: parent
            radius: 10
            color: splitBtn.isActive ? Theme.windowBackground : Theme.dangerSoft
            border.width: 1
            border.color: splitBtn.isActive ? Theme.borderColor : "transparent"
            clip: true
            
            Row {
                anchors.fill: parent
                spacing: 0
                
                // Left: Main toggle button
                Rectangle {
                    id: mainButton
                    width: 40
                    height: parent.height
                    color: mainButtonArea.containsMouse ? Theme.hoverBackground : "transparent"
                    
                    Icon {
                        anchors.centerIn: parent
                        name: splitBtn.isActive ? splitBtn.iconOn : splitBtn.iconOff
                        size: 20
                        color: splitBtn.isActive ? Theme.iconPrimary : Theme.danger
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
                    color: splitBtn.isActive ? Theme.separatorColor : Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.25)
                }
                
                // Right: Dropdown button with arrow
                Rectangle {
                    id: dropdownButton
                    width: 25
                    height: parent.height
                    color: dropdownArea.containsMouse ? Theme.hoverBackground : "transparent"
                    
                    Icon {
                        anchors.centerIn: parent
                        name: menuPopup.visible ? "chevron-down" : "chevron-up"
                        size: 13
                        color: Theme.iconSecondary
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
                color: Theme.popupBackground
                radius: 12
                border.width: 1
                border.color: Theme.popupBorder
                
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: Theme.shadowColor
                    shadowBlur: 0.8
                    shadowVerticalOffset: 4
                }
            }
            
            contentItem: Column {
                id: menuContent
                padding: 8
                spacing: 4
                
                // Header
                Text {
                    text: "选择设备"
                    color: Theme.textMuted
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
                        color: modelData.id === splitBtn.selectedDeviceId ? Theme.activeBackground : (deviceItemArea.containsMouse ? Theme.hoverBackground : "transparent")
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8
                            
                            Text {
                                text: modelData.name || modelData.label || modelData.id
                                color: modelData.id === splitBtn.selectedDeviceId ? Theme.accentColor : Theme.textSecondary
                                font.pixelSize: 12
                                font.weight: modelData.id === splitBtn.selectedDeviceId ? Font.Bold : Font.Normal
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            
                            // Checkmark for selected
                            Icon {
                                visible: modelData.id === splitBtn.selectedDeviceId
                                name: "check"
                                size: 14
                                color: Theme.brand
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
                    color: Theme.separatorColor
                }
                
                // Settings button
                Rectangle {
                    width: 244
                    height: 36
                    radius: 8
                    color: settingsArea.containsMouse ? Theme.hoverBackground : "transparent"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        spacing: 8
                        
                        Icon {
                            name: "settings"
                            size: 14
                        }
                        
                        Text {
                            text: "音视频设置..."
                            color: Theme.textSecondary
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
        property string iconName: ""
        property bool isActive: false
        property bool danger: false
        property color activeColor: Theme.accentLight
        property color activeIconColor: Theme.accentColor
        property string toolTip: ""
        property int badgeCount: 0
        
        implicitWidth: 44
        implicitHeight: 38
        opacity: enabled ? 1.0 : 0.45
        
        background: Rectangle {
            color: iBtn.danger ? (iBtn.hovered ? Theme.dangerHover : Theme.danger)
                 : iBtn.isActive ? iBtn.activeColor
                 : (iBtn.enabled && iBtn.hovered ? Theme.hoverBackground : Theme.hoverClear)
            radius: 10

            Behavior on color { ColorAnimation { duration: 120 } }
        }
        
        contentItem: Item {
            Icon {
                anchors.centerIn: parent
                name: iBtn.iconName
                size: 20
                color: iBtn.danger ? "#FFFFFF"
                     : iBtn.isActive ? iBtn.activeIconColor
                     : (iBtn.hovered ? Theme.iconPrimary : Theme.iconSecondary)
            }
            
            // Badge
            Rectangle {
                visible: iBtn.badgeCount > 0
                width: 16
                height: 16
                radius: 8
                color: Theme.danger
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: -2
                anchors.rightMargin: -2
                border.width: 1
                border.color: Theme.windowBackground
                
                Text {
                    anchors.centerIn: parent
                    text: iBtn.badgeCount > 99 ? "99+" : iBtn.badgeCount.toString()
                    color: "white"
                    font.pixelSize: 9
                    font.bold: true
                }
            }
        }
        
        ToolTip.visible: toolTip.length > 0 && hovered
        ToolTip.text: toolTip
        ToolTip.delay: 500
        
        MouseArea {
            anchors.fill: parent
            cursorShape: iBtn.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onPressed: function(mouse) { mouse.accepted = false }
        }
    }
}
