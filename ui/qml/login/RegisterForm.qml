import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Links

ColumnLayout {
    id: root

    property bool loading: false
    property bool showErrors: false
    property bool nameTouched: false
    property bool emailTouched: false
    property bool codeTouched: false
    property bool passwordTouched: false
    property string displayName: nameField.text
    property string email: emailField.text
    property string code: codeInput.text
    property string password: passwordField.text
    property var emailPattern: /^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$/
    property var passwordPattern: /^(?=.*[A-Za-z])(?=.*\d).{8,}$/
    property var codePattern: /^\d{6}$/
    property bool nameValid: root.displayName.trim().length >= 2 && root.displayName.trim().length <= 20
    property bool emailValid: emailPattern.test(root.email)
    property bool codeValid: codePattern.test(root.code)
    property bool passwordValid: passwordPattern.test(root.password)
    property bool formValid: root.nameValid && root.emailValid && root.codeValid && root.passwordValid

    signal registerRequested(string displayName, string email, string code, string password)

    spacing: 12

    function attemptSubmit() {
        root.showErrors = true
        if (root.formValid) {
            root.registerRequested(root.displayName, root.email, root.code, root.password)
        }
    }

    function resetValidation() {
        root.showErrors = false
        root.nameTouched = false
        root.emailTouched = false
        root.codeTouched = false
        root.passwordTouched = false
    }

    AuthField {
        id: emailField
        label: "邮箱"
        placeholderText: "name@company.com"
        inputMethodHints: Qt.ImhEmailCharactersOnly
        showError: (root.showErrors || root.emailTouched) && !root.emailValid
        errorText: "请输入有效邮箱"

        onEdited: root.emailTouched = true
        onSubmitted: codeInput.forceActiveFocus()
    }

    ColumnLayout {
        spacing: 6

        Text {
            text: "邮箱验证码"
            color: "#374151"
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        RowLayout {
            spacing: 10

            AuthTextField {
                id: codeInput
                Layout.fillWidth: true
                placeholderText: "6 位数字"
                maximumLength: 6
                inputMethodHints: Qt.ImhDigitsOnly
                error: (root.showErrors || root.codeTouched) && !root.codeValid
                onTextChanged: root.codeTouched = true
                Keys.onReturnPressed: passwordField.inputField.forceActiveFocus()
            }

            Button {
                id: sendCodeButton
                text: "发送验证码"
                enabled: root.emailValid && !root.loading
                implicitHeight: 44
                Layout.preferredWidth: 120

                background: Rectangle {
                    color: sendCodeButton.enabled ? "#DBEAFE" : "#F3F4F6"
                    border.color: sendCodeButton.enabled ? "#93C5FD" : "#E5E7EB"
                    border.width: 1
                    radius: 10

                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }
                }

                contentItem: Text {
                    text: sendCodeButton.text
                    color: sendCodeButton.enabled ? "#4338CA" : "#9CA3AF"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: function(mouse) { mouse.accepted = false }
                }
            }
        }

        Text {
            visible: (root.showErrors || root.codeTouched) && !root.codeValid
            text: "请输入 6 位验证码"
            color: "#EF4444"
            font.pixelSize: 12
        }
    }

    AuthField {
        id: passwordField
        label: "密码"
        placeholderText: "至少 8 位"
        echoMode: TextInput.Password
        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
        showError: (root.showErrors || root.passwordTouched) && !root.passwordValid
        errorText: "至少 8 位，包含字母和数字"

        onEdited: root.passwordTouched = true
        onSubmitted: root.attemptSubmit()
    }

    AuthField {
        id: nameField
        label: "用户名"
        placeholderText: "2-20 个字符"
        showError: (root.showErrors || root.nameTouched) && !root.nameValid
        errorText: "用户名需为 2-20 个字符"

        onEdited: root.nameTouched = true
        onSubmitted: root.attemptSubmit()
    }

    PrimaryButton {
        Layout.fillWidth: true
        text: "创建账号"
        loading: root.loading
        enabled: !root.loading
        onClicked: root.attemptSubmit()
    }

    Item { Layout.fillHeight: true }
}
