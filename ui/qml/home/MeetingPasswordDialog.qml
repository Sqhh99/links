import QtQuick
import QtQuick.Controls
import QtQuick.Controls as QQC2
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
    width: 380
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    Overlay.modal: Rectangle {
        color: "#00000066"
    }

    background: Rectangle {
        color: "transparent"
        border.width: 0
    }

    onOpened: {
        passwordInput.text = ""
        passwordInput.forceActiveFocus()
    }

    contentItem: Rectangle {
        radius: 18
        color: "#FFFFFF"
        border.color: "#E5E7EB"
        border.width: 1
        clip: true
        implicitHeight: innerLayout.implicitHeight + 48

        ColumnLayout {
            id: innerLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 24
            spacing: 14

            Text {
                text: root.invalidAttempt ? "密码错误，请重新输入" : "请输入会议密码"
                color: root.invalidAttempt ? "#B91C1C" : "#111827"
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

            Rectangle {
                visible: root.meetingNo.length > 0
                Layout.fillWidth: true
                radius: 9
                color: "#F8FAFC"
                border.color: "#E5E7EB"
                border.width: 1
                implicitHeight: 32

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    text: "会议号: " + root.meetingNo
                    color: "#374151"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "会议密码"
                    color: "#374151"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                QQC2.TextField {
                    id: passwordInput
                    Layout.fillWidth: true
                    implicitHeight: 44
                    placeholderText: "输入会议密码"
                    echoMode: TextInput.Password
                    leftPadding: 12
                    rightPadding: 12
                    color: "#111827"
                    placeholderTextColor: "#9CA3AF"

                    background: Rectangle {
                        radius: 10
                        color: "#FFFFFF"
                        border.width: 1
                        border.color: root.invalidAttempt
                            ? "#DC2626"
                            : (passwordInput.activeFocus ? "#2563EB" : "#D1D5DB")
                    }

                    Keys.onReturnPressed: {
                        if (passwordInput.text.trim().length > 0) {
                            root.submitted(passwordInput.text)
                            root.close()
                        }
                    }
                }

                Text {
                    visible: root.invalidAttempt
                    text: "密码不正确，请检查后重试"
                    color: "#DC2626"
                    font.pixelSize: 11
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Layout.topMargin: 4

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
