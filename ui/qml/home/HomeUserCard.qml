import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

Item {
    id: root

    property bool isGuest: true
    property string userName: "游客"
    property string subtitle: "已登录"
    property string avatarSource: "qrc:/res/icon/user.png"

    signal loginClicked()
    signal settingsRequested()
    signal logoutRequested()

    implicitHeight: 80

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "#FFFFFF"
        border.color: "#E5E7EB"
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Rectangle {
            width: 44
            height: 44
            radius: 22
            color: "#FFFFFF"
            border.color: "#E5E7EB"
            border.width: 1

            Image {
                anchors.centerIn: parent
                source: root.avatarSource
                sourceSize.width: 20
                sourceSize.height: 20
                opacity: root.isGuest ? 0.6 : 0.9
            }
        }

        ColumnLayout {
            spacing: 6
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: root.isGuest ? "游客" : root.userName
                    color: "#111827"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                LinkButton {
                    visible: root.isGuest
                    text: "去登录"
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: root.loginClicked()
                }
            }

            Text {
                visible: !root.isGuest
                text: root.subtitle
                color: "#6B7280"
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        IconButton {
            id: menuButton
            visible: !root.isGuest
            iconSource: "qrc:/res/icon/chevron-down.png"
            toolTipText: "账号"
            onClicked: accountMenu.popup(menuButton)
        }
    }

    Menu {
        id: accountMenu

        MenuItem {
            text: "账号设置"
            onTriggered: root.settingsRequested()
        }

        MenuItem {
            text: "退出登录"
            onTriggered: root.logoutRequested()
        }
    }
}
