import QtQuick
import Links
import QtQuick.Controls

TextField {
    id: root

    property bool error: false

    implicitHeight: 40
    color: Theme.textPrimary
    placeholderTextColor: Theme.inputPlaceholder
    selectionColor: Theme.brand
    font.pixelSize: 14
    leftPadding: 12
    rightPadding: 12

    background: Rectangle {
        color: Theme.inputBackground
        border.color: root.error ? Theme.danger : (root.activeFocus ? Theme.brand : Theme.inputBorder)
        border.width: 1
        radius: 8

        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
    }
}
