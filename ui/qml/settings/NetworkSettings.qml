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
    
    ColumnLayout {
        width: parent.width
        spacing: 24
        
        // API URL
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Text {
                text: "信令地址"
                color: "#374151"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
            
            TextField {
                id: apiUrlInput
                Layout.fillWidth: true
                placeholderText: "信令服务器地址，例如 wss://example.com"
                text: backend ? backend.apiUrl : ""
                
                onTextChanged: {
                    if (backend) {
                        backend.apiUrl = text
                    }
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
