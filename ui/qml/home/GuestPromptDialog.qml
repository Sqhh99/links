import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

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
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    Overlay.modal: Rectangle {
        color: "#00000066"
    }

    background: Rectangle {
        color: "transparent"
    }

    contentItem: Rectangle {
        radius: 16
        color: "#FFFFFF"
        border.color: "#E5E7EB"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Text {
                text: root.titleText
                color: "#111827"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Text {
                text: root.messageText
                color: "#6B7280"
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

                Loader {
                    active: root.showCancel
                    Layout.preferredWidth: 48
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
        }
    }
}
