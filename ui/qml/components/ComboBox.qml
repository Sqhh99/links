import QtQuick
import Links
import QtQuick.Controls
import Links.Backend 1.0

ComboBox {
    id: control
    
    implicitHeight: 40
    
    delegate: ItemDelegate {
        id: delegate
        width: control.width
        contentItem: Text {
            text: {
                if (control.textRole) {
                    if (typeof modelData !== "undefined" && modelData !== null) {
                        return modelData[control.textRole] ?? ""
                    }
                    if (typeof model !== "undefined" && model !== null) {
                        return model[control.textRole] ?? ""
                    }
                    return ""
                }
                return modelData ?? ""
            }
            color: delegate.highlighted ? Theme.popupHighlightText : Theme.popupItemText
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: delegate.highlighted ? Theme.popupHighlight : "transparent"
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
            if (!context) return;
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = control.pressed ? String(Theme.indicatorPressed) : String(Theme.indicatorColor);
            context.fill();
        }

        Connections {
            target: control
            function onPressedChanged() { canvas.requestPaint(); }
        }

        Connections {
            target: ThemeManager
            function onThemeChanged() { canvas.requestPaint(); }
        }
    }

    contentItem: Text {
        leftPadding: 12
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        font: control.font
        color: Theme.inputText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 40
        border.color: control.pressed || control.hovered ? Theme.borderAccent : Theme.inputBorder
        border.width: control.visualFocus ? 2 : 1
        radius: 8
        color: Theme.inputBackground

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
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
            border.color: Theme.popupBorder
            border.width: 1
            radius: 8
            color: Theme.popupBackground
            
            // Subtle shadow
            layer.enabled: true
            layer.effect: null // Placeholder for Shadow if needed
        }
    }
}
