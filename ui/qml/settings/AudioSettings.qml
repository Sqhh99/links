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
            title: "设备"

            SettingsRow {
                label: "麦克风"
                divider: false

                Comp.ComboBox {
                    id: micCombo
                    Layout.preferredWidth: 240
                    model: backend ? backend.microphones : []
                    textRole: "name"
                    valueRole: "id"

                    currentIndex: {
                        if (!backend) return 0
                        var idx = backend.findDeviceIndex(backend.microphones, backend.selectedMicId)
                        return idx >= 0 ? idx : 0
                    }

                    onActivated: {
                        if (backend && currentIndex >= 0) {
                            backend.selectedMicId = currentValue
                        }
                    }
                }
            }

            SettingsRow {
                label: "扬声器"

                Comp.ComboBox {
                    id: speakerCombo
                    Layout.preferredWidth: 240
                    model: backend ? backend.speakers : []
                    textRole: "name"
                    valueRole: "id"

                    currentIndex: {
                        if (!backend) return 0
                        var idx = backend.findDeviceIndex(backend.speakers, backend.selectedSpeakerId)
                        return idx >= 0 ? idx : 0
                    }

                    onActivated: {
                        if (backend && currentIndex >= 0) {
                            backend.selectedSpeakerId = currentValue
                        }
                    }
                }
            }
        }

        // =============================================
        // Basic layer: simple on/off toggles
        // =============================================
        SettingsSection {
            title: "音频处理"

            SettingsRow {
                label: "回声消除"
                description: "AEC，消除扬声器声音被麦克风再次采集"
                divider: false

                ToggleSwitch {
                    id: echoCheck
                    checked: backend ? backend.echoCancel : true
                    onCheckedChanged: {
                        if (backend) backend.echoCancel = checked
                    }
                }
            }

            SettingsRow {
                label: "噪声抑制"
                description: "NS，降低键盘、风扇等环境噪音"

                ToggleSwitch {
                    id: noiseCheck
                    checked: backend ? backend.noiseSuppression : true
                    onCheckedChanged: {
                        if (backend) backend.noiseSuppression = checked
                    }
                }
            }

            SettingsRow {
                label: "自动增益控制"
                description: "AGC，自动平衡说话音量"

                ToggleSwitch {
                    id: agcCheck
                    checked: backend ? backend.autoGainControl : true
                    onCheckedChanged: {
                        if (backend) backend.autoGainControl = checked
                    }
                }
            }

            SettingsRow {
                label: "高通滤波器"
                description: "HPF，滤除低频嗡嗡声"

                ToggleSwitch {
                    id: hpfCheck
                    checked: backend ? backend.highPassFilter : true
                    onCheckedChanged: {
                        if (backend) backend.highPassFilter = checked
                    }
                }
            }
        }

        // =============================================
        // Advanced layer: collapsible panel
        // =============================================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            // Toggle button for advanced settings
            RowLayout {
                Layout.fillWidth: false
                Layout.leftMargin: 4
                spacing: 4

                Icon {
                    name: "chevron-right"
                    size: 14
                    color: Theme.iconSecondary
                    rotation: advancedPanel.visible ? 90 : 0

                    Behavior on rotation { NumberAnimation { duration: 150 } }
                }

                Text {
                    text: "高级音频设置"
                    color: Theme.textTertiary
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: advancedPanel.visible = !advancedPanel.visible }
            }

            // Advanced settings panel (collapsed by default)
            SettingsSection {
                id: advancedPanel
                visible: false

                // -- Echo Cancellation Advanced --
                SettingsRow {
                    label: "回声消除增强"
                    description: "增强模式，强制开启高通滤波"
                    divider: false
                    visible: backend ? backend.echoCancel : false

                    ToggleSwitch {
                        checked: backend ? backend.echoEnhancedFilter : true
                        onCheckedChanged: {
                            if (backend) backend.echoEnhancedFilter = checked
                        }
                    }
                }

                // -- Noise Suppression Level --
                SettingsRow {
                    label: "噪声抑制强度"
                    description: "强度越高，噪音压得越狠，但语音可能更失真"
                    divider: backend ? backend.echoCancel : false
                    visible: backend ? backend.noiseSuppression : false

                    SegmentedPicker {
                        options: ["低", "中", "高", "极高"]
                        segmentWidth: 40
                        currentIndex: backend ? backend.nsLevel : 0
                        onSelected: function(index) {
                            if (backend) backend.nsLevel = index
                        }
                    }
                }

                // -- AGC Mode --
                SettingsRow {
                    label: "增益控制模式"
                    divider: backend ? (backend.echoCancel || backend.noiseSuppression) : false
                    visible: backend ? backend.autoGainControl : false

                    SegmentedPicker {
                        options: ["自适应", "固定增益"]
                        segmentWidth: 64
                        currentIndex: backend ? backend.agcMode : 0
                        onSelected: function(index) {
                            if (backend) backend.agcMode = index
                        }
                    }
                }

                // Adaptive mode: max gain slider
                SettingsRow {
                    stacked: true
                    label: "最大自适应增益: " + (backend ? backend.adaptiveDigitalMaxGainDb.toFixed(0) : "50") + " dB"
                    description: "值越大，安静环境下放大越多（可能放大环境噪音）"
                    visible: backend ? (backend.autoGainControl && backend.agcMode === 0) : false

                    BrandSlider {
                        Layout.fillWidth: true
                        from: 0
                        to: 50
                        stepSize: 1
                        value: backend ? backend.adaptiveDigitalMaxGainDb : 50
                        onMoved: {
                            if (backend) backend.adaptiveDigitalMaxGainDb = value
                        }
                    }
                }

                // Fixed mode: gain slider
                SettingsRow {
                    stacked: true
                    label: "固定数字增益: " + (backend ? backend.fixedDigitalGainDb.toFixed(0) : "0") + " dB"
                    visible: backend ? (backend.autoGainControl && backend.agcMode === 1) : false

                    BrandSlider {
                        Layout.fillWidth: true
                        from: 0
                        to: 50
                        stepSize: 1
                        value: backend ? backend.fixedDigitalGainDb : 0
                        onMoved: {
                            if (backend) backend.fixedDigitalGainDb = value
                        }
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 4 }
    }
}
