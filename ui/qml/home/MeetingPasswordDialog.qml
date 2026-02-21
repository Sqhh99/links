import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

Popup {
    id: root

    property string meetingNo: ""
    property string messageText: "该会议需要密码，请输入会议密码"
    property bool invalidAttempt: false

    signal submitted(string password)
    signal cancelled()

    modal: true
    focus: true
    width: 360
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    Overlay.modal: Rectangle {
        color: "#00000066"
    }

    background: Rectangle {
        color: "transparent"
    }

    onOpened: {
        passwordInput.text = ""
        passwordInput.forceActiveFocus()
    }

    contentItem: Rectangle {
        radius: 16
        color: "#FFFFFF"
        border.color: "#E5E7EB"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            Text {
                text: root.invalidAttempt ? "密码错误，请重试" : "请输入会议密码"
                color: "#111827"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Text {
                text: root.messageText
                color: "#6B7280"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                visible: root.meetingNo.length > 0
                text: "会议号: " + root.meetingNo
                color: "#6B7280"
                font.pixelSize: 11
            }

            TextField {
                id: passwordInput
                Layout.fillWidth: true
                placeholderText: "输入会议密码"
                echoMode: TextInput.Password
                Keys.onReturnPressed: {
                    if (passwordInput.text.trim().length > 0) {
                        root.submitted(passwordInput.text)
                        root.close()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                SecondaryButton {
                    text: "取消"
                    Layout.fillWidth: true
                    onClicked: {
                        root.cancelled()
                        root.close()
                    }
                }

                PrimaryButton {
                    text: "确认"
                    Layout.fillWidth: true
                    enabled: passwordInput.text.trim().length > 0
                    onClicked: {
                        root.submitted(passwordInput.text)
                        root.close()
                    }
                }
            }
        }
    }
}
