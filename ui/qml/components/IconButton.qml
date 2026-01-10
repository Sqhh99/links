import QtQuick
import QtQuick.Controls

Button {
    id: root
    
    property string iconSource: ""
    property string toolTipText: ""
    property string hoverColor: "#F3F4F6"
    
    implicitWidth: 32
    implicitHeight: 24
    
    background: Rectangle {
        color: root.checked ? "#2563EB" : (root.hovered ? root.hoverColor : "transparent")
        radius: 4
        
        Behavior on color {
            ColorAnimation { duration: 100 }
        }
    }
    
    contentItem: Image {
        source: root.iconSource
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignVCenter
        sourceSize.width: 14
        sourceSize.height: 14
        smooth: true
    }
    
    ToolTip.visible: toolTipText.length > 0 && hovered
    ToolTip.text: toolTipText
    ToolTip.delay: 500
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
