import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import Links
import Links.Backend 1.0

Window {
    id: root

    width: 1160
    height: 720
    visible: true
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Window
    title: "Links"
    onClosing: Qt.quit()

    property bool isGuest: !authBackend.isLoggedIn
    property string userName: authBackend.isLoggedIn ? authBackend.userName : "游客"
    property int currentIndex: 0

    function openAuthModal() {
        authModal.open()
    }

    ListModel {
        id: localMeetingsModel
    }

    ListModel {
        id: cloudMeetingsModel
        ListElement { title: "产品评审"; time: "今天 10:00 - 11:00"; tag: "云端" }
        ListElement { title: "周会同步"; time: "明天 09:30 - 10:00"; tag: "云端" }
    }

    LoginBackend {
        id: joinBackend
        onJoinConference: function(url, token, roomName, userName, isHost) {
            meetingPage.closeActionDialog()
            root.hide()
        }
    }

    AuthBackend {
        id: authBackend
        onLoginSucceeded: authModal.close()
        onRegisterSucceeded: authModal.close()
    }

    Rectangle {
        id: windowFrame
        anchors.fill: parent
        color: "#F5F7FA"
        radius: 16
        border.color: "#E5E7EB"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TitleBar {
                Layout.fillWidth: true
                targetWindow: root
                title: "Links"
                onSettingsClicked: settingsDialog.open()
                onMinimizeClicked: root.showMinimized()
                onCloseClicked: Qt.quit()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 20
                Layout.topMargin: 12
                spacing: 20

                HomeSidebar {
                    Layout.preferredWidth: 220
                    Layout.fillHeight: true
                    isGuest: root.isGuest
                    userName: root.userName
                    currentIndex: root.currentIndex
                    onNavChanged: function(index) { root.currentIndex = index }
                    onLoginRequested: root.openAuthModal()
                    onLogoutRequested: authBackend.logout()
                    onAccountSettingsRequested: settingsDialog.open()
                    onSettingsRequested: settingsDialog.open()
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 16
                    color: "transparent"

                    StackLayout {
                        id: pageStack
                        anchors.fill: parent
                        currentIndex: root.currentIndex

                        HomeMeetingPage {
                            id: meetingPage
                            isGuest: root.isGuest
                            loginBackend: joinBackend
                            onPromptRequested: function(titleText, messageText, primaryText, secondaryText, showCancel, cancelText) {
                                promptDialog.titleText = titleText
                                promptDialog.messageText = messageText
                                promptDialog.primaryText = primaryText
                                promptDialog.secondaryText = secondaryText
                                promptDialog.showCancel = showCancel
                                promptDialog.cancelText = cancelText
                                promptDialog.showOptOut = false
                                promptDialog.open()
                            }
                        }

                        HomeRecordingPage {
                            isGuest: root.isGuest
                            onRequestLogin: root.openAuthModal()
                        }
                    }
                }

                MeetingListPanel {
                    Layout.preferredWidth: root.currentIndex === 0 ? 300 : 0
                    Layout.fillHeight: true
                    visible: root.currentIndex === 0
                    isGuest: root.isGuest
                    headerTitle: root.isGuest ? "本地最近会议记录" : "云端会议"
                    headerTag: ""
                    actionText: "查看全部 >"
                    meetingsModel: root.isGuest ? localMeetingsModel : cloudMeetingsModel
                    onQuickMeetingClicked: meetingPage.openAction("quick")
                    onJoinMeetingClicked: meetingPage.openAction("join")
                }
            }
        }
    }

    SettingsWindow {
        id: settingsDialog
    }

    AuthModal {
        id: authModal
        authBackend: authBackend
    }

    GuestPromptDialog {
        id: promptDialog
        onPrimaryClicked: root.openAuthModal()
    }
}
