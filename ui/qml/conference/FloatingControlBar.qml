import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Links
import Links.Backend 1.0

/*
 * FloatingControlBar - Always-on-top control bar during screen sharing
 * Features: Mic/Camera toggles with device selection, share timer, end share button
 * This window uses WDA_EXCLUDEFROMCAPTURE to avoid being captured in screen share
 */
Window {
    id: root
    
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    width: 400
    height: 48
    visible: true
    color: "transparent"
    transientParent: null
    
    // Position at top center of primary screen
    x: Screen.width / 2 - width / 2
    y: 16
    
    property ConferenceBackend backend
    property var settingsBackend: null
    property point dragStart: Qt.point(0, 0)
    property int edgePadding: 0
    
    signal stopSharingClicked()
    
    // Apply capture exclusion when window is shown
    Component.onCompleted: {
        if (backend && backend.shareMode) {
            backend.shareMode.excludeFromCapture(root)
        }
    }

    function screenGeometry() {
        if (root.screen && root.screen.availableGeometry) {
            return root.screen.availableGeometry
        }
        return Qt.rect(Screen.virtualX, Screen.virtualY, Screen.width, Screen.height)
    }

    function clampToScreen() {
        var geo = screenGeometry()
        var minX = geo.x + edgePadding
        var maxX = geo.x + geo.width - root.width - edgePadding
        var minY = geo.y + edgePadding
        var maxY = geo.y + geo.height - root.height - edgePadding
        root.x = Math.min(Math.max(root.x, minX), maxX)
        root.y = Math.min(Math.max(root.y, minY), maxY)
    }

    function snapToEdge() {
        var geo = screenGeometry()
        var leftDist = Math.abs(root.x - geo.x)
        var rightDist = Math.abs(geo.x + geo.width - (root.x + root.width))
        var topDist = Math.abs(root.y - geo.y)
        var bottomDist = Math.abs(geo.y + geo.height - (root.y + root.height))
        var minDist = Math.min(leftDist, rightDist, topDist, bottomDist)

        if (minDist === leftDist) {
            root.x = geo.x + edgePadding
        } else if (minDist === rightDist) {
            root.x = geo.x + geo.width - root.width - edgePadding
        } else if (minDist === topDist) {
            root.y = geo.y + edgePadding
        } else {
            root.y = geo.y + geo.height - root.height - edgePadding
        }
        root.clampToScreen()
    }

    function openSettingsDialog() {
        if (!settingsDialog) return

        var geo = screenGeometry()
        var targetX = geo.x + Math.round((geo.width - settingsDialog.width) / 2)
        var targetY = geo.y + Math.round((geo.height - settingsDialog.height) / 2)
        settingsDialog.x = targetX - root.x
        settingsDialog.y = targetY - root.y
        settingsDialog.open()
    }
    
    // Main container with frosted glass effect
    Rectangle {
        id: container
        anchors.fill: parent
        radius: 24
        color: Qt.rgba(255, 255, 255, 0.95)
        border.width: 1
        border.color: "#E5E7EB"
        
        // Shadow effect
        layer.enabled: true
        layer.effect: null  // No built-in shadow, use border for now
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8
            
            // --- Left: Share status and timer ---
            Item {
                Layout.preferredWidth: statusRow.width
                Layout.fillHeight: true
                
                RowLayout {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: 6
                    
                    // Red recording dot
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#EF4444"
                        
                        SequentialAnimation on opacity {
                            running: true
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 800 }
                            NumberAnimation { to: 1.0; duration: 800 }
                        }
                    }
                    
                    Text {
                        text: "正在共享"
                        color: "#374151"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                    
                    Text {
                        text: backend && backend.shareMode ? backend.shareMode.formattedTime : "00:00"
                        color: "#6B7280"
                        font.pixelSize: 12
                        font.family: "Consolas"
                    }
                }
                
                // Drag handler for moving the window
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SizeAllCursor
                    
                    onPressed: function(mouse) {
                        root.dragStart = Qt.point(mouse.x, mouse.y)
                    }
                    
                    onPositionChanged: function(mouse) {
                        if (pressed) {
                            root.x += mouse.x - root.dragStart.x
                            root.y += mouse.y - root.dragStart.y
                            root.clampToScreen()
                        }
                    }

                    onReleased: root.snapToEdge()
                }
            }
            
            // Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                color: "#E5E7EB"
            }
            
            // --- Center: Media controls ---
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 6
                
                // Microphone split button
                SplitDeviceButton {
                    isActive: backend ? backend.micEnabled : false
                    iconOn: "qrc:/res/icon/Turn_on_the_microphone.png"
                    iconOff: "qrc:/res/icon/mute_the_microphone.png"
                    devices: settingsBackend ? settingsBackend.microphones : []
                    selectedDeviceId: settingsBackend ? settingsBackend.selectedMicId : ""
                    onToggle: if (backend) backend.toggleMicrophone()
                    onDeviceSelected: function(deviceId) {
                        if (backend) {
                            backend.switchMicrophone(deviceId)
                        }
                        if (settingsBackend) {
                            settingsBackend.selectedMicId = deviceId
                        }
                    }
                    onOpenSettings: root.openSettingsDialog()
                }
                
                // Camera split button
                SplitDeviceButton {
                    isActive: backend ? backend.camEnabled : false
                    iconOn: "qrc:/res/icon/video.png"
                    iconOff: "qrc:/res/icon/close_video.png"
                    devices: settingsBackend ? settingsBackend.cameras : []
                    selectedDeviceId: settingsBackend ? settingsBackend.selectedCameraId : ""
                    onToggle: if (backend) backend.toggleCamera()
                    onDeviceSelected: function(deviceId) {
                        if (backend) {
                            backend.switchCamera(deviceId)
                        }
                        if (settingsBackend) {
                            settingsBackend.selectedCameraId = deviceId
                        }
                    }
                    onOpenSettings: root.openSettingsDialog()
                }
                
                // Separator
                Rectangle {
                    width: 1
                    height: 20
                    color: "#E5E7EB"
                }
                
                IconButton {
                    iconSource: "qrc:/res/icon/set_up.png"
                    onClicked: root.openSettingsDialog()
                }
            }
            
            // Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                color: "#E5E7EB"
            }
            
            // --- Right: End Share button ---
            IconButton {
                iconSource: "qrc:/res/icon/screen-share-off.png"
                onClicked: root.stopSharingClicked()
            }
        }
    }
    
    // --- Icon Button Component ---
    component IconButton: Button {
        id: iconBtn
        property string iconSource: ""
        
        implicitWidth: 36
        implicitHeight: 36
        
        background: Rectangle {
            color: "transparent"
            radius: 18
        }
        
        contentItem: Item {
            Image {
                anchors.centerIn: parent
                source: iconBtn.iconSource
                sourceSize.width: 20
                sourceSize.height: 20
            }
            
            Rectangle {
                anchors.fill: parent
                radius: 18
                color: "#000000"
                opacity: iconBtn.hovered ? 0.06 : 0
            }
        }
        
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: function(mouse) { mouse.accepted = false }
        }
    }

    // --- Split Device Button Component ---
    component SplitDeviceButton: Item {
        id: splitBtn
        
        property bool isActive: true
        property string iconOn: ""
        property string iconOff: ""
        property var devices: []
        property string selectedDeviceId: ""
        
        signal toggle()
        signal deviceSelected(string deviceId)
        signal openSettings()
        
        implicitWidth: 64
        implicitHeight: 32
        
        Rectangle {
            id: splitContainer
            anchors.fill: parent
            radius: 10
            color: splitBtn.isActive ? "#FFFFFF" : "#FEF2F2"
            border.width: 1
            border.color: splitBtn.isActive ? "#E5E7EB" : "#FEE2E2"
            clip: true
            
            Row {
                anchors.fill: parent
                spacing: 0
                
                Rectangle {
                    id: mainButton
                    width: 40
                    height: parent.height
                    color: mainArea.containsMouse
                        ? (splitBtn.isActive ? "#F3F4F6" : "#FEE2E2")
                        : "transparent"
                    
                    Image {
                        anchors.centerIn: parent
                        source: splitBtn.isActive ? splitBtn.iconOn : splitBtn.iconOff
                        sourceSize.width: 18
                        sourceSize.height: 18
                    }
                    
                    MouseArea {
                        id: mainArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onPressed: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                splitBtn.openMenu()
                                mouse.accepted = true
                            }
                        }
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.LeftButton) {
                                if (menuPopup.visible) {
                                    menuPopup.close()
                                    return
                                }
                                splitBtn.toggle()
                            }
                        }
                    }
                }
                
                Rectangle {
                    width: 1
                    height: parent.height - 10
                    anchors.verticalCenter: parent.verticalCenter
                    color: splitBtn.isActive ? "#E5E7EB" : "#FECACA"
                }
                
                Rectangle {
                    id: dropdownButton
                    width: 23
                    height: parent.height
                    color: dropdownArea.containsMouse
                        ? (splitBtn.isActive ? "#F3F4F6" : "#FEE2E2")
                        : "transparent"
                    
                    Image {
                        anchors.centerIn: parent
                        source: menuPopup.visible ? "qrc:/res/icon/chevron-down.png" : "qrc:/res/icon/chevron-up.png"
                        sourceSize.width: 9
                        sourceSize.height: 9
                        opacity: splitBtn.isActive ? 0.6 : 0.8
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
                            splitBtn.openMenu()
                        }
                    }
                }
            }
        }

        function openMenu() {
            if (!menuPopup) return

            var geo = root.screenGeometry()
            var localPos = splitBtn.mapToItem(root.contentItem, 0, 0)
            var globalX = root.x + localPos.x
            var globalY = root.y + localPos.y
            var popupWidth = menuPopup.width
            var popupHeight = menuPopup.implicitHeight > 0 ? menuPopup.implicitHeight : menuPopup.height
            var spaceAbove = globalY - geo.y
            var spaceBelow = geo.y + geo.height - (globalY + splitBtn.height)
            var targetX = 0
            var targetY = splitBtn.height + 8
            var globalTargetX = root.x + localPos.x + targetX
            var globalTargetY = root.y + localPos.y + targetY

            if (spaceBelow < popupHeight + 8 && spaceAbove >= popupHeight + 8) {
                targetY = -popupHeight - 8
                globalTargetY = root.y + localPos.y + targetY
            }

            var clampedGlobalX = Math.min(Math.max(globalTargetX, geo.x),
                                          geo.x + geo.width - popupWidth)
            targetX += (clampedGlobalX - globalTargetX)

            menuPopup.x = targetX
            menuPopup.y = targetY
            menuPopup.open()
        }

        Popup {
            id: menuPopup
            popupType: Popup.Window
            x: 0
            width: 220
            padding: 0
            closePolicy: Popup.CloseOnEscape
            
            background: Rectangle {
                color: "#FFFFFF"
                radius: 12
                border.width: 1
                border.color: "#E5E7EB"
                layer.enabled: true
                layer.effect: null
            }
            
            contentItem: Column {
                padding: 8
                spacing: 4
                
                Text {
                    text: "选择设备"
                    color: "#9CA3AF"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 0.5
                    leftPadding: 8
                    topPadding: 4
                }
                
                Repeater {
                    model: splitBtn.devices
                    
                    Rectangle {
                        width: 204
                        height: 34
                        radius: 8
                        color: modelData.id === splitBtn.selectedDeviceId
                            ? "#EFF6FF"
                            : (deviceItemArea.containsMouse ? "#F9FAFB" : "transparent")
                        
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

                Rectangle {
                    width: 204
                    height: 1
                    color: "#F3F4F6"
                }

                Rectangle {
                    width: 204
                    height: 34
                    radius: 8
                    color: settingsItemArea.containsMouse ? "#F9FAFB" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        spacing: 8

                        Image {
                            source: "qrc:/res/icon/set_up.png"
                            sourceSize.width: 12
                            sourceSize.height: 12
                            opacity: 0.6
                        }

                        Text {
                            text: "音视频设置..."
                            color: "#6B7280"
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        id: settingsItemArea
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

    SettingsWindow {
        id: settingsDialog
    }


    Loader {
        id: cameraThumbnailLoader
        active: backend && backend.shareMode && backend.shareMode.isActive &&
                backend.camEnabled
        sourceComponent: CameraThumbnail {
            backend: root.backend
            visible: true
        }
    }
}
