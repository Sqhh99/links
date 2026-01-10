import QtQuick
import QtQuick.Controls
import QtMultimedia
import Links.Backend 1.0

Rectangle {
    id: root
    
    property string title: ""
    property var thumbnail: null
    property string tooltipText: ""
    property bool selected: false
    
    signal clicked()
    
    width: 250
    height: 190
    radius: 12
    
    // Fixed width to prevent layout jumping/flickering during state changes
    border.width: 2
    
    // Background color: Only show color when selected. No background change on hover.
    color: selected ? "#EFF6FF" : "transparent"
    
    // Border color: Use specific transparent color (#00D1D5DB) for default state
    // This ensures transition is pure opacity change (Alpha 0 -> Alpha 1) rather than Color change
    border.color: selected ? "#2563EB" : (mouseArea.containsMouse ? "#D1D5DB" : "#00D1D5DB")
    
    Behavior on border.color {
        ColorAnimation { duration: 150 }
    }
    
    Column {
        anchors.centerIn: parent
        spacing: 8
        
        // Thumbnail container
        Rectangle {
            id: thumbnailContainer
            width: 230
            height: 130
            radius: 10
            color: "#F3F4F6"
            clip: true
            
            // Use VideoRenderer with VideoOutput to display QImage
            VideoRenderer {
                id: imageRenderer
                
                function updateThumbnail() {
                    // Only update if thumbnail is a valid QImage (not null/undefined/invalid)
                    if (root.thumbnail && typeof root.thumbnail !== 'undefined') {
                        try {
                            imageRenderer.updateFrame(root.thumbnail)
                        } catch (e) {
                            // Ignore errors for invalid thumbnails
                        }
                    }
                }
            }
            
            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
                
                Component.onCompleted: {
                    imageRenderer.videoSink = videoOutput.videoSink
                }
            }
            
            // Initial update when component is ready
            Component.onCompleted: {
                Qt.callLater(imageRenderer.updateThumbnail)
            }
        }
        
        // Title text
        Text {
            width: 230
            text: root.title
            color: "#374151"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
            maximumLineCount: 2
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }
    }
    
    // Update when thumbnail property changes on root
    onThumbnailChanged: {
        imageRenderer.updateThumbnail()
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        
        onClicked: root.clicked()
    }
    
    ToolTip.visible: tooltipText.length > 0 && mouseArea.containsMouse
    ToolTip.text: tooltipText
    ToolTip.delay: 500
}
