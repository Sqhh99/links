import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

TextField {
    id: root
    
    implicitHeight: 44
    
    color: Theme.inputText
    placeholderTextColor: Theme.inputPlaceholder
    selectionColor: Theme.accentColor
    font.pixelSize: 15
    leftPadding: 16
    rightPadding: 16
    
    background: Rectangle {
        color: Theme.inputBackground
        border.color: root.activeFocus ? Theme.inputBorderFocus : Theme.inputBorder
        border.width: 1
        radius: 10
        
        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
}
