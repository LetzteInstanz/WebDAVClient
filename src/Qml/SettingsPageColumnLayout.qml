import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "Core" as Core

ColumnLayout {
    function prepare() {
        //pathTxtField.text = settings.getDownloadPath()
        logLevelComboBox.currentIndex = settings.getCurrentLogLevel()
        saveSettingsButton.enabled = false
    }
    function back() { stackLayout.currentIndex = 0 }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: saveSettingsButton.height

        Core.Button {
            text: qsTr("Back")
            onClicked: back()
        }
        Core.Button {
            id: saveSettingsButton
            anchors.right: parent.right
            text: qsTr("Ok")
            onClicked: {
                //settings.setDownloadPath(pathTxtField.text)
                settings.setCurrentLogLevel(logLevelComboBox.currentIndex)
                back()
            }
        }
    }
    Core.BorderRectangle {
        Layout.fillHeight: true
        Layout.fillWidth: true

        Column {
            id: settingsColumnLayout
            anchors.fill: parent
            anchors.margins: 5
            spacing: 5
            function hasChanges() { return /*settings.getDownloadPath() !== pathTxtField.text || */settings.getCurrentLogLevel() !== logLevelComboBox.currentIndex }

            // Label {
            //     text: qsTr("Path:")
            // }
            // Row {
            //     width: parent.width
            //     spacing: 5

            //     TextField {
            //         id: pathTxtField
            //         width: parent.width - parent.spacing - pathButton.width
            //         readOnly: true
            //         onTextChanged: saveSettingsButton.enabled = settingsColumnLayout.hasChanges()
            //         onReleased: (event) => { textContextMenu.hanldeReleaseEvent(pathTxtField, event) }
            //     }
            //     Button {
            //         id: pathButton
            //         text: qsTr("Select")
            //         onClicked: {
            //             pathDlg.currentFolder = encodeURIComponent("file://" + pathTxtField.text)
            //             pathDlg.open()
            //         }
            //     }
            // }
            Label {
                text: qsTr("Maximum log level:")
            }
            ComboBox {
                id: logLevelComboBox
                model: logLevelItemModel
                delegate: ItemDelegate {
                    text: modelData
                    required property string modelData
                }
                onActivated: saveSettingsButton.enabled = settingsColumnLayout.hasChanges()
            }
        }
    }
}
