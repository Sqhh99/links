import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Links.Backend 1.0

Rectangle {
    id: root
    
    color: "#F9FAFB" // Light theme - no black borders
    radius: 12
    clip: true
    
    property ConferenceBackend backend
    
    // Video renderer
    VideoRenderer {
        id: renderer
        participantId: backend ? backend.mainParticipantId : ""
        participantName: getDisplayName()
        videoSink: videoOutput.videoSink
        
        function getDisplayName() {
            if (!backend || !backend.mainParticipantId) return ""
            var info = backend.getParticipantInfo(backend.mainParticipantId)
            return info.name || backend.mainParticipantId
        }
    }
    
    function updateFrame(frame) {
        renderer.updateFrame(frame)
    }
    
    function clearFrame() {
        renderer.clearFrame()
    }
    
    // Video output - fills entire panel
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: renderer.hasFrame
    }
    
    // Placeholder when no video
    Rectangle {
        anchors.fill: parent
        visible: !renderer.hasFrame
        color: "#F9FAFB" // Light theme background
        radius: 12
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 16
            
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 80
                height: 80
                radius: 40
                color: "#E5E7EB" // Light gray circle
                
                Text {
                    anchors.centerIn: parent
                    text: getInitials()
                    color: "#6B7280" // Dark gray text
                    font.pixelSize: 32
                    font.weight: Font.Bold
                    
                    function getInitials() {
                        if (!backend || !backend.mainParticipantId) return "?"
                        var info = backend.getParticipantInfo(backend.mainParticipantId)
                        var name = info.name || backend.mainParticipantId
                        var parts = name.split(' ')
                        var initials = ""
                        for (var i = 0; i < Math.min(2, parts.length); i++) {
                            if (parts[i].length > 0) {
                                initials += parts[i][0].toUpperCase()
                            }
                        }
                        return initials || "?"
                    }
                }
            }
            
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: {
                    if (!backend || !backend.mainParticipantId) {
                        return "Waiting for participants..."
                    }
                    var info = backend.getParticipantInfo(backend.mainParticipantId)
                    return info.name || backend.mainParticipantId
                }
                color: "#111827" // Dark text
                font.pixelSize: 18
                font.weight: Font.Medium
            }
        }
    }
    
    // Name overlay at bottom
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        height: 48
        visible: renderer.hasFrame && backend && backend.mainParticipantId
        color: "transparent"
        
        Rectangle {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            height: 32
            width: nameText.width + 16
            color: "#80000000"
            radius: 6
            
            Text {
                id: nameText
                anchors.centerIn: parent
                text: {
                    if (!backend || !backend.mainParticipantId) return ""
                    var info = backend.getParticipantInfo(backend.mainParticipantId)
                    return info.name || backend.mainParticipantId
                }
                color: "#ffffff"
                font.pixelSize: 14
                font.weight: Font.Medium
            }
        }
    }
}
