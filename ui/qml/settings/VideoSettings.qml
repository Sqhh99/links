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
        
        // Camera
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Text {
                text: "摄像头"
                color: "#374151"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
            
            Comp.ComboBox {
                id: cameraCombo
                Layout.fillWidth: true
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
        
        // Resolution
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Text {
                text: "分辨率"
                color: "#374151"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
            
            Comp.ComboBox {
                id: resolutionCombo
                Layout.fillWidth: true
                model: backend ? backend.resolutions : []
                
                currentIndex: backend ? backend.selectedResolutionIndex : 0
                
                onActivated: {
                    if (backend) {
                        backend.selectedResolutionIndex = currentIndex
                    }
                }
            }
        }
        
        // Hardware acceleration
        Comp.CheckBox {
            id: hardwareCheck
            text: "启用硬件加速"
            checked: backend ? backend.hardwareAccel : true
            onCheckedChanged: {
                if (backend) backend.hardwareAccel = checked
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
