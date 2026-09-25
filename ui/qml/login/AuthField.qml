import QtQuick
import Links
import QtQuick.Layouts

ColumnLayout {
    id: root

    property string label: ""
    property string placeholderText: ""
    property string errorText: ""
    property bool showError: false
    property alias text: input.text
    property alias echoMode: input.echoMode
    property alias inputMethodHints: input.inputMethodHints
    property alias validator: input.validator
    property alias maximumLength: input.maximumLength
    property alias inputMask: input.inputMask
    property alias readOnly: input.readOnly
    property alias inputField: input

    signal edited()
    signal submitted()

    spacing: 6

    Text {
        visible: root.label.length > 0
        text: root.label
        color: Theme.textSecondary
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }

    AuthTextField {
        id: input
        Layout.fillWidth: true
        placeholderText: root.placeholderText
        error: root.showError && root.errorText.length > 0

        onTextChanged: root.edited()
        Keys.onReturnPressed: root.submitted()
    }

    Text {
        visible: root.showError && root.errorText.length > 0
        text: root.errorText
        color: Theme.danger
        font.pixelSize: 12
    }
}
