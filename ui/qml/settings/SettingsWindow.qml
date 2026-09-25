import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Links
import Links.Backend 1.0

Window {
    id: root
    
    width: 680
    height: 500
    visible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Window
    title: "设置"
    
    // Center on screen when first shown
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2

    property point dragLastGlobal: Qt.point(0, 0)
    property bool dragging: false
    
    function open() {
        // Center on screen
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2
        visible = true
        raise()
        requestActivate()
    }
    
    function close() {
        visible = false
    }
    
    // Signal emitted when settings are saved, so active components can re-apply
    signal settingsSaved()
    
    // Backend integration
    SettingsBackend {
        id: backend
        
        Component.onCompleted: {
            refreshDevices()
        }
        
        onAccepted: {
            root.settingsSaved()
            root.close()
        }
        onRejected: root.close()
    }
    
    readonly property var pages: [
        { title: "音频", icon: "mic", description: "麦克风、扬声器与音频处理" },
        { title: "视频", icon: "video", description: "摄像头与视频画质" },
        { title: "网络", icon: "wifi", description: "服务器连接地址" },
        { title: "界面", icon: "palette", description: "主题与会议界面偏好" }
    ]

    Rectangle {
        id: frame
        anchors.fill: parent
        color: Theme.windowBackground
        radius: 12
        border.color: Theme.windowBorder
        border.width: 1
        antialiasing: true

        Behavior on color { ColorAnimation { duration: 200 } }

        // Drag the window by its header strip (the close button sits on top)
        MouseArea {
            id: dragArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 56
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

        RowLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // Left navigation
            Rectangle {
                Layout.preferredWidth: 168
                Layout.fillHeight: true
                color: Theme.sidebarBackground
                topLeftRadius: 11
                bottomLeftRadius: 11

                Behavior on color { ColorAnimation { duration: 200 } }

                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: Theme.sidebarBorder
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: 18
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.bottomMargin: 12
                    spacing: 2

                    Text {
                        text: "设置"
                        color: Theme.textPrimary
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        Layout.leftMargin: 8
                        Layout.bottomMargin: 14
                    }

                    Repeater {
                        model: root.pages

                        NavButton {
                            required property int index
                            required property var modelData
                            text: modelData.title
                            iconName: modelData.icon
                            active: pageStack.currentIndex === index
                            onClicked: pageStack.currentIndex = index
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // Right content
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Header
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 12
                    Layout.topMargin: 14
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 2

                        // The Texts must fill: a layout's maximum width comes from its
                        // children, so fillWidth on this column alone would not push
                        // the close button to the right edge.
                        Text {
                            Layout.fillWidth: true
                            text: root.pages[pageStack.currentIndex].title
                            color: Theme.textPrimary
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.pages[pageStack.currentIndex].description
                            color: Theme.textMuted
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    IconButton {
                        Layout.alignment: Qt.AlignTop
                        iconName: "x"
                        iconSize: 16
                        onClicked: backend.cancel()
                    }
                }

                // Content pages
                StackLayout {
                    id: pageStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.topMargin: 16
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

                    AppearanceSettings {
                    }
                }

                // Footer
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.separatorColor
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.topMargin: 12
                    Layout.bottomMargin: 12
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    SecondaryButton {
                        text: "取消"
                        implicitHeight: 34
                        implicitWidth: 80
                        onClicked: backend.cancel()
                    }

                    PrimaryButton {
                        text: "保存"
                        implicitHeight: 34
                        implicitWidth: 80
                        onClicked: backend.save()
                    }
                }
            }
        }
    }

    // Navigation button component
    component NavButton: Button {
        id: navBtn
        property bool active: false
        property string iconName: ""

        Layout.fillWidth: true
        implicitHeight: 36
        checkable: true
        checked: active

        background: Rectangle {
            radius: 8
            color: navBtn.active ? Theme.activeBackground
                                 : (navBtn.hovered ? Theme.hoverBackground : Theme.hoverClear)

            Behavior on color { ColorAnimation { duration: 120 } }
        }

        contentItem: RowLayout {
            spacing: 10

            Icon {
                Layout.leftMargin: 4
                name: navBtn.iconName
                size: 16
                color: navBtn.active ? Theme.brand : Theme.iconSecondary
            }

            Text {
                text: navBtn.text
                color: navBtn.active ? Theme.brand : Theme.textSecondary
                font.pixelSize: 13
                font.weight: navBtn.active ? Font.DemiBold : Font.Normal
                Layout.fillWidth: true
            }
        }

        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }
}
