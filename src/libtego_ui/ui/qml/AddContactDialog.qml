import QtQuick 2.2
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    id: addContactWindow
    width: 620
    height: 300
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    flags: styleHelper.dialogWindowFlags
    modality: Qt.WindowModal
    title: mainWindow.title

    signal closed
    onVisibleChanged: if (!visible) closed()

    property string staticContactId: fields.contactId.text

    function close() {
        visible = false
    }

    function accept() {
        if (!fields.hasValidRequest)
            return

        userIdentity.contacts.createContactRequest(fields.contactId.text, fields.name.text, "", fields.message.text)
        close()
    }

    ColumnLayout {
        id: infoArea
        z: 2
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: 8
            leftMargin: 16
            rightMargin: 16
        }

        Label {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            horizontalAlignment: Qt.AlignHCenter
            wrapMode: Text.Wrap

            text: qsTr("Share your Ricochet ID to allow connection requests")
            
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }

        ContactIDField {
            id: localId
            Layout.fillWidth: true
            readOnly: true
            text: userIdentity.contactID
            horizontalAlignment: Qt.AlignLeft
        }

        Item { height: 1 }

        Rectangle {
            color: palette.mid
            height: 1
            Layout.fillWidth: true
            Layout.columnSpan: 2
        }

        Item { height: 1 }
    }

    ContactRequestFields {
        id: fields
        anchors {
            left: parent.left
            right: parent.right
            top: infoArea.bottom
            bottom: buttonRow.top
            margins: 8
            leftMargin: 16
            rightMargin: 16
        }

        Component.onCompleted: {
            if (staticContactId.length > 0) {
                fields.contactId.text = staticContactId
                fields.contactId.readOnly = true
                fields.name.focus = true
            } else {
                fields.contactId.focus = true
            }
        }
    }

    RowLayout {
        id: buttonRow
        anchors {
            right: parent.right
            bottom: parent.bottom
            rightMargin: 16
            bottomMargin: 8
        }

        Button {
            text: qsTr("Cancel")

            onClicked: addContactWindow.close()

            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: qsTr("Closes the contact add window") // todo: translation
            Accessible.onPressAction: addContactWindow.close()
        }

        Button {
            isDefault: true
            enabled: fields.hasValidRequest

            text: qsTr("Add")

            onClicked: addContactWindow.accept()

            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: qsTr("Adds the contact") // todo: translation
            Accessible.onPressAction: addContactWindow.close()
        }
    }

    Action {
        shortcut: StandardKey.Close
        onTriggered: addContactWindow.close()
    }

    Action {
        shortcut: "Escape"
        onTriggered: addContactWindow.close()
    }

    Accessible.role: Accessible.Dialog
    Accessible.name: qsTr("Add Contact") // todo: translation
    Accessible.description: qsTr("Dialog to add contacts from ") // todo: translation
}

