import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property string selectedTime: ""
    property bool enabled: true

    signal timeSelected(string value)

    function pad2(value) {
        return value < 10 ? "0" + value : "" + value
    }

    function parseHour() {
        if (selectedTime.indexOf(":") < 0) return 0
        var v = parseInt(selectedTime.split(":")[0], 10)
        return isNaN(v) ? 0 : v
    }

    function parseMinute() {
        if (selectedTime.indexOf(":") < 0) return 0
        var v = parseInt(selectedTime.split(":")[1], 10)
        return isNaN(v) ? 0 : v
    }

    function emitTime() {
        var h = hourTumbler.currentIndex
        var m = minuteTumbler.currentIndex
        var value = pad2(h) + ":" + pad2(m)
        if (root.selectedTime !== value) {
            root.selectedTime = value
            root.timeSelected(value)
        }
    }

    Component.onCompleted: {
        hourTumbler.currentIndex = parseHour()
        minuteTumbler.currentIndex = parseMinute()
    }

    onSelectedTimeChanged: {
        var h = parseHour()
        var m = parseMinute()
        if (hourTumbler.currentIndex !== h)
            hourTumbler.currentIndex = h
        if (minuteTumbler.currentIndex !== m)
            minuteTumbler.currentIndex = m
    }

    spacing: 4

    Text {
        text: "预定时间"
        color: "#374151"
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 100
        radius: 10
        color: "#F9FAFB"
        border.color: "#E5E7EB"
        border.width: 1

        // Intercept wheel events so they don't scroll the parent Flickable
        MouseArea {
            anchors.fill: parent
            onWheel: function(wheel) {
                var target = (wheel.x < parent.width / 2) ? hourTumbler : minuteTumbler
                var delta = (wheel.angleDelta.y > 0) ? -1 : 1
                var next = target.currentIndex + delta
                if (next < 0) next = target.model - 1
                else if (next >= target.model) next = 0
                target.currentIndex = next
                wheel.accepted = true
            }
        }

        RowLayout {
            anchors.centerIn: parent
            spacing: 0

            Tumbler {
                id: hourTumbler
                model: 24
                visibleItemCount: 3
                enabled: root.enabled
                implicitWidth: 64
                implicitHeight: 90
                wrap: true

                onCurrentIndexChanged: root.emitTime()

                delegate: Text {
                    text: root.pad2(modelData)
                    color: Tumbler.displacement === 0 ? "#111827" : "#9CA3AF"
                    font.pixelSize: Tumbler.displacement === 0 ? 22 : 14
                    font.weight: Tumbler.displacement === 0 ? Font.Bold : Font.Normal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 1.0 - Math.abs(Tumbler.displacement) * 0.4
                }
            }

            Text {
                text: ":"
                color: "#111827"
                font.pixelSize: 24
                font.weight: Font.Bold
                Layout.alignment: Qt.AlignVCenter
            }

            Tumbler {
                id: minuteTumbler
                model: 60
                visibleItemCount: 3
                enabled: root.enabled
                implicitWidth: 64
                implicitHeight: 90
                wrap: true

                onCurrentIndexChanged: root.emitTime()

                delegate: Text {
                    text: root.pad2(modelData)
                    color: Tumbler.displacement === 0 ? "#111827" : "#9CA3AF"
                    font.pixelSize: Tumbler.displacement === 0 ? 22 : 14
                    font.weight: Tumbler.displacement === 0 ? Font.Bold : Font.Normal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 1.0 - Math.abs(Tumbler.displacement) * 0.4
                }
            }
        }
    }
}
