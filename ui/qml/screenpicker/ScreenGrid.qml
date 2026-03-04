import QtQuick
import QtQuick.Controls
import Links

Rectangle {
    id: root
    
    property var items: []
    property int selectedIndex: -1
    
    color: Theme.cardBackground
    radius: 12
    border.color: Theme.borderColor
    border.width: 1
    clip: true
    
    GridView {
        id: gridView
        anchors.fill: parent
        anchors.margins: 10
        
        cellWidth: 260  // Item width + spacing
        cellHeight: 200  // Item height + spacing
        
        model: root.items
        
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            
            background: Rectangle {
                color: "transparent"
                implicitWidth: 12
            }
            
            contentItem: Rectangle {
                implicitWidth: 8
                radius: 4
                color: parent.pressed ? Theme.textTertiary : (parent.hovered ? Theme.textTertiary : Theme.textHint)
            }
        }
        
        delegate: ThumbnailItem {
            required property var modelData
            required property int index
            
            thumbnail: modelData.thumbnail
            title: modelData.title || ""
            tooltipText: modelData.tooltip || ""
            selected: root.selectedIndex === index
            
            onClicked: root.selectedIndex = index
        }
    }
}
