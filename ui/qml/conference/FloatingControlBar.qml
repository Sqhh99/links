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
    width: 500
    height: 48
    visible: true
    color: "transparent"
    transientParent: null
    
    // Position at top center of primary screen
    x: Screen.width / 2 - width / 2
    y: 16
    
    property ConferenceBackend backend
    property var settingsBackend: null
    property bool isGuest: false
    property point dragStart: Qt.point(0, 0)
    property int edgePadding: 0
    property string shareUrl: {
        if (!backend || !backend.meetingNo || backend.meetingNo.length === 0) {
            return ""
        }
        if (!settingsBackend || !settingsBackend.apiUrl || settingsBackend.apiUrl.length === 0) {
            return ""
        }
        var base = settingsBackend.apiUrl.replace(/\/+$/, "")
        return base + "/join?meetingNo=" + encodeURIComponent(backend.meetingNo)
    }
    
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

    function formatLatency(value) {
        return value >= 0 ? value + " ms" : "--"
    }

    function formatBitrate(value) {
        return value >= 0 ? value + " kbps" : "--"
    }

    function formatPacketLoss(value) {
        return value >= 0 ? Number(value).toFixed(1) + "%" : "--"
    }

    function formatFps(value) {
        return value >= 0 ? Number(value).toFixed(1) + " FPS" : "--"
    }

    function closeInfoPopups(exceptPopup) {
        if (networkStatsPopup.visible && exceptPopup !== networkStatsPopup) {
            networkStatsPopup.close()
        }
        if (sharePopup.visible && exceptPopup !== sharePopup) {
            sharePopup.close()
        }
    }

    function openAnchoredPopup(popup, anchorItem) {
        if (!popup || !anchorItem) return

        var geo = screenGeometry()
        var localPos = anchorItem.mapToItem(root.contentItem, 0, 0)
        var popupWidth = popup.width > 0 ? popup.width : popup.implicitWidth
        var popupHeight = popup.implicitHeight > 0 ? popup.implicitHeight : popup.height
        var offset = 8

        var targetX = localPos.x + Math.round((anchorItem.width - popupWidth) / 2)
        var targetY = localPos.y + anchorItem.height + offset

        var anchorGlobalTop = root.y + localPos.y
        var anchorGlobalBottom = anchorGlobalTop + anchorItem.height
        var spaceAbove = anchorGlobalTop - geo.y
        var spaceBelow = geo.y + geo.height - anchorGlobalBottom
        if (spaceBelow < popupHeight + offset && spaceAbove >= popupHeight + offset) {
            targetY = localPos.y - popupHeight - offset
        }

        var globalTargetX = root.x + targetX
        var clampedGlobalX = Math.min(Math.max(globalTargetX, geo.x), geo.x + geo.width - popupWidth)
        targetX += (clampedGlobalX - globalTargetX)

        popup.x = targetX
        popup.y = targetY
        popup.open()
    }

    function toggleInfoPopup(popup, anchorItem) {
        if (!popup || !anchorItem) return
        if (popup.visible) {
            popup.close()
            return
        }
        closeInfoPopups(popup)
        openAnchoredPopup(popup, anchorItem)
    }
    
    // Main container with frosted glass effect
    Rectangle {
        id: container
        anchors.fill: parent
        radius: 24
        color: Theme.isDark ? Qt.rgba(40/255, 40/255, 45/255, 0.95) : Qt.rgba(255, 255, 255, 0.95)
        border.width: 1
        border.color: Theme.borderColor
        
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
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                    
                    Text {
                        text: backend && backend.shareMode ? backend.shareMode.formattedTime : "00:00"
                        color: Theme.textMuted
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
                color: Theme.separatorColor
            }
            
            // --- Center: Media controls ---
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 6
                
                // Microphone split button
                SplitDeviceButton {
                    visible: !root.isGuest
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
                    visible: !root.isGuest
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
                    visible: !root.isGuest
                    width: 1
                    height: 20
                    color: Theme.separatorColor
                }

                IconButton {
                    id: networkStatusBtn
                    iconSource: "qrc:/res/icon/file-chart-column-increasing.png"
                    toolTipText: "网络状态"
                    onClicked: root.toggleInfoPopup(networkStatsPopup, networkStatusBtn)
                }

                IconButton {
                    id: shareBtn
                    iconSource: "qrc:/res/icon/square-arrow-out-up-right.png"
                    toolTipText: "分享会议"
                    visible: backend && backend.meetingNo && backend.meetingNo.length > 0
                    onClicked: root.toggleInfoPopup(sharePopup, shareBtn)
                }
                
                IconButton {
                    iconSource: "qrc:/res/icon/set_up.png"
                    toolTipText: "设置"
                    onClicked: root.openSettingsDialog()
                }
            }
            
            // Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                color: Theme.separatorColor
            }
            
            // --- Right: End Share button ---
            IconButton {
                iconSource: "qrc:/res/icon/screen-share-off.png"
                toolTipText: "结束共享"
                onClicked: root.stopSharingClicked()
            }
        }
    }

    Popup {
        id: sharePopup
        popupType: Popup.Window
        x: 0
        y: 0
        width: 340
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        onOpened: {
            if (backend && backend.shareMode && sharePopup.window) {
                backend.shareMode.excludeFromCapture(sharePopup.window)
            }
        }

        background: Rectangle {
            color: Theme.popupBackground
            radius: 12
            border.color: Theme.popupBorder
            border.width: 1
            layer.enabled: true
            layer.effect: null
        }

        contentItem: ColumnLayout {
            spacing: 0

            Text {
                text: "分享会议"
                color: Theme.textPrimary
                font.pixelSize: 14
                font.weight: Font.DemiBold
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 12
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.separatorColor }

            Rectangle {
                Layout.fillWidth: true
                Layout.margins: 12
                Layout.bottomMargin: 6
                implicitHeight: 44
                radius: 8
                color: Theme.cardBackground
                border.color: Theme.borderColor
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 8
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Text {
                            text: "会议号"
                            color: Theme.textMuted
                            font.pixelSize: 10
                        }
                        Text {
                            text: backend ? backend.meetingNo : ""
                            color: Theme.textPrimary
                            font.pixelSize: 13
                            font.weight: Font.Medium
                        }
                    }

                    Rectangle {
                        implicitWidth: 30
                        implicitHeight: 30
                        radius: 6
                        color: copyNoArea.containsMouse ? Theme.hoverBackground : "transparent"
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                        Image {
                            anchors.centerIn: parent
                            source: "qrc:/res/icon/copy.png"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            opacity: copyNoArea.containsMouse ? 1.0 : 0.5
                        }

                        MouseArea {
                            id: copyNoArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                shareCopyHelper.text = backend ? backend.meetingNo : ""
                                shareCopyHelper.selectAll()
                                shareCopyHelper.copy()
                                copyNoBubble.show()
                            }
                        }

                        Rectangle {
                            id: copyNoBubble
                            anchors.right: parent.right
                            anchors.bottom: parent.top
                            anchors.bottomMargin: 4
                            width: 52
                            height: 24
                            radius: 6
                            color: "#111827"
                            visible: false
                            opacity: 0

                            function show() {
                                visible = true
                                opacity = 1
                                copyNoBubbleTimer.restart()
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "已复制"
                                color: "#FFFFFF"
                                font.pixelSize: 10
                            }

                            Timer {
                                id: copyNoBubbleTimer
                                interval: 1200
                                onTriggered: copyNoBubbleAnim.start()
                            }

                            NumberAnimation on opacity {
                                id: copyNoBubbleAnim
                                running: false
                                to: 0
                                duration: 300
                                onFinished: copyNoBubble.visible = false
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: 12
                implicitHeight: 44
                radius: 8
                color: Theme.cardBackground
                border.color: Theme.borderColor
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 8
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Text {
                            text: "会议链接"
                            color: Theme.textMuted
                            font.pixelSize: 10
                        }
                        Text {
                            text: root.shareUrl
                            color: Theme.accentColor
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        implicitWidth: 30
                        implicitHeight: 30
                        radius: 6
                        color: copyLinkArea.containsMouse ? Theme.hoverBackground : "transparent"
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                        Image {
                            anchors.centerIn: parent
                            source: "qrc:/res/icon/copy.png"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            opacity: copyLinkArea.containsMouse ? 1.0 : 0.5
                        }

                        MouseArea {
                            id: copyLinkArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                shareCopyHelper.text = root.shareUrl
                                shareCopyHelper.selectAll()
                                shareCopyHelper.copy()
                                copyLinkBubble.show()
                            }
                        }

                        Rectangle {
                            id: copyLinkBubble
                            anchors.right: parent.right
                            anchors.bottom: parent.top
                            anchors.bottomMargin: 4
                            width: 52
                            height: 24
                            radius: 6
                            color: "#111827"
                            visible: false
                            opacity: 0

                            function show() {
                                visible = true
                                opacity = 1
                                copyLinkBubbleTimer.restart()
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "已复制"
                                color: "#FFFFFF"
                                font.pixelSize: 10
                            }

                            Timer {
                                id: copyLinkBubbleTimer
                                interval: 1200
                                onTriggered: copyLinkBubbleAnim.start()
                            }

                            NumberAnimation on opacity {
                                id: copyLinkBubbleAnim
                                running: false
                                to: 0
                                duration: 300
                                onFinished: copyLinkBubble.visible = false
                            }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: networkStatsPopup
        popupType: Popup.Window
        x: 0
        y: 0
        width: 480
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        onOpened: {
            if (backend && backend.shareMode && networkStatsPopup.window) {
                backend.shareMode.excludeFromCapture(networkStatsPopup.window)
            }
        }

        background: Rectangle {
            color: Theme.popupBackground
            radius: 12
            border.color: Theme.popupBorder
            border.width: 1
        }

        contentItem: RowLayout {
            spacing: 0

            GridLayout {
                columns: 2
                rowSpacing: 8
                columnSpacing: 12
                Layout.fillWidth: true
                Layout.margins: 14

                Text { text: "连接状态"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: backend ? backend.connectionStatus : "--"; color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "连接质量"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: backend ? backend.networkQualityText : "检测中"; color: backend ? backend.networkQualityColor : Theme.textMuted; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "会议时长"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: backend ? backend.meetingDuration : "00:00:00"; color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Rectangle { Layout.columnSpan: 2; Layout.fillWidth: true; height: 1; color: Theme.separatorColor }

                Text { text: "延迟 RTT"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: root.formatLatency(backend ? backend.networkRttMs : -1); color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "抖动"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: root.formatLatency(backend ? backend.networkJitterMs : -1); color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "丢包率"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: root.formatPacketLoss(backend ? backend.networkPacketLossPercent : -1); color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "上行码率"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: root.formatBitrate(backend ? backend.networkUplinkKbps : -1); color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "下行码率"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: root.formatBitrate(backend ? backend.networkDownlinkKbps : -1); color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }
            }

            Rectangle { width: 1; Layout.fillHeight: true; Layout.topMargin: 10; Layout.bottomMargin: 10; color: Theme.separatorColor }

            GridLayout {
                columns: 2
                rowSpacing: 8
                columnSpacing: 12
                Layout.fillWidth: true
                Layout.margins: 14

                Text { text: "传输协议"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: (backend && backend.transportProtocol.length > 0) ? backend.transportProtocol : "--"; color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "可用带宽"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: root.formatBitrate(backend ? backend.availableSendBandwidth : -1); color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Rectangle { Layout.columnSpan: 2; Layout.fillWidth: true; height: 1; color: Theme.separatorColor }

                Text { text: "音频编码"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: (backend && backend.audioCodec.length > 0) ? backend.audioCodec : "--"; color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "视频编码"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: (backend && backend.videoCodec.length > 0) ? backend.videoCodec : "--"; color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Rectangle { Layout.columnSpan: 2; Layout.fillWidth: true; height: 1; color: Theme.separatorColor }

                Text { text: "视频分辨率"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: (backend && backend.videoResolution.length > 0) ? backend.videoResolution : "--"; color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }

                Text { text: "视频帧率"; color: Theme.textMuted; font.pixelSize: 11 }
                Text { text: root.formatFps(backend ? backend.videoFps : -1); color: Theme.textPrimary; font.pixelSize: 11; font.weight: Font.Medium }
            }
        }
    }

    TextEdit {
        id: shareCopyHelper
        visible: false
    }
    
    // --- Icon Button Component ---
    component IconButton: Button {
        id: iconBtn
        property string iconSource: ""
        property string toolTipText: ""
        
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

        ToolTip.visible: toolTipText.length > 0 && hovered
        ToolTip.text: toolTipText
        ToolTip.delay: 500
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
            color: splitBtn.isActive ? "transparent" : Theme.accentLight
            border.width: 1
            border.color: splitBtn.isActive ? Theme.borderColor : Theme.borderAccent
            clip: true
            
            Row {
                anchors.fill: parent
                spacing: 0
                
                Rectangle {
                    id: mainButton
                    width: 40
                    height: parent.height
                    color: mainArea.containsMouse
                        ? Theme.hoverBackground
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
                    color: splitBtn.isActive ? Theme.separatorColor : Theme.borderAccent
                }
                
                Rectangle {
                    id: dropdownButton
                    width: 23
                    height: parent.height
                    color: dropdownArea.containsMouse
                        ? Theme.hoverBackground
                        : "transparent"
                    
                    Image {
                        anchors.centerIn: parent
                        source: menuPopup.visible ? "qrc:/res/icon/chevron-down.png" : "qrc:/res/icon/chevron-up.png"
                        sourceSize.width: 9
                        sourceSize.height: 9
                        opacity: Theme.iconOpacity
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
                color: Theme.popupBackground
                radius: 12
                border.width: 1
                border.color: Theme.popupBorder
                layer.enabled: true
                layer.effect: null
            }
            
            contentItem: Column {
                padding: 8
                spacing: 4
                
                Text {
                    text: "选择设备"
                    color: Theme.textMuted
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
                            ? Theme.activeBackground
                            : (deviceItemArea.containsMouse ? Theme.hoverBackground : "transparent")
                        
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
                            
                            Text {
                                visible: modelData.id === splitBtn.selectedDeviceId
                                text: "✓"
                                color: Theme.accentColor
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
                    color: Theme.separatorColor
                }

                Rectangle {
                    width: 204
                    height: 34
                    radius: 8
                    color: settingsItemArea.containsMouse ? Theme.hoverBackground : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        spacing: 8

                        Image {
                            source: "qrc:/res/icon/set_up.png"
                            sourceSize.width: 12
                            sourceSize.height: 12
                            opacity: Theme.iconOpacity
                        }

                        Text {
                            text: "音视频设置..."
                            color: Theme.textSecondary
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
        onSettingsSaved: {
            if (backend) backend.applyAudioSettings()
        }
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
