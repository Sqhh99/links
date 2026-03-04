import QtQuick
import QtQuick.Layouts
import Links
import Links.Backend 1.0

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

    color: Theme.cardBackground
    border.color: Theme.borderLight
    border.width: 1
    radius: 16
    clip: true

    Behavior on color { ColorAnimation { duration: 200 } }
    Behavior on border.color { ColorAnimation { duration: 200 } }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 18

        HomeUserCard {
            Layout.fillWidth: true
            isGuest: root.isGuest
            userName: root.userName
            onLoginClicked: root.loginRequested()
            onSettingsRequested: root.accountSettingsRequested()
            onSwitchUserRequested: root.switchUserRequested()
            onLogoutRequested: root.logoutRequested()
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            HomeNavButton {
                text: "会议"
                iconSource: "qrc:/res/icon/video.png"
                active: root.currentIndex === 0
                onClicked: root.navChanged(0)
            }

            HomeNavButton {
                text: "录制"
                iconSource: "qrc:/res/icon/screen-share-off.png"
                active: root.currentIndex === 1
                onClicked: root.navChanged(1)
            }
        }

        Item { Layout.fillHeight: true }

        Item { height: 1 }
    }
}
