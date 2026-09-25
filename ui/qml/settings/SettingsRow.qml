import QtQuick
import QtQuick.Layouts
import Links

// One row inside a SettingsSection: label (+ optional description) on the
// left, control on the right. `stacked` puts the control below the label.
Item {
    id: root

    property string label: ""
    property string description: ""
    property bool stacked: false
    property bool divider: true
    default property alias control: slot.data

    Layout.fillWidth: true
    implicitHeight: Math.max(52, grid.implicitHeight + 24)

    Rectangle {
        visible: root.divider
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        height: 1
        color: Theme.separatorColor
    }

    GridLayout {
        id: grid
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        columns: root.stacked ? 1 : 2
        columnSpacing: 16
        rowSpacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: root.label
                color: Theme.textPrimary
                font.pixelSize: 13
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Text {
                visible: root.description.length > 0
                text: root.description
                color: Theme.textMuted
                font.pixelSize: 11
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        RowLayout {
            id: slot
            Layout.fillWidth: root.stacked
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            spacing: 8
        }
    }
}
