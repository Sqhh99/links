import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Links

Item {
    id: root

    property var authBackend: null

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
                text: "登录"
                color: Theme.textPrimary
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Text {
                text: "使用用户名和密码登录，首次登录将自动创建账号"
                color: Theme.textMuted
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        LoginForm {
            id: loginForm
            Layout.fillWidth: true
            Layout.fillHeight: true
            loading: root.authBackend ? root.authBackend.loading : false
            onLoginRequested: function(requestUsername, requestPassword) {
                if (root.authBackend) {
                    root.authBackend.login(requestUsername, requestPassword)
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
