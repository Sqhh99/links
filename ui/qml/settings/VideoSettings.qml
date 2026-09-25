import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links as Comp
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
            title: "摄像头"

            SettingsRow {
                label: "设备"
                divider: false

                Comp.ComboBox {
                    id: cameraCombo
                    Layout.preferredWidth: 240
                    model: backend ? backend.cameras : []
                    textRole: "name"
                    valueRole: "id"

                    currentIndex: {
                        if (!backend) return 0
                        var idx = backend.findDeviceIndex(backend.cameras, backend.selectedCameraId)
                        return idx >= 0 ? idx : 0
                    }

                    onActivated: {
                        if (backend && currentIndex >= 0) {
                            backend.selectedCameraId = currentValue
                        }
                    }
                }
            }

            SettingsRow {
                label: "分辨率"

                Comp.ComboBox {
                    id: resolutionCombo
                    Layout.preferredWidth: 240
                    model: backend ? backend.resolutions : []

                    currentIndex: backend ? backend.selectedResolutionIndex : 0

                    onActivated: {
                        if (backend) {
                            backend.selectedResolutionIndex = currentIndex
                        }
                    }
                }
            }
        }

        SettingsSection {
            title: "性能"

            SettingsRow {
                label: "硬件加速"
                description: "使用 GPU 进行视频编解码，降低 CPU 占用"
                divider: false

                ToggleSwitch {
                    id: hardwareCheck
                    checked: backend ? backend.hardwareAccel : true
                    onCheckedChanged: {
                        if (backend) backend.hardwareAccel = checked
                    }
                }
            }
        }
    }
}
