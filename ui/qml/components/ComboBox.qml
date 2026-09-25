import QtQuick
import Links
import QtQuick.Controls
import QtQuick.Effects
import Links.Backend 1.0

ComboBox {
    id: control

    implicitHeight: 36
    font.pixelSize: 13

    delegate: ItemDelegate {
        id: delegate
        width: control.popup.availableWidth
        height: 32
        contentItem: Text {
            leftPadding: 4
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
            color: control.currentIndex === index ? Theme.popupHighlightText : Theme.popupItemText
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: delegate.highlighted ? Theme.hoverBackground
                                        : (control.currentIndex === index ? Theme.popupHighlight : "transparent")
            radius: Theme.radiusSm
        }
        highlighted: control.highlightedIndex === index
    }

    indicator: Icon {
        x: control.width - width - 10
        y: (control.height - height) / 2
        name: "chevron-down"
        size: 16
        color: control.pressed || control.popup.visible ? Theme.brand : Theme.iconSecondary
        rotation: control.popup.visible ? 180 : 0

        Behavior on rotation { NumberAnimation { duration: 150 } }
    }

    contentItem: Text {
        leftPadding: 12
        rightPadding: 34

        text: control.displayText
        font: control.font
        color: Theme.inputText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 36
        border.color: control.popup.visible || control.visualFocus ? Theme.inputBorderFocus
                     : (control.hovered ? Theme.borderColor : Theme.inputBorder)
        border.width: 1
        radius: 8
        color: Theme.inputBackground

        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    popup: Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 280)
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            spacing: 2
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            border.color: Theme.popupBorder
            border.width: 1
            radius: 10
            color: Theme.popupBackground

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Theme.shadowColor
                shadowBlur: 0.6
                shadowVerticalOffset: 4
            }
        }
    }
}
