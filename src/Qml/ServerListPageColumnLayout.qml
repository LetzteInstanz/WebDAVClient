import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    RowLayout {
        Button {
            text: qsTr("Add")
            property Component editSrvDlgComponent
            Component.onCompleted: editSrvDlgComponent = Qt.createComponent("EditServerDialog.qml", Component.Asynchronous);
            onClicked: {
                function createDlg() {
                    const comp = editSrvDlgComponent
                    if (comp.status === Component.Error) {
                        console.error("EditServerDialog.qml component loading failed: ", comp.errorString());
                        return;
                    }
                    const dlg = comp.createObject(appWindow, {"title": qsTr("Add server")});
                    if (dlg === null) {
                        console.error("EditServerDialog object creation failed");
                        return;
                    }
                    dlg.setData("", "", 80, "")
                    dlg.accepted.connect(() => { srvListView.model.addServerInfo(dlg.desc(), dlg.addr(), dlg.port(), dlg.path()) })
                    dlg.closed.connect(dlg.destroy)
                    dlg.open()
                }

                const comp = editSrvDlgComponent
                if (comp.status === Component.Ready)
                    createDlg();
                else
                    comp.statusChanged.connect(createDlg);
            }
        }
        Button {
            text: qsTr("Settings")
            onClicked: {
                //pathTxtField.text = settings.getDownloadPath()
                logLevelComboBox.currentIndex = settings.getCurrentLogLevel()
                saveSettingsButton.enabled = false
                stackLayout.currentIndex = 2
            }
        }
        Button {
            text: qsTr("Log")
            onClicked: stackLayout.currentIndex = 3
        }
    }
    BorderRectangle {
        Layout.fillHeight: true
        Layout.fillWidth: true

        EditServerItemMenu {
            id: editItemMenu
            view: srvListView
        }
        ListView {
            id: srvListView
            anchors.fill: parent
            model: srvItemModel
            clip: true
            anchors.margins: 2
            spacing: 5
            highlightFollowsCurrentItem: true
            currentIndex: -1

            highlight: Rectangle {
                width: ListView.view.width
                color: "lightgray"
            }
            delegate: Item {
                id: delegate
                width: ListView.view.width
                height: descText.contentHeight + paramText.contentHeight
                required property int index
                required property var model
                required property string desc
                required property string addr
                required property int port
                required property string path

                Text {
                    id: descText
                    font.bold: true
                    font.pointSize: 14
                    elide: Text.ElideRight
                    text: desc
                }
                Text {
                    id: paramText
                    anchors.top: descText.bottom
                    elide: Text.ElideRight
                    text: "http://" + addr + ":" + port + "/" + path
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        delegate.ListView.view.currentIndex = index
                        delayTimer.start()
                    }
                    onPressAndHold: (mouse) => {
                        const view = delegate.ListView.view
                        view.currentIndex = index
                        editItemMenu.popup(view.currentItem, mouse.x, mouse.y)
                    }

                    Timer {
                        id: delayTimer
                        interval: 100
                        repeat: false
                        property Component progressDlgComponent
                        Component.onCompleted: progressDlgComponent = Qt.createComponent("ProgressDialog.qml", Component.Asynchronous);
                        onTriggered: {
                            function createDlg() {
                                const comp = progressDlgComponent
                                if (comp.status === Component.Error) {
                                    console.error("ProgressDialog.qml component loading failed: ", comp.errorString());
                                    return;
                                }
                                const dlg = comp.createObject(appWindow);
                                if (dlg === null) {
                                    console.error("ProgressDialog object creation failed");
                                    return;
                                }
                                function requestFileList() {
                                    stackLayout.currentIndex = 1
                                    const item = srvListView.currentItem
                                    fileSystemModel.setServerInfo(item.addr, item.port, item.path)
                                    fileSystemModel.requestFileList()
                                }
                                dlg.onOpened.connect(requestFileList)
                                dlg.rejected.connect(() => { stackLayout.currentIndex = 0 })
                                dlg.closed.connect(dlg.destroy)
                                dlg.open()
                            }

                            const comp = progressDlgComponent
                            if (comp.status === Component.Ready)
                                createDlg();
                            else
                                comp.statusChanged.connect(createDlg);
                        }
                    }
                }
            }
        }
    }
}