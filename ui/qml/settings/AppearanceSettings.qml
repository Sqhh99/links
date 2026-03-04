import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

ScrollView {
    id: root

    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: parent.width
        spacing: 24

        Text {
            text: "主题"
            color: Theme.textSecondary
            font.pixelSize: 13
            font.weight: Font.Medium
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            // Light Theme Card
            Rectangle {
                Layout.preferredWidth: 160
                Layout.preferredHeight: 120
                radius: 12
                color: ThemeManager.currentTheme === "light" ? "#EFF6FF" : Theme.cardBackground
                border.color: ThemeManager.currentTheme === "light" ? Theme.accentColor : Theme.borderLight
                border.width: ThemeManager.currentTheme === "light" ? 2 : 1

                Behavior on border.color { ColorAnimation { duration: 150 } }
                Behavior on color { ColorAnimation { duration: 150 } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Light theme preview
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: "#FFFFFF"
                        border.color: "#E5E7EB"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4

                            Rectangle {
                                Layout.preferredWidth: 24
                                Layout.fillHeight: true
                                radius: 4
                                color: "#F3F4F6"
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 4
                                color: "#FAFAFA"

                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: 4
                                    height: 6
                                    radius: 2
                                    color: "#E5E7EB"
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        // Radio indicator
                        Rectangle {
                            width: 16
                            height: 16
                            radius: 8
                            color: "transparent"
                            border.color: ThemeManager.currentTheme === "light" ? Theme.accentColor : Theme.borderColor
                            border.width: 1.5

                            Rectangle {
                                anchors.centerIn: parent
                                width: 8
                                height: 8
                                radius: 4
                                color: Theme.accentColor
                                visible: ThemeManager.currentTheme === "light"
                            }
                        }

                        Text {
                            text: "浅色"
                            color: Theme.textPrimary
                            font.pixelSize: 13
                            font.weight: ThemeManager.currentTheme === "light" ? Font.Bold : Font.Medium
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: ThemeManager.setTheme("light")
                }
            }

            // Dark Theme Card
            Rectangle {
                Layout.preferredWidth: 160
                Layout.preferredHeight: 120
                radius: 12
                color: ThemeManager.currentTheme === "dark" ? "#2A3A5C" : Theme.cardBackground
                border.color: ThemeManager.currentTheme === "dark" ? Theme.accentColor : Theme.borderLight
                border.width: ThemeManager.currentTheme === "dark" ? 2 : 1

                Behavior on border.color { ColorAnimation { duration: 150 } }
                Behavior on color { ColorAnimation { duration: 150 } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Dark theme preview
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: "#1E1E2E"
                        border.color: "#3A3A4E"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4

                            Rectangle {
                                Layout.preferredWidth: 24
                                Layout.fillHeight: true
                                radius: 4
                                color: "#252538"
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 4
                                color: "#2A2A3C"

                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: 4
                                    height: 6
                                    radius: 2
                                    color: "#3A3A4E"
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        // Radio indicator
                        Rectangle {
                            width: 16
                            height: 16
                            radius: 8
                            color: "transparent"
                            border.color: ThemeManager.currentTheme === "dark" ? Theme.accentColor : Theme.borderColor
                            border.width: 1.5

                            Rectangle {
                                anchors.centerIn: parent
                                width: 8
                                height: 8
                                radius: 4
                                color: Theme.accentColor
                                visible: ThemeManager.currentTheme === "dark"
                            }
                        }

                        Text {
                            text: "深色"
                            color: Theme.textPrimary
                            font.pixelSize: 13
                            font.weight: ThemeManager.currentTheme === "dark" ? Font.Bold : Font.Medium
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: ThemeManager.setTheme("dark")
                }
            }
        }

        Text {
            text: "选择主题后将立即应用到所有界面"
            color: Theme.textHint
            font.pixelSize: 11
        }

        Item { Layout.fillHeight: true }
    }
}
