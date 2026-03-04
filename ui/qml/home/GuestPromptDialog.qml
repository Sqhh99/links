import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

Popup {
    id: root

    property string titleText: "需要登录"
    property string messageText: ""
    property string primaryText: "登录/注册"
    property string secondaryText: "先用游客模式"
    property bool showCancel: false
    property string cancelText: "取消"
    property bool showOptOut: true
    property string optOutText: "7 天内不再提示"

    signal primaryClicked()
    signal secondaryClicked()
    signal cancelClicked()
    signal optOutChanged(bool checked)

    modal: true
    focus: true
    width: 360
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    Overlay.modal: Rectangle {
        color: "#00000066"
    }

    background: Rectangle {
        color: "transparent"
    }

    contentItem: Rectangle {
        id: dialogCard
        implicitHeight: dialogLayout.implicitHeight + 52
        radius: 16
        color: Theme.windowBackground
        border.color: Theme.borderLight
        border.width: 1

        ColumnLayout {
            id: dialogLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 20
            spacing: 14

            Text {
                text: root.titleText
                color: Theme.textPrimary
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Text {
                text: root.messageText
                color: Theme.textMuted
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            CheckBox {
                visible: root.showOptOut
                text: root.optOutText
                onCheckedChanged: root.optOutChanged(checked)
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Layout.topMargin: 8

                Loader {
                    visible: root.showCancel
                    active: root.showCancel
                    Layout.preferredWidth: root.showCancel ? 48 : 0
                    Layout.preferredHeight: root.showCancel ? 26 : 0
                    sourceComponent: LinkButton {
                        text: root.cancelText
                        onClicked: {
                            root.cancelClicked()
                            root.close()
                        }
                    }
                }

                SecondaryButton {
                    text: root.secondaryText
                    Layout.fillWidth: true
                    onClicked: {
                        root.secondaryClicked()
                        root.close()
                    }
                }

                PrimaryButton {
                    text: root.primaryText
                    Layout.fillWidth: true
                    onClicked: {
                        root.primaryClicked()
                        root.close()
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 10
            }
        }
    }
}
