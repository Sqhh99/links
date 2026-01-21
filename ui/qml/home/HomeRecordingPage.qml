import QtQuick
import QtQuick.Layouts
import Links

Item {
    id: root

    property bool isGuest: true

    signal requestLogin()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        ColumnLayout {
            spacing: 6

            Text {
                text: "录制"
                color: "#111827"
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Text {
                text: root.isGuest ? "游客模式可查看本地录制" : "管理本地与云端录制"
                color: "#6B7280"
                font.pixelSize: 12
            }
        }

        ColumnLayout {
            spacing: 10

            Text {
                text: "本地录制"
                color: "#111827"
                font.pixelSize: 13
                font.weight: Font.Medium
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                radius: 12
                color: "#FFFFFF"
                border.color: "#E5E7EB"
                border.width: 1

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        text: "暂无本地录制"
                        color: "#6B7280"
                        font.pixelSize: 12
                    }

                    LinkButton {
                        text: "查看录制文件夹"
                    }
                }
            }
        }

        Item { height: 12 }

        ColumnLayout {
            spacing: 10

            Text {
                text: "云录制"
                color: "#111827"
                font.pixelSize: 13
                font.weight: Font.Medium
            }

            LoginPromptCard {
                Layout.fillWidth: true
                title: "登录以启用云录制"
                message: "支持云端存储、共享与回放管理。"
                showSecondary: false
                primaryWidth: 200
                centerPrimary: true
                onPrimaryClicked: root.requestLogin()
                onSecondaryClicked: {}
            }
        }

        Item { Layout.fillHeight: true }
    }
}
