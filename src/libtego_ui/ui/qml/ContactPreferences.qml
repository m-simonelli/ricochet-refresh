import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import im.ricochet 1.0

Item {
    property alias selectedContact: contacts.selectedContact

    RowLayout {
        anchors {
            fill: parent
            margins: 8
        }

        ContactList {
            id: contacts
            Layout.preferredWidth: 200
            Layout.minimumWidth: 150
            Layout.fillHeight: true
            frameVisible: true
            
            Accessible.role: Accessible.List
            Accessible.name: qsTr("Contact list")
        }

        data: [
            ContactActions {
                id: contactActions
                contact: contacts.selectedContact
            }
        ]

        ColumnLayout {
            id: contactInfo
            visible: contact !== null
            Layout.fillHeight: true
            Layout.fillWidth: true

            property QtObject contact: contacts.selectedContact
            property QtObject request: (contact !== null) ? contact.contactRequest : null

            Item { height: 1; width: 1 }
            Label {
                id: nickname
                Layout.fillWidth: true
                text: visible ? contactInfo.contact.nickname : ""
                textFormat: Text.PlainText
                horizontalAlignment: Qt.AlignHCenter
                font.pointSize: styleHelper.pointSize + 1

                property bool renameMode
                property Item renameItem
                onRenameModeChanged: {
                    if (renameMode && renameItem === null) {
                        renameItem = renameComponent.createObject(nickname)
                        renameItem.forceActiveFocus()
                        renameItem.selectAll()
                    } else if (!renameMode && renameItem !== null) {
                        renameItem.focus = false
                        renameItem.visible = false
                        renameItem.destroy()
                        renameItem = null
                    }
                }

                MouseArea { anchors.fill: parent; onDoubleClicked: nickname.renameMode = true }

                Component {
                    id: renameComponent

                    TextField {
                        id: nameField
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        text: contactInfo.contact.nickname
                        horizontalAlignment: nickname.horizontalAlignment
                        font.pointSize: nickname.font.pointSize
                        onEditingFinished: {
                            contactInfo.contact.nickname = text
                            nickname.renameMode = false
                        }
                    }
                }
            }
            Item { height: 1; width: 1 }

            ContactIDField {
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                readOnly: true
                text: visible ? contactInfo.contact.contactID : ""

                Accessible.role: Accessible.Pane // todo: what role best fits contactidfield?
                Accessible.name: qsTr("Contact ID for ") + visible ? nickname.text : qsTr("Unknown user") // todo: translations
                Accessible.description: text
            }

            Item { height: 1; width: 1 }
            Rectangle {
                color: palette.mid
                height: 1
                Layout.fillWidth: true
            }
            Item { height: 1; width: 1 }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: qsTr("Rename")
                    onClicked: nickname.renameMode = !nickname.renameMode

                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.description: qsTr("Renames this contact") // todo: translation
                }

                Item { Layout.fillWidth: true; height: 1 }

                Button {
                    text: qsTr("Remove")
                    onClicked: contactActions.removeContact()

                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.description: qsTr("Removes this contact") // todo: translation
                }
            }

            Item {
                Layout.fillHeight: true
                width: 1
            }

            Accessible.role: Accessible.Window
            Accessible.name: qsTr("Preferences for contact ") + visible ? nickname.text : qsTr("Unknown user") // todo: translations
        }
    }

    Accessible.role: Accessible.Window
    Accessible.name: qsTr("Contact Preferences Window") // todo: translation
    Accessible.description: qsTr("A list of all your contacts, with their ricochet IDs, and options such as renaming and removing") // todo: translation
}
