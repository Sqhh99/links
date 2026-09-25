import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

ScrollView {
    id: root

    contentWidth: availableWidth
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    // Theme preview card; preview colors are literal so each card always
    // shows its own theme regardless of the active one.
    component ThemeCard: Rectangle {
        id: card

        property string themeKey: "light"
        property string label: ""
        property string iconName: "sun"
        property color previewPage: "#F4F7FC"
        property color previewRail: "#FFFFFF"
        property color previewCard: "#FFFFFF"
        property color previewLine: "#E3E8F0"
        readonly property bool selected: ThemeManager.currentTheme === card.themeKey

        Layout.fillWidth: true
        Layout.preferredHeight: 132
        radius: Theme.radiusMd
        color: Theme.cardBackground
        border.color: selected ? Theme.brand : (cardHover.hovered ? Theme.borderColor : Theme.borderLight)
        border.width: selected ? 2 : 1

        Behavior on border.color { ColorAnimation { duration: 150 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            // Mini window preview
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 6
                color: card.previewPage
                border.color: card.previewLine
                clip: true

                Rectangle {
                    id: previewRail
                    width: 18
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.margins: 1
                    color: card.previewRail

                    Column {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 6
                        spacing: 5

                        Rectangle { width: 8; height: 8; radius: 4; color: "#1F6FFF" }
                        Rectangle { width: 8; height: 8; radius: 2; color: card.previewLine }
                        Rectangle { width: 8; height: 8; radius: 2; color: card.previewLine }
                    }
                }

                Grid {
                    anchors.left: previewRail.right
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    columns: 2
                    spacing: 6

                    Repeater {
                        model: 4
                        Rectangle { width: 16; height: 16; radius: 5; color: "#1F6FFF" }
                    }
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 6
                    width: parent.width * 0.34
                    radius: 3
                    color: card.previewCard

                    Column {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 4

                        Rectangle { width: parent.width * 0.5; height: 5; radius: 2; color: card.previewLine }
                        Rectangle { width: parent.width; height: 3; radius: 1; color: card.previewLine }
                        Rectangle { width: parent.width * 0.8; height: 3; radius: 1; color: card.previewLine }
                        Rectangle { width: parent.width; height: 3; radius: 1; color: card.previewLine }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Icon {
                    name: card.iconName
                    size: 15
                    color: card.selected ? Theme.brand : Theme.iconSecondary
                }

                Text {
                    text: card.label
                    color: card.selected ? Theme.brand : Theme.textPrimary
                    font.pixelSize: 13
                    font.weight: card.selected ? Font.DemiBold : Font.Normal
                    Layout.fillWidth: true
                }

                Rectangle {
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: card.selected ? Theme.brand : "transparent"
                    border.color: card.selected ? Theme.brand : Theme.checkboxBorder
                    border.width: 1.5

                    Icon {
                        anchors.centerIn: parent
                        visible: card.selected
                        name: "check"
                        size: 10
                        color: "#FFFFFF"
                    }
                }
            }
        }

        HoverHandler {
            id: cardHover
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onTapped: ThemeManager.setTheme(card.themeKey)
        }
    }

    ColumnLayout {
        width: root.availableWidth
        spacing: 20

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "主题"
                color: Theme.textTertiary
                font.pixelSize: 12
                font.weight: Font.DemiBold
                Layout.leftMargin: 4
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ThemeCard {
                    themeKey: "light"
                    label: "浅色"
                    iconName: "sun"
                    previewPage: "#F4F7FC"
                    previewRail: "#FFFFFF"
                    previewCard: "#FFFFFF"
                    previewLine: "#E3E8F0"
                }

                ThemeCard {
                    themeKey: "dark"
                    label: "深色"
                    iconName: "moon"
                    previewPage: "#14161B"
                    previewRail: "#181B21"
                    previewCard: "#1C1F26"
                    previewLine: "#2E323C"
                }
            }

            Text {
                text: "选择主题后将立即应用到所有界面"
                color: Theme.textMuted
                font.pixelSize: 11
                Layout.leftMargin: 4
            }
        }

        SettingsSection {
            title: "会议界面"

            SettingsRow {
                label: "自动隐藏顶部栏和底部栏"
                description: "会议中无操作时自动隐藏，移动到边缘可再次显示"
                divider: false

                ToggleSwitch {
                    checked: AppearanceManager.autoHideConferenceChrome
                    onToggled: AppearanceManager.setAutoHideConferenceChrome(checked)
                }
            }
        }
    }
}
