import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Links
import Links.Backend 1.0

Item {
    id: root

    property bool isGuest: true

    function toLocalFileUrl(path) {
        if (!path || path.length === 0) {
            return ""
        }

        var normalized = path.replace(/\\/g, "/")
        var isWindowsAbsolutePath = /^[A-Za-z]:\//.test(normalized)
        var encodedPath = normalized.split("/").map(function(segment, index) {
            if (segment.length === 0) {
                return segment
            }
            if (isWindowsAbsolutePath && index === 0 && /^[A-Za-z]:$/.test(segment)) {
                return segment
            }
            return encodeURIComponent(segment)
        }).join("/")

        if (isWindowsAbsolutePath) {
            return "file:///" + encodedPath
        }
        if (encodedPath.startsWith("/")) {
            return "file://" + encodedPath
        }
        return "file:///" + encodedPath
    }

    Component.onCompleted: {
        LocalRecordingManager.refreshRecentRecordings()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        ColumnLayout {
            spacing: 6

            Text {
                text: "录制"
                color: Theme.textPrimary
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Text {
                text: "管理本地录制"
                color: Theme.textMuted
                font.pixelSize: 12
            }

            Text {
                text: "保存目录: " + LocalRecordingManager.outputDirectory
                color: Theme.textHint
                font.pixelSize: 11
                elide: Text.ElideMiddle
                Layout.preferredWidth: 560
            }
        }

        ColumnLayout {
            spacing: 10
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "本地录制"
                    color: Theme.textPrimary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                }

                Item { Layout.fillWidth: true }

                LinkButton {
                    text: "刷新列表"
                    onClicked: LocalRecordingManager.refreshRecentRecordings()
                }

                LinkButton {
                    text: "查看录制文件夹"
                    onClicked: LocalRecordingManager.openRecordingFolder()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                color: Theme.cardBackground
                border.color: Theme.borderLight
                border.width: 1

                StackLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    currentIndex: LocalRecordingManager.recentRecordings.length > 0 ? 1 : 0

                    Item {
                        Text {
                            anchors.centerIn: parent
                            text: "暂无本地录制"
                            color: Theme.textMuted
                            font.pixelSize: 12
                        }
                    }

                    ScrollView {
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        ListView {
                            id: recordingsList
                            model: LocalRecordingManager.recentRecordings
                            spacing: 8

                            delegate: Rectangle {
                                width: recordingsList.width
                                height: 66
                                radius: 8
                                color: Theme.windowBackground
                                border.color: Theme.borderLight
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Text {
                                            text: modelData.fileName
                                            color: Theme.textPrimary
                                            font.pixelSize: 12
                                            font.weight: Font.Medium
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }

                                        Text {
                                            text: modelData.createdAtText + "  ·  " + modelData.sizeText
                                            color: Theme.textMuted
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }

                                    LinkButton {
                                        text: "打开"
                                        onClicked: {
                                            var url = root.toLocalFileUrl(modelData.path)
                                            if (url.length > 0) {
                                                Qt.openUrlExternally(url)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
