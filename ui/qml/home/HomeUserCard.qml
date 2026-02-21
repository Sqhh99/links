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
    property bool suppressMenuToggleClick: false
    property bool closingFromToggle: false

    signal loginClicked()
    signal settingsRequested()
    signal switchUserRequested()
    signal logoutRequested()

    implicitHeight: 80

    function toggleAccountMenu() {
        if (suppressMenuToggleClick) {
            suppressMenuToggleClick = false
            return
        }

        if (accountPopup.opened) {
            closingFromToggle = true
            accountPopup.close()
            return
        }

        const overlay = Overlay.overlay
        if (!overlay) {
            accountPopup.open()
            return
        }

        const popupHeight = accountPopup.implicitHeight > 0 ? accountPopup.implicitHeight : accountPopup.height
        const buttonPos = menuButton.mapToItem(overlay, menuButton.width, 0)
        const desiredX = buttonPos.x + 8
        const desiredY = buttonPos.y + (menuButton.height - popupHeight) / 2

        const maxX = Math.max(8, overlay.width - accountPopup.width - 8)
        const maxY = Math.max(8, overlay.height - popupHeight - 8)

        accountPopup.x = Math.min(desiredX, maxX)
        accountPopup.y = Math.min(Math.max(8, desiredY), maxY)
        accountPopup.open()
    }

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
            iconSource: accountPopup.opened ? "qrc:/res/icon/chevron-down.png" : "qrc:/res/icon/chevron-up.png"
            toolTipText: ""
            onClicked: root.toggleAccountMenu()
        }
    }

    Popup {
        id: accountPopup
        parent: Overlay.overlay
        width: 196
        padding: 8
        implicitHeight: accountMenuContent.implicitHeight + topPadding + bottomPadding
        height: implicitHeight
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onAboutToHide: {
            if (!root.closingFromToggle && (menuButton.down || menuButton.hovered)) {
                root.suppressMenuToggleClick = true
            }
        }
        onClosed: root.closingFromToggle = false

        background: Rectangle {
            color: "#FFFFFF"
            radius: 10
            border.color: "#E5E7EB"
            border.width: 1
        }

        contentItem: ColumnLayout {
            id: accountMenuContent
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 30
                radius: 8
                color: settingsArea.containsMouse ? "#F9FAFB" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Image {
                        source: "qrc:/res/icon/set_up.png"
                        sourceSize.width: 12
                        sourceSize.height: 12
                        opacity: 0.65
                    }

                    Text {
                        text: "账号设置"
                        color: "#374151"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: settingsArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        accountPopup.close()
                        root.settingsRequested()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#F3F4F6"
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 30
                radius: 8
                color: switchArea.containsMouse ? "#F9FAFB" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Image {
                        source: "qrc:/res/icon/user.png"
                        sourceSize.width: 12
                        sourceSize.height: 12
                        opacity: 0.65
                    }

                    Text {
                        text: "切换账号"
                        color: "#374151"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: switchArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        accountPopup.close()
                        root.switchUserRequested()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#F3F4F6"
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 30
                radius: 8
                color: logoutArea.containsMouse ? "#FEF2F2" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Image {
                        source: "qrc:/res/icon/user-x.png"
                        sourceSize.width: 12
                        sourceSize.height: 12
                        opacity: 0.75
                    }

                    Text {
                        text: "退出登录"
                        color: "#B91C1C"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: logoutArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        accountPopup.close()
                        root.logoutRequested()
                    }
                }
            }
        }
    }
}
