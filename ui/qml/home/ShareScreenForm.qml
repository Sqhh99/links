import QtQuick
import QtQuick.Layouts
import Links

ColumnLayout {
    id: root

    property bool loading: false

    signal shareClicked()

    spacing: 12

    Text {
        text: "共享屏幕"
        color: "#111827"
        font.pixelSize: 14
        font.weight: Font.DemiBold
    }

    Text {
        text: "选择要共享的窗口或屏幕，其他人将看到你的实时画面。"
        color: "#6B7280"
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    CheckBox {
        text: "共享系统音频"
    }

    PrimaryButton {
        Layout.fillWidth: true
        text: "开始共享"
        loading: root.loading
        enabled: !root.loading
        onClicked: root.shareClicked()
    }

    Item { Layout.fillHeight: true }
}
