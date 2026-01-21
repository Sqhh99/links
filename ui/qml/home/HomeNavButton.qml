import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    property bool active: false
    property string iconSource: ""

    Layout.fillWidth: true
    implicitHeight: 42
    checkable: true
    checked: active

    background: Rectangle {
        color: root.checked ? "#EEF2FF" : "transparent"
        radius: 10
    }

    contentItem: RowLayout {
        spacing: 10
        anchors.fill: parent
        anchors.leftMargin: 12

        Rectangle {
            width: 22
            height: 22
            radius: 6
            color: root.active ? "#DBEAFE" : "#F3F4F6"

            Image {
                source: root.iconSource
                sourceSize.width: 14
                sourceSize.height: 14
                anchors.centerIn: parent
                opacity: root.active ? 0.9 : 0.6
            }
        }

        Text {
            text: root.text
            color: root.active ? "#1D4ED8" : "#4B5563"
            font.pixelSize: 13
            font.weight: root.active ? Font.DemiBold : Font.Medium
            Layout.fillWidth: true
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
