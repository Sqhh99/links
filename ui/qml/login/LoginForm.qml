import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Links

ColumnLayout {
    id: root

    property bool loading: false
    property bool showErrors: false
    property bool usernameTouched: false
    property bool passwordTouched: false
    property string username: usernameField.text.trim()
    property string password: passwordField.text
    // The server applies the username rules only when it creates an account,
    // so accounts with older usernames (e.g. an email address) can still sign in.
    property bool usernameValid: root.username.length > 0
    property var passwordPattern: /^(?=.*[A-Za-z])(?=.*\d).{8,}$/
    property bool passwordValid: passwordPattern.test(root.password)
    property bool formValid: root.usernameValid && root.passwordValid

    signal loginRequested(string username, string password)

    spacing: 16

    function attemptSubmit() {
        root.showErrors = true
        if (root.formValid) {
            root.loginRequested(root.username, root.password)
        }
    }

    function resetValidation() {
        root.showErrors = false
        root.usernameTouched = false
        root.passwordTouched = false
    }

    AuthField {
        id: usernameField
        label: "用户名"
        placeholderText: "2-32 个字符：字母、数字、_ - ."
        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
        showError: (root.showErrors || root.usernameTouched) && !root.usernameValid
        errorText: "请输入用户名"

        onEdited: root.usernameTouched = true
        onSubmitted: passwordField.inputField.forceActiveFocus()
    }

    AuthField {
        id: passwordField
        label: "密码"
        placeholderText: "至少 8 位，包含字母和数字"
        echoMode: TextInput.Password
        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
        showError: (root.showErrors || root.passwordTouched) && !root.passwordValid
        errorText: "至少 8 位，包含字母和数字"

        onEdited: root.passwordTouched = true
        onSubmitted: root.attemptSubmit()
    }

    PrimaryButton {
        Layout.fillWidth: true
        Layout.topMargin: 4
        text: "登录"
        loading: root.loading
        enabled: !root.loading
        onClicked: root.attemptSubmit()
    }

    Item { Layout.fillHeight: true }
}
