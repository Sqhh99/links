import QtQuick
import QtQuick.Controls

CheckBox {
    id: control
    
    implicitHeight: 24
    
    indicator: Rectangle {
        implicitWidth: 20
        implicitHeight: 20
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 6
        border.color: control.checked ? "#2563EB" : (control.hovered ? "#2563EB" : "#D1D5DB")
        border.width: 1
        color: control.checked ? "#2563EB" : "#FFFFFF"

        Behavior on color { ColorAnimation { duration: 100 } }
        Behavior on border.color { ColorAnimation { duration: 100 } }

        // Checkmark
        Text {
            anchors.centerIn: parent
            text: "✓"
            color: "white"
            font.pixelSize: 14
            font.weight: Font.Bold
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: "#374151"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
}
