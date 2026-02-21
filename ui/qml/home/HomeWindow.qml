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
    property int rightPanelTab: 0

    function openAuthModal(mode) {
        authModal.openWithMode(mode ? mode : "login")
    }

    function handleSessionExpired(message) {
        authBackend.logout()
        localMeetingsModel.clear()
        hostMeetingsModel.clear()

        promptDialog.titleText = "登录已过期"
        promptDialog.messageText = (message && message.length > 0)
            ? message
            : "登录状态已过期，请重新登录后继续。"
        promptDialog.primaryText = "重新登录"
        promptDialog.secondaryText = "稍后"
        promptDialog.showCancel = false
        promptDialog.showOptOut = false
        promptDialog.open()
    }

    function reloadMeetingData() {
        if (authBackend.isLoggedIn) {
            joinBackend.loadMeetingRecords()
            joinBackend.loadHostMeetings()
        } else {
            localMeetingsModel.clear()
            hostMeetingsModel.clear()
        }
    }

    ListModel {
        id: localMeetingsModel
    }

    ListModel {
        id: hostMeetingsModel
    }

    LoginBackend {
        id: joinBackend
        sessionLoggedIn: authBackend.isLoggedIn
        sessionAuthToken: authBackend.authToken

        onJoinConference: function(url, token, roomName, meetingNo, userName, isHost, userAuthToken, isGuest) {
            meetingPage.closeActionDialog()
            passwordDialog.close()
            root.hide()
        }

        onMeetingRecordsLoaded: function(records) {
            localMeetingsModel.clear()
            for (var i = 0; i < records.length; ++i) {
                localMeetingsModel.append(records[i])
            }
        }

        onHostMeetingsLoaded: function(records) {
            hostMeetingsModel.clear()
            for (var i = 0; i < records.length; ++i) {
                hostMeetingsModel.append(records[i])
            }
        }

        onScheduledMeetingCreated: function(meetingNo, roomName, shareUrl) {
            meetingPage.closeActionDialog()
            root.rightPanelTab = 1
            joinBackend.loadHostMeetings()
        }

        onMeetingPasswordRequired: function(meetingNo, message, invalidAttempt) {
            passwordDialog.meetingNo = meetingNo
            passwordDialog.messageText = message
            passwordDialog.invalidAttempt = invalidAttempt
            passwordDialog.open()
        }
    }

    AuthBackend {
        id: authBackend
        onLoginSucceeded: {
            joinBackend.syncParticipantNameFromSession()
            authModal.close()
            root.reloadMeetingData()
        }
        onRegisterSucceeded: {
            joinBackend.syncParticipantNameFromSession()
            authModal.close()
            root.reloadMeetingData()
        }
        onSwitchUserRequested: {
            root.openAuthModal("login")
        }
        onSessionExpired: function(message) {
            root.handleSessionExpired(message)
        }
    }

    Connections {
        target: joinBackend
        function onSessionExpired(message) {
            root.handleSessionExpired(message)
        }
    }

    Connections {
        target: authBackend
        function onIsLoggedInChanged() {
            root.rightPanelTab = 0
            root.reloadMeetingData()
        }
    }

    Component.onCompleted: {
        root.reloadMeetingData()
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
                    onSwitchUserRequested: {
                        authBackend.switchUser()
                        localMeetingsModel.clear()
                        hostMeetingsModel.clear()
                    }
                    onLogoutRequested: {
                        authBackend.logout()
                        localMeetingsModel.clear()
                        hostMeetingsModel.clear()
                    }
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
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: root.currentIndex === 0 ? 300 : 0
                    Layout.fillHeight: true
                    visible: root.currentIndex === 0
                    color: "transparent"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            visible: !root.isGuest
                            spacing: 8

                            TabButton {
                                Layout.fillWidth: true
                                text: "会议记录"
                                active: root.rightPanelTab === 0
                                onClicked: root.rightPanelTab = 0
                            }

                            TabButton {
                                Layout.fillWidth: true
                                text: "我的预定"
                                active: root.rightPanelTab === 1
                                onClicked: root.rightPanelTab = 1
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            sourceComponent: (!root.isGuest && root.rightPanelTab === 1)
                                ? hostMeetingsPanelComponent
                                : meetingRecordsPanelComponent
                        }

                        Component {
                            id: meetingRecordsPanelComponent

                            MeetingListPanel {
                                isGuest: root.isGuest
                                headerTitle: "会议记录"
                                headerTag: ""
                                actionText: "查看全部 >"
                                meetingsModel: localMeetingsModel
                                onQuickMeetingClicked: meetingPage.openAction("quick")
                                onJoinMeetingClicked: meetingPage.openAction("join")
                            }
                        }

                        Component {
                            id: hostMeetingsPanelComponent

                            HostMeetingListPanel {
                                meetingsModel: hostMeetingsModel
                                loading: joinBackend.loading
                                errorMessage: joinBackend.errorMessage
                                onJoinMeeting: function(meetingNo) {
                                    joinBackend.joinHostedMeeting(meetingNo)
                                }
                                onCancelMeeting: function(meetingNo) {
                                    for (var i = 0; i < hostMeetingsModel.count; ++i) {
                                        var row = hostMeetingsModel.get(i)
                                        if (row.meetingNo === meetingNo) {
                                            cancelMeetingDialog.meetingNo = meetingNo
                                            cancelMeetingDialog.meetingTitle = row.title
                                            cancelMeetingDialog.open()
                                            break
                                        }
                                    }
                                }
                            }
                        }
                    }
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
        onPrimaryClicked: root.openAuthModal("login")
    }

    MeetingPasswordDialog {
        id: passwordDialog
        onSubmitted: function(password) {
            joinBackend.submitMeetingPassword(password)
        }
        onCancelled: {
            joinBackend.cancelPasswordRetry()
        }
    }

    CancelMeetingDialog {
        id: cancelMeetingDialog
        onConfirmed: function(meetingNo) {
            joinBackend.cancelHostedMeeting(meetingNo)
        }
    }
}
