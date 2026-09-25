import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Links

Item {
    id: root

    property string mode: "login"
    property var authBackend: null

    onModeChanged: {
        loginForm.resetValidation()
        registerForm.resetValidation()
    }

    Rectangle {
        anchors.fill: parent
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
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        anchors.bottomMargin: 20
        spacing: 16

        ColumnLayout {
            spacing: 6

            Text {
                text: root.mode === "login" ? "欢迎回来" : "创建账号"
                color: Theme.textPrimary
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Text {
                text: root.mode === "login" ? "使用邮箱与密码登录" : "创建你的会议账户"
                color: Theme.textMuted
                font.pixelSize: 12
            }
        }

        AuthTabs {
            mode: root.mode
            onModeSelected: function(modeValue) { root.mode = modeValue }
        }

        Item {
            id: formStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            LoginForm {
                id: loginForm
                anchors.fill: parent
                loading: root.authBackend ? root.authBackend.loading : false
                opacity: root.mode === "login" ? 1 : 0
                x: root.mode === "login" ? 0 : -24
                enabled: root.mode === "login"
                onLoginRequested: function(requestEmail, requestPassword) {
                    if (root.authBackend) {
                        root.authBackend.login(requestEmail, requestPassword)
                    }
                }

                Behavior on opacity {
                    NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
                }
                Behavior on x {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
            }

            RegisterForm {
                id: registerForm
                anchors.fill: parent
                loading: root.authBackend ? root.authBackend.loading : false
                codeCooldown: root.authBackend ? root.authBackend.codeCooldown : 0
                opacity: root.mode === "register" ? 1 : 0
                x: root.mode === "register" ? 0 : 24
                enabled: root.mode === "register"
                onRegisterRequested: function(requestDisplayName, requestEmail, requestCode, requestPassword) {
                    if (root.authBackend) {
                        root.authBackend.registerUser(requestDisplayName, requestEmail, requestCode, requestPassword)
                    }
                }
                onRequestCodeClicked: function(requestEmail) {
                    if (root.authBackend) {
                        root.authBackend.requestCode(requestEmail)
                    }
                }

                Behavior on opacity {
                    NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
                }
                Behavior on x {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
            }
        }

        // Error message display
        Text {
            visible: root.authBackend && root.authBackend.errorMessage.length > 0
            text: root.authBackend ? root.authBackend.errorMessage : ""
            color: Theme.danger
            font.pixelSize: 13
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }
}
