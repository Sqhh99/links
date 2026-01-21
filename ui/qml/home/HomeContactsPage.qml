import QtQuick
import QtQuick.Layouts
import Links

Item {
    id: root

    property bool isGuest: true

    signal requestLogin()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        ColumnLayout {
            spacing: 6

            Text {
                text: "通讯录"
                color: "#111827"
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Text {
                text: root.isGuest ? "游客模式下可使用基础邀请方式" : "同步团队通讯录与联系人"
                color: "#6B7280"
                font.pixelSize: 12
            }
        }

        ColumnLayout {
            spacing: 10

            Text {
                text: "邀请方式"
                color: "#111827"
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                InviteOptionCard {
                    Layout.fillWidth: true
                    title: "复制邀请链接"
                    subtitle: "生成快速入会链接并分享"
                    actionText: "复制链接"
                    iconSource: "qrc:/res/icon/pin.png"
                }

                InviteOptionCard {
                    Layout.fillWidth: true
                    title: "二维码邀请"
                    subtitle: "扫码即可加入会议"
                    actionText: "生成二维码"
                    iconSource: "qrc:/res/icon/chevron-down.png"
                }

                InviteOptionCard {
                    Layout.fillWidth: true
                    title: "邮件邀请"
                    subtitle: "一键发送会议邀请"
                    actionText: "发送邮件"
                    iconSource: "qrc:/res/icon/message.png"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#E5E7EB"
        }

        ColumnLayout {
            spacing: 10

            Text {
                text: "团队通讯录"
                color: "#111827"
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            LoginPromptCard {
                Layout.fillWidth: true
                title: "登录后开启团队通讯录"
                message: "同步组织成员、常用联系人与历史协作记录。"
                onPrimaryClicked: root.requestLogin()
                onSecondaryClicked: {}
            }
        }

        Item { Layout.fillHeight: true }
    }
}
