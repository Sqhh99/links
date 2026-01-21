import QtQuick
import QtQuick.Layouts
import Links

Rectangle {
    id: root

    property bool isGuest: true
    property string userName: "游客"
    property int currentIndex: 0

    signal navChanged(int index)
    signal loginRequested()
    signal logoutRequested()
    signal accountSettingsRequested()
    signal settingsRequested()

    color: "#FFFFFF"
    border.color: "#E5E7EB"
    border.width: 1
    radius: 16
    clip: true

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
