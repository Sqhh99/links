import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root

    property url avatarSource: ""
    property string initials: ""
    property color accentColor: "#2563EB"
    readonly property bool hasAvatar: avatarSource.toString().length > 0

    signal picked()

    implicitWidth: 120
    implicitHeight: 104

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Rectangle {
            id: avatar
            Layout.alignment: Qt.AlignHCenter
            width: 72
            height: 72
            radius: 36
            color: root.hasAvatar ? "#EFF6FF" : "#F3F4F6"
            border.color: root.hasAvatar ? root.accentColor : "#D1D5DB"
            border.width: 1
            clip: true

            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }

            Image {
                anchors.fill: parent
                source: root.avatarSource
                visible: root.hasAvatar
                fillMode: Image.PreserveAspectCrop
            }

            Text {
                anchors.centerIn: parent
                text: root.initials.length > 0 ? root.initials : "+"
                color: root.hasAvatar ? root.accentColor : "#9CA3AF"
                font.pixelSize: root.hasAvatar ? 18 : 26
                font.weight: Font.DemiBold
                visible: !root.hasAvatar
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    fileDialog.open()
                }
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.hasAvatar ? "更换头像" : "上传头像"
            color: "#6B7280"
            font.pixelSize: 12
        }
    }

    FileDialog {
        id: fileDialog
        title: "选择头像"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.bmp *.gif)"]
        onAccepted: {
            root.avatarSource = selectedFile
            root.picked()
        }
    }
}
