import QtQuick
import QtQuick.Controls
import Links
import Links.Backend 1.0

// One participant in the gallery grid: a borderless rounded video tile with a
// name pill, camera/screen switch chevrons and a single highlight ring.
Item {
    id: root

    property string participantId: ""
    // Bare name, used for the avatar initials
    property string participantName: ""
    // Text shown in the name pill
    property string label: participantName
    property bool micEnabled: false
    property bool camEnabled: false
    property bool screenSharing: false
    // Mirror camera frames (local preview); screen frames are never mirrored
    property bool mirrorCamera: false
    property bool highlighted: false

    // With both camera and screen, the chevrons pick which one the tile shows
    property bool showingScreen: false
    readonly property bool hasDualStreams: camEnabled && screenSharing
    readonly property bool displayingScreen: hasDualStreams ? showingScreen : (screenSharing && !camEnabled)

    signal clicked()

    function updateFrame(frame) {
        thumbnail.updateFrame(frame)
    }

    function clearFrame() {
        thumbnail.clearFrame()
    }

    VideoThumbnail {
        id: thumbnail
        anchors.fill: parent
        radius: Theme.radiusMd
        participantId: root.participantId
        participantName: root.participantName
        micEnabled: root.micEnabled
        camEnabled: root.showingScreen ? true : (root.camEnabled || root.screenSharing)
        mirrored: root.mirrorCamera && !root.displayingScreen
        showStatus: false
        showLabel: false
    }

    // Click to pin
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
        z: 1
    }

    // Left chevron (switch to camera)
    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 8
        width: 28
        height: 28
        radius: 14
        color: leftArea.containsMouse ? Qt.rgba(0, 0, 0, 0.6) : Qt.rgba(0, 0, 0, 0.4)
        visible: root.hasDualStreams && root.showingScreen
        z: 20

        Icon {
            anchors.centerIn: parent
            name: "chevron-left"
            size: 14
            color: "#FFFFFF"
        }

        MouseArea {
            id: leftArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.showingScreen = false
        }
    }

    // Right chevron (switch to screen)
    Rectangle {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 8
        width: 28
        height: 28
        radius: 14
        color: rightArea.containsMouse ? Qt.rgba(0, 0, 0, 0.6) : Qt.rgba(0, 0, 0, 0.4)
        visible: root.hasDualStreams && !root.showingScreen
        z: 20

        Icon {
            anchors.centerIn: parent
            name: "chevron-right"
            size: 14
            color: "#FFFFFF"
        }

        MouseArea {
            id: rightArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.showingScreen = true
        }
    }

    // Name pill with mic status (bottom-left)
    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        height: 24
        width: Math.min(nameRow.implicitWidth + 16, root.width - 16)
        radius: 6
        color: Theme.stageOverlay
        z: 10

        Row {
            id: nameRow
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 8
            spacing: 5

            // Green level bar while unmuted, mic-off icon while muted
            Item {
                width: 11
                height: 11
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.centerIn: parent
                    width: 4
                    height: 8
                    radius: 2
                    color: Theme.success
                    visible: root.micEnabled
                }

                Icon {
                    anchors.centerIn: parent
                    name: "mic-off"
                    size: 11
                    color: Theme.danger
                    visible: !root.micEnabled
                }
            }

            Text {
                width: Math.min(implicitWidth, root.width - 16 - 16 - 11 - nameRow.spacing)
                text: root.label
                color: Theme.stageText
                font.pixelSize: 11
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
        }
    }

    // Highlight ring for the participant shown in the speaker view
    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusMd
        color: "transparent"
        border.color: Theme.accentColor
        border.width: 2
        visible: root.highlighted
        z: 15
    }
}
