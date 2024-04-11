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
        Text{
            Layout.leftMargin: 20
            height:30
            text:"读取图片时间间隔:"
            font.pointSize: 12
        }

        TextField{
            id: textInputFrameId
            height:30
            width:40
            Layout.preferredWidth:  width
            Layout.rightMargin: 10
            font.pointSize: 12
            enabled:playsStates == false ? true : false
            text:myVideoItemId.frameSet
            validator: IntValidator{bottom: 0; top: 2000;}
            onTextChanged: {
                myVideoItemId.frameSet = Number(text)
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
            newDataPath(":/desktop.png");
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
            enabled: playsStates == false && myVideoItemId.currentNum > 0? true : false
            onClicked: {
                myVideoItemId.preImage()
            }

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
            enabled:playsStates == false && myVideoItemId.totalNum > 0?  true : false
            onClicked: {
                myVideoItemId.nextImage()
            }
        }
    }

    Text{
        anchors.top: parent.top
        anchors.topMargin: 50
        anchors.right: parent.right
        anchors.rightMargin: 20
        color:"red"
        font.pointSize: 12
        text: "读取数量:" + myVideoItemId.showNum + " / " + myVideoItemId.totalNum
    }
    Text{
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 50
        color:"green"
        font.pointSize: 12
        font.bold: true
        text: myVideoItemId.frameRate
    }

    FileDialog{
        id:fileDialogId
        title: "Please choose a file"
        //folder: shortcuts.home
        folder:"file:///" + "E:/images"
        selectExisting: true
        selectFolder: false
        onAccepted: {
            var strfile = "file:///"
            var fileUrl = fileDialogId.folder.toString()
            textInputFloderPathId.text = fileUrl.substring(strfile.length)
            console.log("You chose file  url: " + fileDialogId.fileUrl)
        }
        onRejected: {
            console.log("Canceled")
        }
    }
    onClosing:{
        myVideoItemId.onClosing()
    }
}
