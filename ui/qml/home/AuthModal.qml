import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

Popup {
    id: root

    property var authBackend: null

    modal: true
    focus: true
    parent: Overlay.overlay
    width: 400
    height: Math.min(440, (parent ? parent.height : 580) - 24)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    Overlay.modal: Rectangle {
        color: Theme.overlayColor
    }

    background: Rectangle {
        color: "transparent"
    }

    contentItem: Item {
        anchors.fill: parent

        LoginCard {
            id: loginCard
            anchors.fill: parent
            authBackend: root.authBackend
        }
    }
}
