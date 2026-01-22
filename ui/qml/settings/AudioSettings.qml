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
        
        // Checkboxes
        ColumnLayout {
            spacing: 12
            
            Comp.CheckBox {
                id: echoCheck
                text: "回声消除"
                checked: backend ? backend.echoCancel : true
                onCheckedChanged: {
                    if (backend) backend.echoCancel = checked
                }
            }
            
            Comp.CheckBox {
                id: noiseCheck
                text: "噪声抑制"
                checked: backend ? backend.noiseSuppression : true
                onCheckedChanged: {
                    if (backend) backend.noiseSuppression = checked
                }
            }
            
            Comp.CheckBox {
                id: agcCheck
                text: "自动增益控制"
                checked: backend ? backend.autoGainControl : true
                onCheckedChanged: {
                    if (backend) backend.autoGainControl = checked
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
