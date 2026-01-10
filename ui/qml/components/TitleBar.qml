import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    property string title: "LiveKit Conference"
    property string iconSource: "qrc:/res/icon/video.png"
    property bool showSettingsButton: true
    
    signal settingsClicked()
    signal minimizeClicked()
    signal closeClicked()
    
    height: 48
    color: "transparent"
    
    // Drag handler for window movement
    property var targetWindow: null
    property point dragStartPos
    property bool dragging: false
    
    MouseArea {
        anchors.fill: parent
        
        onPressed: function(mouse) {
            if (root.targetWindow) {
                root.dragging = true
                root.dragStartPos = Qt.point(mouse.x, mouse.y)
            }
        }
        
        onPositionChanged: function(mouse) {
            if (root.dragging && root.targetWindow) {
                var delta = Qt.point(mouse.x - root.dragStartPos.x, 
                                     mouse.y - root.dragStartPos.y)
                root.targetWindow.x += delta.x
                root.targetWindow.y += delta.y
            }
        }
        
        onReleased: {
            root.dragging = false
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 10
        
        Image {
            source: root.iconSource
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            fillMode: Image.PreserveAspectFit
            smooth: true
        }
        
        Text {
            text: root.title
            color: "#111827"
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
        
        Item { Layout.fillWidth: true }
        
        IconButton {
            id: settingsBtn
            visible: root.showSettingsButton
            iconSource: "qrc:/res/icon/set_up.png"
            toolTipText: "Settings"
            onClicked: root.settingsClicked()
        }
        
        IconButton {
            id: minimizeBtn
            iconSource: "qrc:/res/icon/minimize.png"
            onClicked: root.minimizeClicked()
        }
        
        IconButton {
            id: closeBtn
            iconSource: "qrc:/res/icon/close.png"
            hoverColor: "#26ff5252"  // 15% red
            onClicked: root.closeClicked()
        }
    }
}
