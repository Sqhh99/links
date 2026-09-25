import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls
import Links
import Links.Backend 1.0

// Solid brand tile with a label underneath (Tencent Meeting style)
Item {
    id: root

    property string title: ""
    property string iconName: ""
    property bool locked: false
    property string toolTipText: ""

    signal clicked()

    implicitWidth: 104
    implicitHeight: 104

    readonly property bool hovered: hover.hovered
    readonly property bool pressed: tap.pressed

    Rectangle {
        id: tile
        width: 64
        height: 64
        radius: 18
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        color: root.pressed ? Theme.brandPressed : (root.hovered ? Theme.brandHover : Theme.brand)
        scale: root.pressed ? 0.96 : (root.hovered ? 1.04 : 1.0)

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: Qt.rgba(Theme.brand.r, Theme.brand.g, Theme.brand.b, root.hovered ? 0.38 : 0.22)
            shadowBlur: 0.7
            shadowVerticalOffset: root.hovered ? 6 : 4
        }

        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: 28
            color: "#FFFFFF"
        }

        // Lock badge for actions that need login
        Rectangle {
            visible: root.locked
            width: 20
            height: 20
            radius: 10
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: -5
            anchors.topMargin: -5
            color: Theme.cardBackground
            border.color: Theme.borderLight
            border.width: 1

            Icon {
                anchors.centerIn: parent
                name: "lock"
                size: 11
                color: Theme.iconSecondary
            }
        }
    }

    Text {
        anchors.top: tile.bottom
        anchors.topMargin: 12
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.title
        color: Theme.textPrimary
        font.pixelSize: 13
        font.weight: Font.Medium
    }

    HoverHandler {
        id: hover
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        id: tap
        onTapped: root.clicked()
    }

    ToolTip.visible: root.toolTipText.length > 0 && root.hovered
    ToolTip.text: root.toolTipText
    ToolTip.delay: 600
}
