import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Links

ColumnLayout {
    id: root

    property bool loading: false
    property bool showErrors: false
    property bool emailTouched: false
    property bool passwordTouched: false
    property string email: emailField.text
    property string password: passwordField.text
    property var emailPattern: /^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$/
    property var passwordPattern: /^(?=.*[A-Za-z])(?=.*\d).{8,}$/
    property bool emailValid: emailPattern.test(root.email)
    property bool passwordValid: passwordPattern.test(root.password)
    property bool formValid: root.emailValid && root.passwordValid

    signal loginRequested(string email, string password)

    spacing: 16

    function attemptSubmit() {
        root.showErrors = true
        if (root.formValid) {
            root.loginRequested(root.email, root.password)
        }
    }

    function resetValidation() {
        root.showErrors = false
        root.emailTouched = false
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
        onSubmitted: passwordField.inputField.forceActiveFocus()
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

    RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Item { Layout.fillWidth: true }

        LinkButton {
            text: "忘记密码？"
        }
    }

    PrimaryButton {
        Layout.fillWidth: true
        text: "登录"
        loading: root.loading
        enabled: !root.loading
        onClicked: root.attemptSubmit()
    }

    Item { Layout.fillHeight: true }
}
