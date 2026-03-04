import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Links

RowLayout {
    id: root
    
    signal closeClicked()
    
    height: 44
    spacing: 8
    
    Text {
        text: "选择共享内容"
        color: Theme.textPrimary
        font.pixelSize: 16
        font.weight: Font.Bold
    }
    
    Item { Layout.fillWidth: true }
    
    IconButton {
        iconSource: "qrc:/res/icon/close.png"
        hoverColor: Theme.hoverBackground
        onClicked: root.closeClicked()
    }
}
