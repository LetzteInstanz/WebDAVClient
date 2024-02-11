import QtQml
import QtQuick
import QtQuick.Layouts

import "Util.js" as Util

BorderRectangle {
    Layout.fillHeight: true
    Layout.fillWidth: true
    function setViewModelAsync() {
        function setModel() {
            console.debug("QML: The file view model was set")
            view.model = fileItemModel
            view.model.modelReset.connect(() => { view.currentIndex = -1 })
            fileSystemModel.replyGot.disconnect(setModel)
        }
        fileSystemModel.replyGot.connect(setModel)
        fileSystemModel.errorOccurred.connect(() => { fileSystemModel.replyGot.disconnect(setModel) })
    }

    ListView {
        id: view
        anchors.fill: parent
        anchors.margins: 2
        model: null
        clip: true
        spacing: 5
        highlightFollowsCurrentItem: true
        highlight: Rectangle {
            width: ListView.view.width
            color: "lightgray"
        }
        delegate: BorderRectangle {
            id: delegate
            height: (Math.max(image.height, nameText.contentHeight + dateTimeText.contentHeight) + rowLayout.anchors.topMargin + rowLayout.anchors.bottomMargin)
            width: ListView.view.width
            color: "transparent"
            required property int index
            required property var model

            RowLayout {
                id: rowLayout
                anchors.fill: parent
                anchors.margins: 2

                Image {
                    id: image
                    source: "image://icons/" + model.extension
                }
                ColumnLayout {
                    Text {
                        id: nameText
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.verticalStretchFactor: 1
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        font.bold: true
                        font.pointSize: 14
                        wrapMode: Text.Wrap
                        text: model.name
                    }
                    Text {
                        id: dateTimeText
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.preferredHeight: 16
                        verticalAlignment: Text.AlignBottom
                        horizontalAlignment: Text.AlignRight
                        text: model.datetime
                    }
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    delegate.ListView.view.currentIndex = index
                    if (model.isFile)
                        return

                    delayTimer.start()
                }

                Timer {
                    id: delayTimer
                    interval: 100
                    repeat: false
                    onTriggered: {
                        function createDlg(comp) {
                            const dlg = Util.createPopup(comp, appWindow, "ProgressDialog", {})
                            if (dlg === null)
                                return

                            dlg.onOpened.connect(() => { console.debug("QML: A new file list was requested"); fileSystemModel.requestFileList(model.name) })
                            dlg.open()
                        }

                        Util.createObjAsync(progressDlgComponent, createDlg)
                    }
                }
            }
        }
    }
}