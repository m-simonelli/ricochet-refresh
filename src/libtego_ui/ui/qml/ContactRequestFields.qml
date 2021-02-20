import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import im.ricochet 1.0

GridLayout {
    id: contactFields
    columns: 2

    property bool readOnly
    property ContactIDField contactId: contactIdField
    property TextField name: nameField
    property TextArea message: messageField
    property bool hasValidRequest: contactIdField.acceptableInput && nameField.text.length

    Label {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

        text: qsTr("ID:")

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    ContactIDField {
        id: contactIdField

        Layout.fillWidth: true
        readOnly: contactFields.readOnly
        showCopyButton: false
    }

    Label {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

        text: qsTr("Name:")

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    TextField {
        id: nameField

        Layout.fillWidth: true
        readOnly: contactFields.readOnly

        Accessible.role: Accessible.Dialog
        Accessible.name: text
        Accessible.description: qsTr("Field to enter the contact's name into") // todo: translation
    }

    Label {
        Layout.alignment: Qt.AlignTop | Qt.AlignRight

        text: qsTr("Message:")

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    TextArea {
        id: messageField
        
        textFormat: TextEdit.PlainText
        readOnly: contactFields.readOnly
        Layout.fillWidth: true
        Layout.fillHeight: true

        Accessible.role: Accessible.Dialog
        Accessible.name: text
        Accessible.description: qsTr("Field for the contact's message") // todo: translation
    }
}
