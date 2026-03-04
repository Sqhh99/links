import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links as Comp
import Links.Backend 1.0

ScrollView {
    id: root
    
    property SettingsBackend backend
    
    contentWidth: availableWidth
    clip: true
    
    ColumnLayout {
        width: parent.width
        spacing: 24
        
        // Microphone
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Text {
                text: "麦克风"
                color: "#374151"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
            
            Comp.ComboBox {
                id: micCombo
                Layout.fillWidth: true
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
        
        // Speaker
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Text {
                text: "扬声器"
                color: "#374151"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
            
            Comp.ComboBox {
                id: speakerCombo
                Layout.fillWidth: true
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
        
        // =============================================
        // Basic layer: simple on/off toggles
        // =============================================
        ColumnLayout {
            spacing: 12
            
            Text {
                text: "音频处理"
                color: "#374151"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
            
            Comp.CheckBox {
                id: echoCheck
                text: "回声消除 (AEC)"
                checked: backend ? backend.echoCancel : true
                onCheckedChanged: {
                    if (backend) backend.echoCancel = checked
                }
            }
            
            Comp.CheckBox {
                id: noiseCheck
                text: "噪声抑制 (NS)"
                checked: backend ? backend.noiseSuppression : true
                onCheckedChanged: {
                    if (backend) backend.noiseSuppression = checked
                }
            }
            
            Comp.CheckBox {
                id: agcCheck
                text: "自动增益控制 (AGC)"
                checked: backend ? backend.autoGainControl : true
                onCheckedChanged: {
                    if (backend) backend.autoGainControl = checked
                }
            }
            
            Comp.CheckBox {
                id: hpfCheck
                text: "高通滤波器 (HPF)"
                checked: backend ? backend.highPassFilter : true
                onCheckedChanged: {
                    if (backend) backend.highPassFilter = checked
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
                spacing: 6
                
                Text {
                    text: advancedPanel.visible ? "▼" : "▶"
                    color: "#6B7280"
                    font.pixelSize: 11
                }
                
                Text {
                    text: "高级音频设置"
                    color: "#6B7280"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: advancedPanel.visible = !advancedPanel.visible
                    }
                }
            }
            
            // Advanced settings panel (collapsed by default)
            ColumnLayout {
                id: advancedPanel
                visible: false
                Layout.fillWidth: true
                Layout.leftMargin: 12
                spacing: 16
                
                // -- Echo Cancellation Advanced --
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: backend ? backend.echoCancel : false
                    
                    Text {
                        text: "回声消除增强"
                        color: "#4B5563"
                        font.pixelSize: 12
                    }
                    
                    Comp.CheckBox {
                        text: "增强模式 (强制高通滤波)"
                        checked: backend ? backend.echoEnhancedFilter : true
                        onCheckedChanged: {
                            if (backend) backend.echoEnhancedFilter = checked
                        }
                    }
                }
                
                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#E5E7EB"
                }
                
                // -- Noise Suppression Level --
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: backend ? backend.noiseSuppression : false
                    
                    Text {
                        text: "噪声抑制强度"
                        color: "#4B5563"
                        font.pixelSize: 12
                    }
                    
                    RowLayout {
                        spacing: 8
                        
                        Repeater {
                            model: ["低", "中", "高", "极高"]
                            
                            Rectangle {
                                width: 56
                                height: 28
                                radius: 6
                                color: (backend && backend.nsLevel === index) ? "#3B82F6" : "#F3F4F6"
                                border.color: (backend && backend.nsLevel === index) ? "#3B82F6" : "#D1D5DB"
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: (backend && backend.nsLevel === index) ? "#FFFFFF" : "#374151"
                                    font.pixelSize: 12
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (backend) backend.nsLevel = index
                                    }
                                }
                            }
                        }
                    }
                    
                    Text {
                        text: "强度越高，噪音压得越狠，但语音可能更失真"
                        color: "#9CA3AF"
                        font.pixelSize: 10
                    }
                }
                
                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#E5E7EB"
                }
                
                // -- AGC Mode & Parameters --
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: backend ? backend.autoGainControl : false
                    
                    Text {
                        text: "增益控制模式"
                        color: "#4B5563"
                        font.pixelSize: 12
                    }
                    
                    RowLayout {
                        spacing: 8
                        
                        Repeater {
                            model: ["自适应", "固定增益"]
                            
                            Rectangle {
                                width: 80
                                height: 28
                                radius: 6
                                color: (backend && backend.agcMode === index) ? "#3B82F6" : "#F3F4F6"
                                border.color: (backend && backend.agcMode === index) ? "#3B82F6" : "#D1D5DB"
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: (backend && backend.agcMode === index) ? "#FFFFFF" : "#374151"
                                    font.pixelSize: 12
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (backend) backend.agcMode = index
                                    }
                                }
                            }
                        }
                    }
                    
                    // Adaptive mode: max gain slider
                    ColumnLayout {
                        visible: backend ? backend.agcMode === 0 : true
                        spacing: 4
                        Layout.fillWidth: true
                        
                        Text {
                            text: "最大自适应增益: " + (backend ? backend.adaptiveDigitalMaxGainDb.toFixed(0) : "50") + " dB"
                            color: "#6B7280"
                            font.pixelSize: 11
                        }
                        
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 50
                            stepSize: 1
                            value: backend ? backend.adaptiveDigitalMaxGainDb : 50
                            onMoved: {
                                if (backend) backend.adaptiveDigitalMaxGainDb = value
                            }
                        }
                        
                        Text {
                            text: "值越大，安静环境下放大越多（可能放大环境噪音）"
                            color: "#9CA3AF"
                            font.pixelSize: 10
                        }
                    }
                    
                    // Fixed mode: gain slider
                    ColumnLayout {
                        visible: backend ? backend.agcMode === 1 : false
                        spacing: 4
                        Layout.fillWidth: true
                        
                        Text {
                            text: "固定数字增益: " + (backend ? backend.fixedDigitalGainDb.toFixed(0) : "0") + " dB"
                            color: "#6B7280"
                            font.pixelSize: 11
                        }
                        
                        Slider {
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
        }
        
        Item { Layout.fillHeight: true }
    }
}
