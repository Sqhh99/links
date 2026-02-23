import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root

    height: 52
    color: "#FFFFFF" // Light theme
    radius: 12
    clip: true

    property var targetWindow
    property ConferenceBackend backend
    property string shareUrl: ""

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

        // --- Left Section: Room Name, ID, Share Button ---
        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            // Room info
            ColumnLayout {
                spacing: 0

                Text {
                    text: backend ? backend.roomName : "产品设计评审会"
                    color: "#111827"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }

                Text {
                    text: backend ? (backend.meetingNo && backend.meetingNo.length > 0 ? "会议号: " + backend.meetingNo : "房间: " + backend.roomName) : "会议"
                    color: "#9CA3AF"
                    font.pixelSize: 10
                }
            }

            // Share button
            Rectangle {
                id: shareBtn
                implicitWidth: 32
                implicitHeight: 32
                radius: 6
                color: shareBtnArea.containsMouse ? "#F3F4F6" : "transparent"
                visible: backend && backend.meetingNo && backend.meetingNo.length > 0

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/res/icon/square-arrow-out-up-right.png"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    opacity: 0.7
                }

                MouseArea {
                    id: shareBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (sharePopup.visible)
                            sharePopup.close()
                        else
                            sharePopup.open()
                    }
                }

                ToolTip.visible: shareBtnArea.containsMouse && !sharePopup.visible
                ToolTip.text: "分享会议"
                ToolTip.delay: 500

                Popup {
                    id: sharePopup
                    y: shareBtn.height + 8
                    x: 0
                    width: 340
                    padding: 0
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

                    background: Rectangle {
                        color: "#FFFFFF"
                        radius: 12
                        border.color: "#E5E7EB"
                        border.width: 1

                        layer.enabled: true
                        layer.effect: null
                    }

                    contentItem: ColumnLayout {
                        spacing: 0

                        // Header
                        Text {
                            text: "分享会议"
                            color: "#111827"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            Layout.topMargin: 16
                            Layout.leftMargin: 16
                            Layout.bottomMargin: 12
                        }

                        // Divider
                        Rectangle { Layout.fillWidth: true; height: 1; color: "#F3F4F6" }

                        // Meeting number row
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.margins: 12
                            Layout.bottomMargin: 6
                            implicitHeight: 44
                            radius: 8
                            color: "#F9FAFB"
                            border.color: "#E5E7EB"
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
                                        color: "#9CA3AF"
                                        font.pixelSize: 10
                                    }
                                    Text {
                                        text: backend ? backend.meetingNo : ""
                                        color: "#111827"
                                        font.pixelSize: 13
                                        font.weight: Font.Medium
                                    }
                                }

                                Rectangle {
                                    implicitWidth: 30
                                    implicitHeight: 30
                                    radius: 6
                                    color: copyNoArea.containsMouse ? "#E5E7EB" : "transparent"
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
                                            shareCopyHelper.text = backend.meetingNo
                                            shareCopyHelper.selectAll()
                                            shareCopyHelper.copy()
                                        }
                                    }
                                }
                            }
                        }

                        // Meeting link row
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Layout.bottomMargin: 12
                            implicitHeight: 44
                            radius: 8
                            color: "#F9FAFB"
                            border.color: "#E5E7EB"
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
                                        color: "#9CA3AF"
                                        font.pixelSize: 10
                                    }
                                    Text {
                                        text: root.shareUrl
                                        color: "#2563EB"
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
                                    color: copyLinkArea.containsMouse ? "#E5E7EB" : "transparent"
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
                                        }
                                    }
                                }
                            }
                        }


                    }

                    // Hidden TextEdit for clipboard
                    TextEdit {
                        id: shareCopyHelper
                        visible: false
                    }
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
                                source: "qrc:/res/icon/user.png" // Using as grid icon
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
