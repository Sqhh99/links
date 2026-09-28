import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Links
import Links.Backend 1.0

// Round avatar at the top of the home rail. Clicking it opens the account
// popup (guest: login card, signed in: account actions).
Item {
    id: root

    property bool isGuest: true
    property string userName: "游客"
    property string subtitle: "已登录"
    property bool suppressMenuToggleClick: false
    property bool closingFromToggle: false

    signal loginClicked()
    signal settingsRequested()
    signal switchUserRequested()
    signal logoutRequested()

    implicitWidth: 40
    implicitHeight: 40

    readonly property string initial: {
        var name = root.isGuest ? "" : root.userName.trim()
        return name.length > 0 ? name.charAt(0).toUpperCase() : ""
    }

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
        const buttonPos = avatar.mapToItem(overlay, avatar.width, 0)
        const desiredX = buttonPos.x + 12
        const desiredY = buttonPos.y - 4

        const maxX = Math.max(8, overlay.width - accountPopup.width - 8)
        const maxY = Math.max(8, overlay.height - popupHeight - 8)

        accountPopup.x = Math.min(desiredX, maxX)
        accountPopup.y = Math.min(Math.max(8, desiredY), maxY)
        accountPopup.open()
    }

    // Inline components cannot see this file's ids, so state is passed in.
    component Avatar: Rectangle {
        id: avatarItem
        property int diameter: 40
        property bool guest: true
        property string initial: ""
        width: diameter
        height: diameter
        radius: diameter / 2
        color: guest ? Theme.hoverBackground : Theme.brand

        Text {
            anchors.centerIn: parent
            visible: avatarItem.initial.length > 0
            text: avatarItem.initial
            color: "#FFFFFF"
            font.pixelSize: avatarItem.diameter * 0.42
            font.weight: Font.DemiBold
        }

        Icon {
            anchors.centerIn: parent
            visible: avatarItem.initial.length === 0
            name: "user"
            size: Math.round(avatarItem.diameter * 0.45)
            color: avatarItem.guest ? Theme.iconSecondary : "#FFFFFF"
        }
    }

    Avatar {
        id: avatar
        anchors.centerIn: parent
        diameter: 40
        guest: root.isGuest
        initial: root.initial
        border.width: avatarHover.hovered || accountPopup.opened ? 2 : 0
        border.color: Theme.brandSoftStrong

        HoverHandler {
            id: avatarHover
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onTapped: root.toggleAccountMenu()
        }

        ToolTip.visible: avatarHover.hovered && !accountPopup.opened
        ToolTip.text: root.isGuest ? "游客 · 点击登录" : root.userName
        ToolTip.delay: 500
    }

    component MenuRow: Rectangle {
        id: menuRow
        property string iconName: ""
        property string label: ""
        property bool destructive: false
        signal triggered()

        Layout.fillWidth: true
        implicitHeight: 34
        radius: Theme.radiusSm
        color: rowHover.hovered ? (destructive ? Theme.dangerSoft : Theme.hoverBackground) : "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 10

            Icon {
                name: menuRow.iconName
                size: 15
                color: menuRow.destructive ? Theme.danger : Theme.iconSecondary
            }

            Text {
                text: menuRow.label
                color: menuRow.destructive ? Theme.danger : Theme.textPrimary
                font.pixelSize: 13
                Layout.fillWidth: true
            }
        }

        HoverHandler {
            id: rowHover
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onTapped: menuRow.triggered()
        }
    }

    Popup {
        id: accountPopup
        parent: Overlay.overlay
        width: 232
        padding: 8
        implicitHeight: accountMenuContent.implicitHeight + topPadding + bottomPadding
        height: implicitHeight
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onAboutToHide: {
            if (!root.closingFromToggle && avatarHover.hovered) {
                root.suppressMenuToggleClick = true
            }
        }
        onClosed: root.closingFromToggle = false

        background: Rectangle {
            color: Theme.popupBackground
            radius: 12
            border.color: Theme.popupBorder
            border.width: 1

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Theme.shadowColor
                shadowBlur: 0.8
                shadowVerticalOffset: 6
            }
        }

        contentItem: ColumnLayout {
            id: accountMenuContent
            spacing: 2

            // Header: avatar + name
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 8
                spacing: 10

                Avatar {
                    diameter: 36
                    guest: root.isGuest
                    initial: root.initial
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: root.isGuest ? "游客" : root.userName
                        color: Theme.textPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: root.isGuest ? "仅可加入已存在的普通会议" : root.subtitle
                        color: Theme.textMuted
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            PrimaryButton {
                visible: root.isGuest
                Layout.fillWidth: true
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.bottomMargin: 6
                implicitHeight: 34
                text: "登录"
                onClicked: {
                    accountPopup.close()
                    root.loginClicked()
                }
            }

            Rectangle {
                visible: !root.isGuest
                Layout.fillWidth: true
                Layout.topMargin: 2
                Layout.bottomMargin: 4
                implicitHeight: 1
                color: Theme.separatorColor
            }

            MenuRow {
                visible: !root.isGuest
                iconName: "user-cog"
                label: "账号设置"
                onTriggered: {
                    accountPopup.close()
                    root.settingsRequested()
                }
            }

            MenuRow {
                visible: !root.isGuest
                iconName: "repeat"
                label: "切换账号"
                onTriggered: {
                    accountPopup.close()
                    root.switchUserRequested()
                }
            }

            MenuRow {
                visible: !root.isGuest
                iconName: "log-out"
                label: "退出登录"
                destructive: true
                onTriggered: {
                    accountPopup.close()
                    root.logoutRequested()
                }
            }
        }
    }
}
