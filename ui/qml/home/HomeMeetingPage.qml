import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Item {
    id: root

    property bool isGuest: true
    property string currentAction: "join"
    property var loginBackend: null

    signal promptRequested(string titleText, string messageText, string primaryText, string secondaryText, bool showCancel, string cancelText)

    function showGuestRestrictedPrompt() {
        root.promptRequested("需要登录",
                             "游客仅支持加入已存在普通房间，创建或预定会议请先登录。",
                             "去登录",
                             "稍后",
                             false,
                             "取消")
    }

    function openAction(action) {
        if (root.isGuest && (action === "quick" || action === "schedule")) {
            showGuestRestrictedPrompt()
            return
        }
        root.currentAction = action
        actionDialog.open()
    }

    function closeActionDialog() {
        actionDialog.close()
    }

    function actionTitle() {
        switch (root.currentAction) {
        case "join":
            return "加入会议"
        case "quick":
            return "快速会议"
        case "schedule":
            return "预定会议"
        case "share":
            return "共享屏幕"
        default:
            return "会议"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        ColumnLayout {
            spacing: 6
            Layout.bottomMargin: 16

            Text {
                text: "开始新的会议"
                color: Theme.textPrimary
                font.pixelSize: 21
                font.weight: Font.DemiBold
            }

            Text {
                text: root.isGuest
                    ? "游客仅可加入已存在普通房间，创建会议需登录"
                    : "可加入、创建与预定会议"
                color: Theme.textMuted
                font.pixelSize: 12
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            rowSpacing: 16
            columnSpacing: 16

            QuickActionCard {
                Layout.fillWidth: true
                title: "加入会议"
                subtitle: "输入会议号或链接"
                iconSource: "qrc:/res/icon/video.png"
                accentColor: "#2563EB"
                accentOpacity: 0.22
                iconOpacity: 1.0
                onClicked: root.openAction("join")
            }

            QuickActionCard {
                Layout.fillWidth: true
                title: "快速会议"
                subtitle: root.isGuest ? "登录后可创建临时会议" : "一键创建临时会议"
                iconSource: "qrc:/res/icon/monitor-up.png"
                accentColor: "#16A34A"
                accentOpacity: 0.22
                iconOpacity: 1.0
                onClicked: root.openAction("quick")
            }

            QuickActionCard {
                Layout.fillWidth: true
                title: "预定会议"
                subtitle: root.isGuest ? "登录后可预定会议" : "设置时间、密码与准入策略"
                iconSource: "qrc:/res/icon/pin.png"
                accentColor: "#F59E0B"
                accentOpacity: 0.18
                iconOpacity: 0.9
                onClicked: root.openAction("schedule")
            }

            QuickActionCard {
                Layout.fillWidth: true
                title: "共享屏幕"
                subtitle: "开始屏幕演示"
                iconSource: "qrc:/res/icon/screen_sharing_sidebar.png"
                accentColor: "#7C3AED"
                accentOpacity: 0.18
                iconOpacity: 0.9
                onClicked: root.openAction("share")
            }
        }
    }

    Popup {
        id: actionDialog

        modal: true
        focus: true
        width: root.currentAction === "schedule" ? 520 : 420
        height: root.currentAction === "schedule" ? 600 : 520
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: parent

        Overlay.modal: Rectangle {
            color: "#00000066"
        }

        background: Rectangle {
            color: "transparent"
        }

        contentItem: Rectangle {
            radius: 16
            color: Theme.cardBackground
            border.color: Theme.borderLight
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: root.currentAction === "schedule" ? 24 : 20
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: root.actionTitle()
                        color: Theme.textPrimary
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Item { Layout.fillWidth: true }

                    IconButton {
                        iconSource: "qrc:/res/icon/close.png"
                        toolTipText: "关闭"
                        onClicked: actionDialog.close()
                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: {
                        switch (root.currentAction) {
                        case "join": return 0
                        case "quick": return 1
                        case "schedule": return 2
                        case "share": return 3
                        default: return 0
                        }
                    }

                    JoinForm {
                        id: joinForm
                        userName: root.loginBackend ? root.loginBackend.userName : ""
                        roomName: root.loginBackend ? root.loginBackend.roomName : ""
                        loading: root.loginBackend ? root.loginBackend.loading : false
                        guestMode: root.isGuest
                        onUserNameChanged: {
                            if (root.loginBackend) root.loginBackend.userName = userName
                        }
                        onRoomNameChanged: {
                            if (root.loginBackend) root.loginBackend.roomName = roomName
                        }
                        onJoinClicked: {
                            if (root.loginBackend) {
                                root.loginBackend.join()
                            }
                        }
                    }

                    QuickStartForm {
                        loading: root.loginBackend ? root.loginBackend.loading : false
                        allowGuestJoin: root.loginBackend ? root.loginBackend.allowGuestJoin : false
                        onAllowGuestJoinToggled: function(checked) {
                            if (root.loginBackend) {
                                root.loginBackend.allowGuestJoin = checked
                            }
                        }
                        onQuickJoinClicked: {
                            if (root.loginBackend) {
                                root.loginBackend.quickJoin()
                            }
                        }
                    }

                    ScheduleForm {
                        loading: root.loginBackend ? root.loginBackend.loading : false
                        onCreateRoomClicked: function(topic,
                                                      localDate,
                                                      hour,
                                                      minute,
                                                      allowGuestJoin,
                                                      meetingPassword,
                                                      noJoinAutoEndMinutes,
                                                      emptyAutoEndMinutes) {
                            if (root.isGuest) {
                                root.showGuestRestrictedPrompt()
                                return
                            }
                            if (root.loginBackend) {
                                root.loginBackend.createScheduledMeeting(topic,
                                                                         localDate,
                                                                         hour,
                                                                         minute,
                                                                         allowGuestJoin,
                                                                         meetingPassword,
                                                                         noJoinAutoEndMinutes,
                                                                         emptyAutoEndMinutes)
                            }
                        }
                    }

                    ShareScreenForm {
                        onShareClicked: actionDialog.close()
                    }
                }

                Text {
                    visible: root.loginBackend && root.loginBackend.errorMessage.length > 0
                    text: root.loginBackend ? root.loginBackend.errorMessage : ""
                    color: "#EF4444"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
