import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import QtMultimedia
import Links
import Links.Backend 1.0

// A participant's video, or an avatar while no frame has arrived.
// The whole tile, video included, is masked to `radius`.
Rectangle {
    id: root

    radius: 8
    color: Theme.hoverBackground
    clip: true

    property string participantId: ""
    property string participantName: ""
    property bool micEnabled: false
    property bool camEnabled: false
    property bool mirrored: false
    property bool showStatus: true
    // Built-in name bar; hosts that draw their own label turn it off
    property bool showLabel: true

    signal clicked()

    layer.enabled: radius > 0
    layer.effect: MultiEffect {
        maskEnabled: true
        maskSource: cornerMask
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0
    }

    Rectangle {
        id: cornerMask
        anchors.fill: parent
        radius: root.radius
        visible: false
        layer.enabled: true
        layer.smooth: true
    }

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

    function displayName() {
        // Drop suffixes such as " (You)" so they don't end up in the initials
        return (root.participantName || "").replace(/\s*\(.*\)\s*$/, "")
    }

    function getInitials() {
        var name = root.displayName()
        if (!name) return "?"
        var parts = name.split(' ')
        var initials = ""
        for (var i = 0; i < Math.min(2, parts.length); i++) {
            if (parts[i].length > 0) {
                initials += parts[i][0].toUpperCase()
            }
        }
        return initials || "?"
    }

    function avatarColor() {
        var palette = ["#1F6FFF", "#16B26B", "#7C5CFF", "#F59E0B", "#EC4899", "#0EA5E9", "#F04A4A", "#6366F1"]
        var key = root.displayName() || root.participantId
        var hash = 0
        for (var i = 0; i < key.length; i++) {
            hash = (hash * 31 + key.charCodeAt(i)) % 2147483647
        }
        return palette[hash % palette.length]
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

    // Avatar when no video
    Rectangle {
        id: avatar
        anchors.centerIn: parent
        visible: !renderer.hasFrame
        width: Math.round(Math.max(32, Math.min(96, Math.min(root.width, root.height) * 0.3)))
        height: width
        radius: width / 2
        color: root.avatarColor()

        Text {
            anchors.centerIn: parent
            text: root.getInitials()
            color: Theme.textOnAccent
            font.pixelSize: Math.round(avatar.width * 0.4)
            font.weight: Font.DemiBold
        }
    }

    // Overlay container
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 36
        visible: root.showLabel
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.4) : Qt.rgba(0, 0, 0, 0.3) }
        }
    }

    // Name and status
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        spacing: 6
        visible: root.showLabel

        Text {
            text: root.participantName
            color: "#ffffff"
            font.pixelSize: 12
            font.weight: Font.Medium
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        // Mic status only
        Icon {
            visible: root.showStatus
            name: root.micEnabled ? "mic" : "mic-off"
            size: 14
            color: root.micEnabled ? "#FFFFFF" : Theme.danger
        }
    }
}
