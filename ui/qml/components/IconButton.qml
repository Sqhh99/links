import QtQuick
import QtQuick.Controls
import Links

Button {
    id: root

    property string iconName: ""
    property int iconSize: 16
    property string toolTipText: ""
    property color iconColor: Theme.iconSecondary
    property color hoverColor: Theme.hoverBackground
    property color hoverIconColor: Theme.iconPrimary

    implicitWidth: 28
    implicitHeight: 28
    padding: 0

    background: Rectangle {
        color: root.pressed ? Qt.darker(root.hoverColor, 1.06)
                            : (root.hovered ? root.hoverColor : Theme.clearOf(root.hoverColor))
        radius: Theme.radiusSm

        Behavior on color { ColorAnimation { duration: 120 } }
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: root.iconSize
            color: root.hovered ? root.hoverIconColor : root.iconColor
        }
    }

    ToolTip.visible: toolTipText.length > 0 && hovered
    ToolTip.text: toolTipText
    ToolTip.delay: 500

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
