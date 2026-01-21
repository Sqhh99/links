import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

Item {
    id: root

    property string mode: "login"
    property bool loading: false

    signal loginRequested(string email, string password)
    signal registerRequested(string displayName, string email, string code, string password)

    onModeChanged: {
        loginForm.resetValidation()
        registerForm.resetValidation()
    }

    Rectangle {
        anchors.fill: parent
        radius: 16
        color: "#FFFFFF"
        border.color: "#E5E7EB"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        anchors.bottomMargin: 24
        spacing: 16

        ColumnLayout {
            spacing: 6

            Text {
                text: root.mode === "login" ? "欢迎回来" : "创建账号"
                color: "#111827"
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }

            Text {
                text: root.mode === "login" ? "使用邮箱与密码登录" : "创建你的会议账户"
                color: "#6B7280"
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
                loading: root.loading
                opacity: root.mode === "login" ? 1 : 0
                x: root.mode === "login" ? 0 : -24
                enabled: root.mode === "login"
                onLoginRequested: root.loginRequested(email, password)

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
                loading: root.loading
                opacity: root.mode === "register" ? 1 : 0
                x: root.mode === "register" ? 0 : 24
                enabled: root.mode === "register"
                onRegisterRequested: root.registerRequested(displayName, email, code, password)

                Behavior on opacity {
                    NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
                }
                Behavior on x {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
            }
        }
    }
}
