import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Links.Backend 1.0

Rectangle {
    id: root
    
    radius: 8
    color: "#F3F4F6" // Light theme - no black borders
    clip: true
    
    property string participantId: ""
    property string participantName: ""
    property bool micEnabled: false
    property bool camEnabled: false
    property bool mirrored: false
    property bool showStatus: true
    
    signal clicked()
    
    // Video renderer (C++ backend)
    VideoRenderer {
        id: renderer
        participantId: root.participantId
        participantName: root.participantName
        micEnabled: root.micEnabled
        camEnabled: root.camEnabled
        mirrored: root.mirrored
        videoSink: videoOutput.videoSink
    }
    
    function updateFrame(frame) {
        renderer.updateFrame(frame)
    }
    
    function clearFrame() {
        renderer.clearFrame()
    }
    
    function getInitials() {
        if (!root.participantName) return "?"
        var parts = root.participantName.split(' ')
        var initials = ""
        for (var i = 0; i < Math.min(2, parts.length); i++) {
            if (parts[i].length > 0) {
                initials += parts[i][0].toUpperCase()
            }
        }
        return initials || "?"
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    
    // Video output
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: renderer.hasFrame
    }
    
    // Placeholder when no video
    Rectangle {
        anchors.fill: parent
        visible: !renderer.hasFrame
        color: "#F3F4F6" // Light theme placeholder background
        
        Text {
            anchors.centerIn: parent
            text: root.getInitials()
            color: "#6B7280" // Light theme text
            font.pixelSize: 28
            font.weight: Font.Bold
        }
    }
    
    // Overlay container
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 36
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: "#80000000" }
        }
    }
    
    // Name and status
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        spacing: 6
        
        Text {
            text: root.participantName
            color: "#ffffff"
            font.pixelSize: 12
            font.weight: Font.Medium
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        
        // Mic status only
        Image {
            visible: root.showStatus
            source: root.micEnabled ? "qrc:/res/icon/Turn_on_the_microphone.png" : "qrc:/res/icon/mute_the_microphone.png"
            sourceSize.width: 14
            sourceSize.height: 14
            opacity: root.micEnabled ? 1.0 : 0.6
        }
    }
}
