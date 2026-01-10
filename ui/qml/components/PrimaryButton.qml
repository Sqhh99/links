import QtQuick
import QtQuick.Controls

Button {
    id: root
    
    property bool loading: false
    
    implicitHeight: 46
    
    background: Rectangle {
        color: {
            if (!root.enabled) return "#E5E7EB"
            if (root.pressed) return "#1E40AF"
            if (root.hovered) return "#1D4ED8"
            return "#2563EB"
        }
        radius: 10
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
    
    contentItem: Row {
        spacing: 8
        anchors.centerIn: parent
        
        BusyIndicator {
            visible: root.loading
            running: root.loading
            width: 18
            height: 18
            anchors.verticalCenter: parent.verticalCenter
            palette.dark: "white"
        }
        
        Text {
            text: root.text
            color: root.enabled ? "#ffffff" : "#9CA3AF"
            font.pixelSize: 15
            font.weight: Font.Bold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) { mouse.accepted = false }
    }
}
