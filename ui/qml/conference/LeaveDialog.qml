import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

Popup {
    id: root

    modal: true
    width: 360
    // Height will be determined by contentItem's implicitHeight + padding
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 24 // Use padding on Popup instead of margins on layout

    signal confirmClicked()
    signal cancelClicked()

    background: Rectangle {
        color: Theme.popupBackground
        radius: 16
        border.color: Theme.popupBorder
        border.width: 1

        // Shadow effect
        layer.enabled: true
        layer.effect: null // Placeholder for shadow if needed, but keeping it clean
    }

    contentItem: ColumnLayout {
        spacing: 32 // More space between sections

        // Text content
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: "结束会议"
                color: Theme.textPrimary
                font.pixelSize: 18
                font.weight: Font.Bold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: "您确定要离开当前会议吗？"
                color: Theme.textMuted
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }

        // Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                id: cancelButton
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                background: Rectangle {
                    color: cancelButton.down ? Theme.activeBackground : (cancelButton.hovered ? Theme.hoverBackground : "transparent")
                    radius: 8
                    border.color: Theme.borderColor
                    border.width: 1
                }

                contentItem: Text {
                    text: "取消"
                    color: Theme.textPrimary
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.cancelClicked()
                    root.close()
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: function(mouse) { mouse.accepted = false }
                }
            }

            Button {
                id: confirmButton
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                background: Rectangle {
                    color: confirmButton.down ? "#991B1B" : (confirmButton.hovered ? "#B91C1C" : "#DC2626")
                    radius: 8
                }

                contentItem: Text {
                    text: "退出"
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.confirmClicked()
                    root.close()
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: function(mouse) { mouse.accepted = false }
                }
            }
        }
    }
}