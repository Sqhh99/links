import QtQuick
import QtQuick.Effects
import Links

// Lucide icon from res/icon/lucide/, tinted to `color`.
// The SVGs are stored with a white stroke so colorization maps them exactly
// onto the requested color. The root is an Item because Image's implicit
// size is read-only (derived from the source).
Item {
    id: root

    property string name: ""
    property int size: 18
    property color color: Theme.iconSecondary

    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.fill: parent
        source: root.name.length > 0 ? "qrc:/res/icon/lucide/" + root.name + ".svg" : ""
        sourceSize.width: root.size
        sourceSize.height: root.size
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true

        layer.enabled: root.name.length > 0
        layer.smooth: true
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: root.color
        }
    }
}
