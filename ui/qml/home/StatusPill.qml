import QtQuick
import Links

// Small rounded status tag for meeting rows
Rectangle {
    id: root

    property string text: ""
    property bool live: false
    property bool upcoming: false

    implicitWidth: label.implicitWidth + 14
    implicitHeight: 20
    radius: 10
    color: live ? Theme.brand : (upcoming ? Theme.warningSoft : Theme.hoverBackground)

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.live ? "#FFFFFF" : (root.upcoming ? Theme.warning : Theme.textTertiary)
        font.pixelSize: 11
        font.weight: Font.Medium
    }
}
