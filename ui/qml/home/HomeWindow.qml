import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import Links
import Links.Backend 1.0

Window {
    id: root

    width: 880
    height: 580
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

    // Current time for the date header, refreshed every minute
    property date now: new Date()

    Timer {
        interval: 60 * 1000
        running: root.visible
        repeat: true
        onTriggered: root.now = new Date()
    }

    readonly property var weekdayNames: ["周日", "周一", "周二", "周三", "周四", "周五", "周六"]

    function pad2(n) {
        return n < 10 ? "0" + n : "" + n
    }

    Rectangle {
        id: windowFrame
        anchors.fill: parent
        radius: 12
        border.color: Theme.windowBorder
        border.width: 1
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.pageWashTop }
            GradientStop { position: 0.55; color: Theme.pageWashBottom }
            GradientStop { position: 1.0; color: Theme.pageBackground }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            HomeSidebar {
                Layout.preferredWidth: 64
                Layout.fillHeight: true
                topLeftRadius: 11
                bottomLeftRadius: 11
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

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                TitleBar {
                    Layout.fillWidth: true
                    targetWindow: root
                    showTitle: false
                    showSettingsButton: false
                    onMinimizeClicked: root.showMinimized()
                    onCloseClicked: Qt.quit()
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    StackLayout {
                        id: pageStack
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.leftMargin: root.currentIndex === 0 ? 0 : 28
                        Layout.rightMargin: root.currentIndex === 0 ? 0 : 28
                        Layout.bottomMargin: 20
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

                    // Separator between actions and the schedule panel
                    Rectangle {
                        visible: root.currentIndex === 0
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        Layout.bottomMargin: 28
                        color: Theme.separatorColor
                    }

                    // Date header + meeting lists
                    // Nested layouts default to fillWidth: true, which would let this
                    // column compete with the action page for space; pin it to 300.
                    ColumnLayout {
                        visible: root.currentIndex === 0
                        Layout.fillWidth: false
                        Layout.preferredWidth: 300
                        Layout.maximumWidth: 300
                        Layout.fillHeight: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        spacing: 0

                        Text {
                            text: root.pad2(root.now.getMonth() + 1) + "/" + root.pad2(root.now.getDate())
                            color: Theme.textPrimary
                            font.pixelSize: 40
                            font.weight: Font.Bold
                            font.letterSpacing: -0.5
                            Layout.leftMargin: 10
                        }

                        RowLayout {
                            Layout.leftMargin: 10
                            Layout.topMargin: 2
                            spacing: 6

                            Icon {
                                name: "calendar"
                                size: 14
                                color: Theme.iconSecondary
                            }

                            Text {
                                text: root.weekdayNames[root.now.getDay()] + "  ·  "
                                      + (root.isGuest
                                         ? "游客模式"
                                         : (hostMeetingsModel.count > 0
                                            ? hostMeetingsModel.count + " 场预定会议"
                                            : "暂无预定"))
                                color: Theme.textTertiary
                                font.pixelSize: 13
                            }
                        }

                        // Segmented tabs
                        Rectangle {
                            visible: !root.isGuest
                            Layout.fillWidth: true
                            Layout.topMargin: 18
                            Layout.leftMargin: 6
                            Layout.rightMargin: 4
                            implicitHeight: 36
                            radius: 9
                            color: Theme.tabInactiveBg

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 3
                                spacing: 3

                                TabButton {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    text: "会议记录"
                                    active: root.rightPanelTab === 0
                                    onClicked: root.rightPanelTab = 0
                                }

                                TabButton {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    text: "我的预定"
                                    active: root.rightPanelTab === 1
                                    onClicked: root.rightPanelTab = 1
                                }
                            }
                        }

                        Text {
                            visible: root.isGuest
                            text: "会议记录"
                            color: Theme.textSecondary
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            Layout.leftMargin: 10
                            Layout.topMargin: 22
                        }

                        Loader {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.topMargin: 10
                            sourceComponent: (!root.isGuest && root.rightPanelTab === 1)
                                ? hostMeetingsPanelComponent
                                : meetingRecordsPanelComponent
                        }

                        Component {
                            id: meetingRecordsPanelComponent

                            MeetingListPanel {
                                isGuest: root.isGuest
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
