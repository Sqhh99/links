import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

RowLayout {
    id: root
    
    property int currentIndex: 0
    property bool windowShareSupported: true
    
    height: 44
    spacing: 0
    
    TabItem {
        text: "整个屏幕"
        active: root.currentIndex === 0
        onClicked: root.currentIndex = 0
    }
    
    TabItem {
        text: "窗口"
        visible: root.windowShareSupported
        enabled: root.windowShareSupported
        active: root.currentIndex === 1
        onClicked: root.currentIndex = 1
    }
    
    Item { Layout.fillWidth: true }
    
    component TabItem: Button {
        id: tabButton
        
        property bool active: false
        
        implicitHeight: 44
        leftPadding: 14
        rightPadding: 14
        
        background: Rectangle {
            color: "transparent"
            
            Rectangle {
                width: parent.width
                height: 2
                anchors.bottom: parent.bottom
                color: tabButton.active ? Theme.accentColor : "transparent"
            }
        }
        
        contentItem: Text {
            text: tabButton.text
            color: tabButton.active ? Theme.accentColor : Theme.textSecondary
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: function(mouse) { mouse.accepted = false }
        }
    }
}
