import QtQuick
import Links
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

// Icon-only navigation button for the home rail
Button {
    id: root

    property bool active: false
    property string iconName: ""

    Layout.alignment: Qt.AlignHCenter
    implicitWidth: 64
    implicitHeight: 44
    padding: 0
    checkable: true
    checked: active

    background: Item {
        // Active indicator bar on the rail's left edge
        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 3
            height: root.active ? 20 : 0
            radius: 2
            color: Theme.brand

            Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        }

        Rectangle {
            anchors.centerIn: parent
            width: 40
            height: 40
            radius: Theme.radiusMd
            color: root.active ? Theme.brandSoft : (root.hovered ? Theme.hoverBackground : Theme.hoverClear)

            Behavior on color { ColorAnimation { duration: 120 } }
        }
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: 21
            color: root.active ? Theme.brand : (root.hovered ? Theme.iconPrimary : Theme.iconSecondary)
        }
    }

    ToolTip.visible: hovered && text.length > 0
    ToolTip.text: text
    ToolTip.delay: 400

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
