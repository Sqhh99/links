import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

// Neutral outline button (cancel / secondary actions)
Button {
    id: root

    // "outline" (neutral bordered) or "soft" (brand tinted)
    property string variant: "outline"

    implicitHeight: 40
    leftPadding: 18
    rightPadding: 18

    background: Rectangle {
        radius: 8
        color: {
            if (!root.enabled) return Theme.disabledBg
            if (root.variant === "soft")
                return root.hovered ? Theme.secondaryHoverBg : Theme.secondaryBg
            return root.pressed ? Theme.pressedBackground
                                : (root.hovered ? Theme.buttonCancelHoverBg : Theme.buttonCancelBg)
        }
        border.color: root.variant === "soft" ? "transparent" : Theme.buttonCancelBorder
        border.width: root.variant === "soft" ? 0 : 1

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }

    contentItem: Text {
        text: root.text
        color: !root.enabled ? Theme.disabledText
                             : (root.variant === "soft" ? Theme.secondaryText : Theme.buttonCancelText)
        font.pixelSize: 14
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
