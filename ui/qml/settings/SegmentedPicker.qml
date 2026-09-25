import QtQuick
import QtQuick.Layouts
import Links

// Small segmented selector for a handful of mutually exclusive options
Rectangle {
    id: root

    property var options: []
    property int currentIndex: 0
    property int segmentWidth: 56

    signal selected(int index)

    implicitWidth: row.implicitWidth + 6
    implicitHeight: 30
    radius: 8
    color: Theme.tabInactiveBg

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: 3
        spacing: 2

        Repeater {
            model: root.options

            Rectangle {
                id: segment
                required property int index
                required property var modelData
                readonly property bool active: root.currentIndex === index

                Layout.preferredWidth: root.segmentWidth
                Layout.fillHeight: true
                radius: 6
                color: active ? Theme.tabActiveBg : (segHover.hovered ? Theme.hoverBackground : Theme.hoverClear)
                border.color: active && !Theme.isDark ? Theme.borderLight : "transparent"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 120 } }

                Text {
                    anchors.centerIn: parent
                    text: segment.modelData
                    color: segment.active ? Theme.brand : Theme.textTertiary
                    font.pixelSize: 12
                    font.weight: segment.active ? Font.DemiBold : Font.Normal
                }

                HoverHandler {
                    id: segHover
                    cursorShape: Qt.PointingHandCursor
                }

                TapHandler {
                    onTapped: root.selected(segment.index)
                }
            }
        }
    }
}
