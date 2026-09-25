import QtQuick
import Links
import QtQuick.Controls
import QtQuick.Layouts
import Links.Backend 1.0

Rectangle {
    id: root

    property string title: "Links"
    property string iconName: "video"
    property bool showTitle: true
    property bool showSettingsButton: true

    signal settingsClicked()
    signal minimizeClicked()
    signal closeClicked()

    height: 40
    color: "transparent"

    // Drag handler for window movement
    property var targetWindow: null
    property point dragStartPos
    property bool dragging: false

    MouseArea {
        anchors.fill: parent
        z: -1  // Ensure this is behind the buttons
        hoverEnabled: false  // Don't interfere with button hover states

        onPressed: function(mouse) {
            if (root.targetWindow) {
                root.dragging = true
                root.dragStartPos = Qt.point(mouse.x, mouse.y)
            }
        }

        onPositionChanged: function(mouse) {
            if (root.dragging && root.targetWindow) {
                var delta = Qt.point(mouse.x - root.dragStartPos.x,
                                     mouse.y - root.dragStartPos.y)
                root.targetWindow.x += delta.x
                root.targetWindow.y += delta.y
            }
        }

        onReleased: {
            root.dragging = false
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 8
        spacing: 8

        Rectangle {
            visible: root.showTitle
            Layout.preferredWidth: 22
            Layout.preferredHeight: 22
            radius: 6
            color: Theme.brand

            Icon {
                anchors.centerIn: parent
                name: root.iconName
                size: 13
                color: "#FFFFFF"
            }
        }

        Text {
            visible: root.showTitle
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        Item { Layout.fillWidth: true }

        IconButton {
            id: settingsBtn
            visible: root.showSettingsButton
            iconName: "settings"
            toolTipText: "设置"
            onClicked: root.settingsClicked()
        }

        IconButton {
            id: minimizeBtn
            iconName: "minus"
            onClicked: root.minimizeClicked()
        }

        IconButton {
            id: closeBtn
            iconName: "x"
            hoverColor: Theme.closeHoverColor
            hoverIconColor: "#FFFFFF"
            onClicked: root.closeClicked()
        }
    }
}
