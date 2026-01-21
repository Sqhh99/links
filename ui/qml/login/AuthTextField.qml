import QtQuick
import QtQuick.Controls

TextField {
    id: root

    property bool error: false

    implicitHeight: 44
    color: "#111827"
    placeholderTextColor: "#C7CCD5"
    selectionColor: "#2563EB"
    font.pixelSize: 15
    leftPadding: 16
    rightPadding: 16

    background: Rectangle {
        color: "#FFFFFF"
        border.color: root.error ? "#EF4444" : (root.activeFocus ? "#2563EB" : "#D1D5DB")
        border.width: 1
        radius: 10

        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
    }
}
