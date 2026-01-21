import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

Popup {
    id: root

    signal loginRequested(string email, string password)
    signal registerRequested(string displayName, string email, string code, string password)

    modal: true
    focus: true
    width: 420
    height: 560
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    Overlay.modal: Rectangle {
        color: "#00000066"
    }

    background: Rectangle {
        color: "transparent"
    }

    contentItem: Item {
        anchors.fill: parent

        LoginCard {
            anchors.fill: parent
            onLoginRequested: root.loginRequested(email, password)
            onRegisterRequested: root.registerRequested(displayName, email, code, password)
        }
    }
}
