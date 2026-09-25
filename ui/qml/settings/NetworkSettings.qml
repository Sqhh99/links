import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links
import Links.Backend 1.0

ScrollView {
    id: root

    property SettingsBackend backend

    contentWidth: availableWidth
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
        width: root.availableWidth
        spacing: 20

        SettingsSection {
            title: "服务器"

            SettingsRow {
                label: "信令地址"
                description: "会议服务的 API / 信令服务器地址"
                stacked: true
                divider: false

                TextField {
                    id: apiUrlInput
                    Layout.fillWidth: true
                    placeholderText: "例如 wss://example.com"
                    text: backend ? backend.apiUrl : ""

                    onTextChanged: {
                        if (backend) {
                            backend.apiUrl = text
                        }
                    }
                }
            }
        }
    }
}
