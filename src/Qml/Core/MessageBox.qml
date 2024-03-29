import QtQuick
import QtQuick.Controls

// MessageDialog { // note: Doesn't work under Android in Qt6.6 version: https://forum.qt.io/topic/142589/messagedialog-not-working-on-android-qt6-4-0
//     id: removeMsgDlg
//     buttons: MessageDialog.Yes | MessageDialog.No
//     onAccepted: srvListView.model.removeRow(srvListView.currentIndex)
// }
Dialog {
    id: dlg
    anchors.centerIn: parent
    modal: true
    closePolicy: Popup.CloseOnEscape
    background: BorderRectangle {}
    property string text

    contentItem: Label {
        id: txtLabel
        horizontalAlignment: Text.AlignHCenter
        text: dlg.text
    }
}
