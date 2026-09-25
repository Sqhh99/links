import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

TextField {
    id: root

    implicitHeight: 40

    color: Theme.inputText
    placeholderTextColor: Theme.inputPlaceholder
    selectionColor: Theme.brand
    selectedTextColor: "#FFFFFF"
    font.pixelSize: 14
    leftPadding: 12
    rightPadding: 12

    background: Rectangle {
        color: Theme.inputBackground
        border.color: root.activeFocus ? Theme.inputBorderFocus
                                       : (root.hovered ? Theme.borderColor : Theme.inputBorder)
        border.width: root.activeFocus ? 1.5 : 1
        radius: 8

        Behavior on border.color {
            ColorAnimation { duration: 120 }
        }
    }
}
