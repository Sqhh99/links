import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

Item {
    id: root

    property bool isGuest: true
    property string currentAction: "join"
    property var loginBackend: null

    signal promptRequested(string titleText, string messageText, string primaryText, string secondaryText, bool showCancel, string cancelText)

    function openAction(action) {
        root.currentAction = action
        actionDialog.open()
    }

    function closeActionDialog() {
        actionDialog.close()
    }

    function ensureUserName() {
        if (!root.loginBackend) return
        var name = root.loginBackend.userName ? root.loginBackend.userName.trim() : ""
        if (name.length === 0) {
            root.loginBackend.userName = "Guest"
        }
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
                color: "#111827"
                font.pixelSize: 21
                font.weight: Font.DemiBold
            }

            Text {
                text: "无需登录即可加入或创建会议"
                color: "#6B7280"
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
                subtitle: "一键创建临时会议"
                iconSource: "qrc:/res/icon/monitor-up.png"
                accentColor: "#16A34A"
                accentOpacity: 0.22
                iconOpacity: 1.0
                onClicked: root.openAction("quick")
            }

            QuickActionCard {
                Layout.fillWidth: true
                title: "预定会议"
                subtitle: "设置时间并本地提醒"
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
        width: 420
        height: 520
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
            color: "#FFFFFF"
            border.color: "#E5E7EB"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: root.actionTitle()
                        color: "#111827"
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
                        onQuickJoinClicked: {
                            if (root.loginBackend) {
                                root.ensureUserName()
                                root.loginBackend.quickJoin()
                            }
                        }
                    }

                    ScheduleForm {
                        onCreateRoomClicked: {
                            actionDialog.close()
                            if (root.isGuest) {
                                root.promptRequested("已保存到本地",
                                                     "登录后可跨设备同步会议安排",
                                                     "继续创建",
                                                     "去登录",
                                                     true,
                                                     "取消")
                            }
                            if (root.loginBackend) {
                                root.ensureUserName()
                                root.loginBackend.createScheduledRoom()
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
