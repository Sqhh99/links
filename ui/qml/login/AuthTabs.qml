import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root

    property string mode: "login"

    signal modeSelected(string mode)

    spacing: 18
    Layout.fillWidth: true

    Item {
        id: loginTab
        Layout.preferredHeight: 30
        implicitWidth: loginLabel.implicitWidth + 16
        Layout.preferredWidth: implicitWidth

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 0

            Text {
                id: loginLabel
                text: "登录"
                color: root.mode === "login" ? "#2563EB" : "#94A3B8"
                font.pixelSize: 13
                font.weight: root.mode === "login" ? Font.DemiBold : Font.Medium
            }

            Item {
                Layout.preferredHeight: 4
            }

            Rectangle {
                width: loginTab.width
                height: 1
                radius: 1
                color: root.mode === "login" ? "#2563EB" : "#E5E7EB"
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (root.mode === "login") return
                root.modeSelected("login")
            }
        }
    }

    Item {
        id: registerTab
        Layout.preferredHeight: 30
        implicitWidth: registerLabel.implicitWidth + 16
        Layout.preferredWidth: implicitWidth

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 0

            Text {
                id: registerLabel
                text: "注册"
                color: root.mode === "register" ? "#2563EB" : "#94A3B8"
                font.pixelSize: 13
                font.weight: root.mode === "register" ? Font.DemiBold : Font.Medium
            }

            Item {
                Layout.preferredHeight: 4
            }

            Rectangle {
                width: registerTab.width
                height: 1
                radius: 1
                color: root.mode === "register" ? "#2563EB" : "#E5E7EB"
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (root.mode === "register") return
                root.modeSelected("register")
            }
        }
    }

    Item { Layout.fillWidth: true }
}
