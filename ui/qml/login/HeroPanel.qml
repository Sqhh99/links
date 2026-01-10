import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    
    property string currentTime: ""
    property string currentDate: ""
    
    color: "transparent"
    
    Rectangle {
        anchors.fill: parent
        radius: 14
        border.color: "#1E40AF"
        border.width: 1
        
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#2563EB" }
            GradientStop { position: 1.0; color: "#1E40AF" }
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16
        
        // Brand
        RowLayout {
            spacing: 10
            
            Rectangle {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 32
                radius: 10
                color: "white"
                
                Text {
                    anchors.centerIn: parent
                    text: "LIVE"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.weight: Font.ExtraBold
                    font.letterSpacing: 1
                }
            }
            
            Text {
                text: "CloudMeet"
                color: "white"
                font.pixelSize: 18
                font.weight: Font.ExtraBold
            }
            
            Item { Layout.fillWidth: true }
        }
        
        // Time
        Text {
            text: root.currentTime
            color: "white"
            font.pixelSize: 44
            font.weight: Font.Light
            font.letterSpacing: -1
        }
        
        // Date
        Text {
            text: root.currentDate
            color: "#E0E7FF" // Blue-100
            font.pixelSize: 14
        }
        
        // Highlight card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            radius: 12
            color: "#1AFFFFFF"  // 10% white
            border.color: "#33FFFFFF"  // 20% white
            border.width: 1
            
            Text {
                anchors.fill: parent
                anchors.margins: 14
                text: "下一场会议：产品评审\n10:00 - 11:00"
                color: "white"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }
        }
        
        // Bullet points
        ColumnLayout {
            spacing: 8
            
            Repeater {
                model: [
                    "低延迟高清音视频",
                    "一键屏幕共享与录制",
                    "端到端加密与入会鉴权"
                ]
                
                RowLayout {
                    spacing: 8
                    
                    Rectangle {
                        width: 10
                        height: 10
                        radius: 6
                        color: "#34D399" // Emerald-400
                    }
                    
                    Text {
                        text: modelData
                        color: "#EFF6FF" // Blue-50
                        font.pixelSize: 13
                    }
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
