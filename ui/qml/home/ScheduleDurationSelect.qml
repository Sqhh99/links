import QtQuick
import QtQuick.Layouts
import Links

ColumnLayout {
    id: root

    property string title: ""
    property string hintText: ""
    property int selectedMinutes: 15
    property bool enabled: true
    property var optionsModel: [5, 10, 15, 20, 30, 45, 60, 90, 120, 180]

    signal minutesSelected(int value)

    property var displayOptions: []

    function buildOptions() {
        var list = []
        for (var i = 0; i < optionsModel.length; ++i) {
            var v = Number(optionsModel[i])
            list.push({ "label": v + " 分钟", "value": v })
        }
        displayOptions = list
        syncIndex()
    }

    function syncIndex() {
        var idx = 0
        for (var i = 0; i < displayOptions.length; ++i) {
            if (displayOptions[i].value === selectedMinutes) {
                idx = i
                break
            }
        }

        if (durationCombo.currentIndex !== idx) {
            durationCombo.currentIndex = idx
        }
    }

    onSelectedMinutesChanged: syncIndex()
    onOptionsModelChanged: buildOptions()

    Component.onCompleted: buildOptions()

    spacing: 4

    Text {
        text: root.title
        color: "#374151"
        font.pixelSize: 12
        font.weight: Font.DemiBold
        visible: root.title.length > 0
    }

    ComboBox {
        id: durationCombo
        Layout.fillWidth: true
        enabled: root.enabled
        textRole: "label"
        model: root.displayOptions
        onActivated: {
            var data = root.displayOptions[currentIndex]
            if (!data)
                return
            var value = Number(data.value)
            if (root.selectedMinutes !== value) {
                root.selectedMinutes = value
                root.minutesSelected(value)
            }
        }
    }

    Text {
        text: root.hintText
        color: "#9CA3AF"
        font.pixelSize: 11
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        visible: root.hintText.length > 0
    }
}
