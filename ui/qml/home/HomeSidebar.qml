import QtQuick
import QtQuick.Layouts
import Links
import Links.Backend 1.0

// Narrow navigation rail: avatar, page icons, settings at the bottom
Rectangle {
    id: root

    property bool isGuest: true
    property string userName: "游客"
    property int currentIndex: 0

    signal navChanged(int index)
    signal loginRequested()
    signal switchUserRequested()
    signal logoutRequested()
    signal accountSettingsRequested()
    signal settingsRequested()

    color: Theme.railBackground

    Behavior on color { ColorAnimation { duration: 200 } }

    // Hairline on the right edge
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Theme.separatorColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 20
        anchors.bottomMargin: 14
        spacing: 6

        HomeUserCard {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 18
            isGuest: root.isGuest
            userName: root.userName
            onLoginClicked: root.loginRequested()
            onSettingsRequested: root.accountSettingsRequested()
            onSwitchUserRequested: root.switchUserRequested()
            onLogoutRequested: root.logoutRequested()
        }

        HomeNavButton {
            text: "会议"
            iconName: "video"
            active: root.currentIndex === 0
            onClicked: root.navChanged(0)
        }

        HomeNavButton {
            text: "录制"
            iconName: "circle-dot"
            active: root.currentIndex === 1
            onClicked: root.navChanged(1)
        }

        Item { Layout.fillHeight: true }

        HomeNavButton {
            text: "设置"
            iconName: "settings"
            active: false
            checkable: false
            onClicked: root.settingsRequested()
        }
    }
}
