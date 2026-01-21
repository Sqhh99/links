import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    
    property string currentTime: ""
    property string currentDate: ""
    
    color: "transparent"
    
    Rectangle {
        anchors.fill: parent
        radius: 16
        border.color: "#1E40AF"
        border.width: 1

        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#1D4ED8" }
            GradientStop { position: 1.0; color: "#0F172A" }
        }
    }

    Rectangle {
        width: 180
        height: 180
        radius: 90
        color: "#1A93C5FD"
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: -40
        anchors.rightMargin: -60
    }

    Rectangle {
        width: 140
        height: 140
        radius: 70
        color: "#1A22D3EE"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.bottomMargin: -30
        anchors.leftMargin: -40
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
                    text: "LINKS"
                    color: "#2563EB"
                    font.pixelSize: 11
                    font.weight: Font.ExtraBold
                    font.letterSpacing: 1
                }
            }
            
            Text {
                text: "Links"
                color: "white"
                font.pixelSize: 20
                font.weight: Font.ExtraBold
            }
            
            Item { Layout.fillWidth: true }
        }
        
        Text {
            text: "现代化音视频会议体验"
            color: "#E0E7FF"
            font.pixelSize: 14
            font.weight: Font.Medium
        }

        Rectangle {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 4
            radius: 2
            color: "#60A5FA"
        }

        Item { Layout.fillHeight: true }

        ColumnLayout {
            spacing: 6

            Text {
                text: root.currentTime
                color: "white"
                font.pixelSize: 36
                font.weight: Font.Light
                font.letterSpacing: -1
            }

            Text {
                text: root.currentDate
                color: "#BFDBFE"
                font.pixelSize: 12
            }
        }
    }
}
