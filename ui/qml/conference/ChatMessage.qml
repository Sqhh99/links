import QtQuick
import QtQuick.Layouts
// import QtQuick.Effects

Rectangle {
    id: root
    
    implicitHeight: contentLayout.implicitHeight + 24
    radius: 12
    
    // Bubble style: Blue for Local, White for Remote
    color: isLocal ? "#2563EB" : "#FFFFFF"
    
    // Remote bubbles get a border/shadow
    border.color: isLocal ? "transparent" : "#E5E7EB"
    border.width: isLocal ? 0 : 1
    
    property string senderName: ""
    property string message: ""
    property var timestamp: 0
    property bool isLocal: false
    
    // layer.enabled: !isLocal
    /*
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "#10000000"
        shadowBlur: 8
        shadowVerticalOffset: 2
    }
    */
    
    ColumnLayout {
        id: contentLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 4
        
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Text {
                text: root.senderName
                color: root.isLocal ? "#BFDBFE" : "#6B7280" // Blue-200 : Gray-500
                font.pixelSize: 10
                font.weight: Font.Medium
            }
            
            Item { Layout.fillWidth: true }
            
            Text {
                text: formatTime(root.timestamp)
                color: root.isLocal ? "#BFDBFE" : "#9CA3AF"
                font.pixelSize: 10
                
                function formatTime(ts) {
                    if (!ts) return ""
                    var date = new Date(ts)
                    var hours = date.getHours().toString().padStart(2, '0')
                    var mins = date.getMinutes().toString().padStart(2, '0')
                    return hours + ":" + mins
                }
            }
        }
        
        Text {
            Layout.fillWidth: true
            text: root.message
            color: root.isLocal ? "#FFFFFF" : "#1F2937" // White : Gray-800
            font.pixelSize: 13
            wrapMode: Text.Wrap
        }
    }
}
