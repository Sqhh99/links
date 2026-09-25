import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
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

    GridLayout {
        anchors.centerIn: parent
        columns: 2
        rowSpacing: 28
        columnSpacing: 36

        QuickActionCard {
            title: "加入会议"
            iconName: "plus"
            onClicked: root.openAction("join")
        }

        QuickActionCard {
            title: "快速会议"
            iconName: "zap"
            locked: root.isGuest
            toolTipText: root.isGuest ? "登录后可创建临时会议" : "一键创建临时会议"
            onClicked: root.openAction("quick")
        }

        QuickActionCard {
            title: "预定会议"
            iconName: "calendar-check"
            locked: root.isGuest
            toolTipText: root.isGuest ? "登录后可预定会议" : "设置时间、密码与准入策略"
            onClicked: root.openAction("schedule")
        }

        QuickActionCard {
            title: "共享屏幕"
            iconName: "screen-share"
            onClicked: root.openAction("share")
        }
    }

    Popup {
        id: actionDialog

        modal: true
        focus: true
        parent: Overlay.overlay
        width: root.currentAction === "schedule" ? 480 : 400
        height: Math.min(root.currentAction === "schedule" ? 548 : 420,
                         (parent ? parent.height : 580) - 32)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: parent

        Overlay.modal: Rectangle {
            color: Theme.overlayColor
        }

        background: Rectangle {
            color: "transparent"
        }

        contentItem: Rectangle {
            radius: Theme.radiusLg
            color: Theme.cardBackground
            border.color: Theme.popupBorder
            border.width: 1

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Theme.shadowColor
                shadowBlur: 1.0
                shadowVerticalOffset: 8
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
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
                        iconName: "x"
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
                    id: errorText
                    visible: root.loginBackend && root.loginBackend.errorMessage.length > 0
                    text: root.loginBackend ? root.loginBackend.errorMessage : ""
                    color: Theme.errorColor
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
