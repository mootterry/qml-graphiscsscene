import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.12
import Scene 1.0
Window {
    visible: true
    width: 1280
    height: 720
    Row {
       id:toolbar
    spacing: 5
       ToolButton {
           text:"Select"
       }

       ToolButton {
           text:"Drag"
       }
    }

    SceneView {
        anchors.fill: parent
        anchors.topMargin: toolbar.height
    }

    title: qsTr("Hello World")
}
