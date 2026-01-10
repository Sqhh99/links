import QtQuick
import QtQuick.Controls

ComboBox {
    id: control
    
    implicitHeight: 40
    
    delegate: ItemDelegate {
        id: delegate
        width: control.width
        contentItem: Text {
            text: {
                if (control.textRole) {
                    if (typeof modelData !== "undefined") {
                        return modelData[control.textRole]
                    }
                    return model[control.textRole]
                }
                return modelData
            }
            color: delegate.highlighted ? "#FFFFFF" : "#111827"
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: delegate.highlighted ? "#2563EB" : "transparent"
            radius: 4
            anchors.fill: parent
            anchors.margins: 2
        }
        highlighted: control.highlightedIndex === index
    }

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = control.pressed ? "#2563EB" : "#6B7280";
            context.fill();
        }

        Connections {
            target: control
            function onPressedChanged() { canvas.requestPaint(); }
        }
    }

    contentItem: Text {
        leftPadding: 12
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        font: control.font
        color: "#111827"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 40
        border.color: control.pressed || control.hovered ? "#2563EB" : "#D1D5DB"
        border.width: control.visualFocus ? 2 : 1
        radius: 8
        color: "#FFFFFF"
    }

    popup: Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: contentItem.implicitHeight + 4
        padding: 2

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            border.color: "#E5E7EB"
            border.width: 1
            radius: 8
            color: "#FFFFFF"
            
            // Subtle shadow
            layer.enabled: true
            layer.effect: null // Placeholder for Shadow if needed
        }
    }
}
