import QtQuick
import QtQuick.Layouts
import Links

Rectangle {
    id: root

    property string title: "登录解锁功能"
    property string message: ""
    property string primaryText: "登录/注册"
    property string secondaryText: "先用游客模式"
    property bool showSecondary: true
    property int primaryWidth: 0
    property bool centerPrimary: false

    signal primaryClicked()
    signal secondaryClicked()

    radius: 12
    color: "#F8FAFC"
    border.color: "#E5E7EB"
    border.width: 1

    implicitHeight: 140

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Text {
            text: root.title
            color: "#111827"
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }

        Text {
            text: root.message
            color: "#6B7280"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Loader {
                active: root.showSecondary
                Layout.fillWidth: true
                sourceComponent: SecondaryButton {
                    text: root.secondaryText
                    Layout.fillWidth: true
                    onClicked: root.secondaryClicked()
                }
            }

            Item {
                visible: root.centerPrimary && !root.showSecondary
                Layout.fillWidth: true
            }

            PrimaryButton {
                text: root.primaryText
                Layout.fillWidth: root.primaryWidth <= 0
                Layout.preferredWidth: root.primaryWidth > 0 ? root.primaryWidth : -1
                Layout.alignment: root.centerPrimary ? Qt.AlignHCenter : Qt.AlignLeft
                onClicked: root.primaryClicked()
            }
        }
    }
}
