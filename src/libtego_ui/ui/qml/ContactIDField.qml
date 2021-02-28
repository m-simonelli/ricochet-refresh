import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import QtQuick.Layouts 1.0
import im.ricochet 1.0
import "utils.js" as Utils

FocusScope {
    id: contactId
    z: 4
    height: layout.height
    Layout.fillWidth: true

    property alias text: field.text
    property alias readOnly: field.readOnly
    property alias horizontalAlignment: field.horizontalAlignment
    property alias acceptableInput: field.acceptableInput
    property bool showCopyButton: true

    RowLayout {
        id: layout
        width: parent.width

        TextField {
            id: field
            Layout.fillWidth: true
            font.family: "Courier"
            validator: readOnly ? null : idValidator
            placeholderText: "ricochet:"
            focus: true

            ContactIDValidator {
                id: idValidator

                onFailed: {
                    var contact
                    // XXX: how should accessibility be implemented here?
                    // should there be a sound for errors? 
                    if ((contact = matchingContact(field.text)))
                        errorBubble.show(qsTr("<b>%1</b> is already your contact").arg(Utils.htmlEscaped(contact.nickname)))
                    else if (matchesIdentity(field.text))
                        errorBubble.show(qsTr("You can't add yourself as a contact"))
                    else
                        errorBubble.show(qsTr("Enter an ID starting with <b>ricochet:</b>"))
                }
            }

            Bubble {
                id: errorBubble
                target: field
                horizontalAlignment: Qt.AlignLeft
                textFormat: Text.RichText

                function show(value) {
                    text = value
                    opacity = 1
                }

                function clear() {
                    opacity = 0
                }
            }

            function copyLoudly() {
                // The LinkedText helper also copies to the X11 selection clipboard
                LinkedText.copyToClipboard(field.text)
                copyBubble.displayed = true
                bubbleResetTimer.start()
            }

            MouseArea {
                anchors.fill: parent
                enabled: field.readOnly
                onClicked: field.copyLoudly()
            }

            Bubble {
                id: copyBubble
                target: field
                displayed: false

                text: qsTr("Copied to clipboard")

                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }

            Timer {
                id: bubbleResetTimer
                interval: 1000
                onTriggered: copyBubble.displayed = false
            }

            onTextChanged: errorBubble.clear()

            Accessible.role: Accessible.Dialog
            Accessible.name: text
            // todo: translation
            Accessible.description: qsTr("Field for the contact's ricochet I D") // todo: translation
        }

        Button {
            visible: contactId.showCopyButton

            text: qsTr("Copy")

            onClicked: field.copyLoudly()

            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: qsTr("Copies your ricochet I D") // todo: translation
            Accessible.onPressAction: field.copyLoudly()
        }
    }
}

