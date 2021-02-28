import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.1
import "ContactWindow.js" as ContactWindow

Item {
    id: contactMenu

    property QtObject contact

    function openWindow() {
        var window = ContactWindow.getWindow(contact)
        window.raise()
        window.requestActivate()
    }

    function removeContact() {
        removeContactDialog.active = true
        if (removeContactDialog.item !== null) {
            removeContactDialog.item.open()
        } else if (uiMain.showRemoveContactDialog(contact)) {
            contact.deleteContact()
        }
    }

    function openContextMenu() {
        contextMenu.popup()
    }

    function openPreferences() {
        root.openPreferences("ContactPreferences.qml", { 'selectedContact': contact })
    }

    signal renameTriggered

    Menu {
        id: contextMenu

        MenuItem {
            text: qsTr("Open Window")
            onTriggered: openWindow()

            Accessible.role: QAccessible.StaticText
            Accessible.name: text
        }
        MenuItem {
            text: qsTr("Details...")
            onTriggered: openPreferences()

            Accessible.role: QAccessible.StaticText
            Accessible.name: text
        }
        MenuItem {
            text: qsTr("Rename")
            onTriggered: renameTriggered()

            Accessible.role: QAccessible.StaticText
            Accessible.name: text
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr("Remove")
            onTriggered: removeContact()

            Accessible.role: QAccessible.StaticText
            Accessible.name: text
        }

        Accessible.role: QAccessible.List
        Accessible.name: qsTr("Contact options")
    }

    Loader {
        id: removeContactDialog
        source: "MessageDialogWrapper.qml"
        active: false
    }
}

