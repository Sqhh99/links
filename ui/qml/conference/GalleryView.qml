import QtQuick
import QtQuick.Controls
import Links
import Links.Backend 1.0

// Gallery layout: every participant as an equal 16:9 tile. The column count is
// chosen so tiles are as large as possible while the whole grid fits; the grid
// is centered and scrolls only once tiles would drop below minTileWidth.
Item {
    id: root

    property ConferenceBackend backend

    readonly property var remoteParticipants: backend
        ? backend.participants.filter(function(p) { return p.identity !== "local" })
        : []
    readonly property int tileCount: remoteParticipants.length + 1
    readonly property int gap: 8
    readonly property int minTileWidth: 220
    readonly property var grid: computeGrid(flick.width, flick.height, tileCount)

    function computeGrid(areaWidth, areaHeight, count) {
        var columns = 1
        var tileWidth = 0
        for (var c = 1; c <= count; c++) {
            var r = Math.ceil(count / c)
            var w = Math.min((areaWidth - (c - 1) * gap) / c,
                             (areaHeight - (r - 1) * gap) / r * 16 / 9)
            if (w > tileWidth) {
                columns = c
                tileWidth = w
            }
        }
        if (tileWidth < minTileWidth) {
            // Too many tiles to fit: keep them readable and scroll instead
            columns = Math.min(count, Math.max(1, Math.floor((areaWidth + gap) / (minTileWidth + gap))))
            tileWidth = (areaWidth - (columns - 1) * gap) / columns
        }
        tileWidth = Math.max(0, Math.floor(tileWidth))
        var tileHeight = Math.floor(tileWidth * 9 / 16)
        var rows = Math.ceil(count / columns)
        return {
            columns: columns,
            tileWidth: tileWidth,
            tileHeight: tileHeight,
            contentHeight: rows * tileHeight + (rows - 1) * gap
        }
    }

    // The last row is centered on its own
    function tileX(index) {
        var row = Math.floor(index / grid.columns)
        var inRow = Math.min(grid.columns, tileCount - row * grid.columns)
        var rowWidth = inRow * grid.tileWidth + (inRow - 1) * gap
        return Math.round((flick.width - rowWidth) / 2 + (index % grid.columns) * (grid.tileWidth + gap))
    }

    function tileY(index) {
        var top = Math.max(0, (flick.height - grid.contentHeight) / 2)
        return Math.round(top + Math.floor(index / grid.columns) * (grid.tileHeight + gap))
    }

    // Frame routing. With both camera and screen active, a tile shows the
    // stream its chevrons selected; otherwise it shows whichever one exists.
    function updateLocalCameraFrame(frame) {
        if ((!localTile.hasDualStreams || !localTile.showingScreen) && backend.camEnabled) {
            localTile.updateFrame(frame)
        }
    }

    function updateLocalScreenFrame(frame) {
        if ((!localTile.hasDualStreams || localTile.showingScreen) && backend.screenSharing) {
            localTile.updateFrame(frame)
        }
    }

    function clearLocalFrame() {
        localTile.clearFrame()
    }

    function updateRemoteFrame(participantId, frame, isScreenFrame) {
        for (var i = 0; i < remoteRepeater.count; i++) {
            var tile = remoteRepeater.itemAt(i)
            if (tile && tile.participantId === participantId) {
                if (!tile.hasDualStreams || isScreenFrame === tile.showingScreen) {
                    tile.updateFrame(frame)
                }
                return
            }
        }
    }

    function clearRemoteFrame(participantId) {
        for (var i = 0; i < remoteRepeater.count; i++) {
            var tile = remoteRepeater.itemAt(i)
            if (tile && tile.participantId === participantId) {
                tile.clearFrame()
                return
            }
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, root.grid.contentHeight)
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {
            policy: flick.interactive ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }

        GalleryTile {
            id: localTile
            x: root.tileX(0)
            y: root.tileY(0)
            width: root.grid.tileWidth
            height: root.grid.tileHeight
            participantId: "local"
            participantName: root.backend ? root.backend.userName : ""
            label: localTile.displayingScreen ? "我 (屏幕)" : "我 (You)"
            micEnabled: root.backend ? root.backend.micEnabled : false
            camEnabled: root.backend ? root.backend.camEnabled : false
            screenSharing: root.backend ? root.backend.screenSharing : false
            mirrorCamera: true
            highlighted: root.tileCount > 1 && !!root.backend && root.backend.mainParticipantId === "local"
            onClicked: root.backend.pinParticipant("local")
        }

        Repeater {
            id: remoteRepeater
            model: root.remoteParticipants

            GalleryTile {
                id: remoteTile
                required property var modelData
                required property int index

                x: root.tileX(index + 1)
                y: root.tileY(index + 1)
                width: root.grid.tileWidth
                height: root.grid.tileHeight
                participantId: modelData.identity
                participantName: modelData.name || modelData.identity
                label: remoteTile.displayingScreen ? remoteTile.participantName + " (屏幕)" : remoteTile.participantName
                micEnabled: modelData.micEnabled
                camEnabled: modelData.camEnabled
                screenSharing: modelData.screenSharing
                highlighted: root.tileCount > 1 && !!root.backend && root.backend.mainParticipantId === modelData.identity
                onClicked: root.backend.pinParticipant(modelData.identity)
            }
        }
    }
}
