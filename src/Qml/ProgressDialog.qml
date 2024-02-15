import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "Core" as Core
import "Util.js" as Util

Dialog {
    anchors.centerIn: parent
    width: parent.width / 2
    modal: true
    closePolicy: Popup.CloseOnEscape
    standardButtons: Dialog.Cancel
    background: Core.BorderRectangle {}

    ColumnLayout {
        anchors.fill: parent

        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            id: textLabel
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
        }
        Connections {
            target: fileSystemModel
            function onMaxProgressEnabled(enabled) { progressBar.indeterminate = !enabled }
            function onProgressChanged(value) { progressBar.value = value }
            function onMaxProgressChanged(max) { progressBar.to = max }
            function onProgressTextChanged(text) { textLabel.text = text }
            function onErrorOccurred(text) {
                console.debug("QML: An error occurred")
                function createDlg(comp) {
                    const dlg = Util.createPopup(comp, appWindow, "CustomMessageBox", {"standardButtons": Dialog.Ok, "text": text})
                    if (dlg === null)
                        return

                    dlg.closed.connect(reject)
                    dlg.open()
                }

                Util.createObjAsync(msgBoxComponent, createDlg)
            }
            function onReplyGot() { console.debug("QML: A reply was received"); accept() }
        }
    }
}
