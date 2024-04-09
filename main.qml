import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import QtQml 2.15
import SceneGraphRender 1.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("QMLVideoItem")
    color: "#f0f0f0"
    property bool playsStates: myVideoItemId.playing

    RowLayout{
        id:titleRowLayoutId
        height:35
        width:parent.width
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 5
        Button{
            height:30
            width:100
            Layout.leftMargin: 10
            text:"选择文件目录"
            onClicked: {
                fileDialogId.open()
            }
        }
        TextField{
            id:textInputFloderPathId
            height:30
            Layout.fillWidth: true
            placeholderText:"文件目录"
            selectByMouse: true
            onTextChanged: {
                myVideoItemId.setImageFolder(text)
            }
        }
    }

    MyVideoItem {
        id:myVideoItemId
        anchors.top: titleRowLayoutId.bottom
        anchors.left:  parent.left
        anchors.right: parent.right
        anchors.bottom: rowId.top
        Component.onCompleted: {
            newData(":/desktop.png");
        }
    }

    RowLayout{
        id:rowId
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        height: 50
        spacing: 10
        Button{
            height:30
            width:60
            text: "上一张"
            enabled: playsStates == false ? true : false

        }
        Button{
            height:30
            width:60
            text: playsStates == false ? "播放" : "暂停"
            onClicked: {
                myVideoItemId.playStart();
            }


        }
        Button{
            height:30
            width:60
            text: "下一张"
            enabled:playsStates == false ? true : false

        }
    }

    FileDialog{
        id:fileDialogId
        title: "Please choose a file"
        //folder: shortcuts.home
        folder:"file:///E:/WORKSPACE/ImageRead/Images"
        selectFolder: true
        onAccepted: {
            var strfile = "file:///"
            var fileUrl = fileDialogId.fileUrl.toString()
            textInputFloderPathId.text = fileUrl.substring(strfile.length)
            console.log("You chose file  url: " + fileDialogId.fileUrl)
        }
        onRejected: {
            console.log("Canceled")
        }
    }
}
