import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

TextArea {
    id: logDisplay
    readOnly: true
    text: torInstance.logMessages.join('\n')
    textFormat: TextEdit.PlainText
    wrapMode: TextEdit.Wrap

    Connections {
        target: torInstance
        function onLogMessage(message) {
            logDisplay.append(message)
        }
    }

    Accessible.name: qsTr("Tor log") // todo: translation
    Accessible.role: Accessible.StaticText
    Accessible.description: text // XXX: seems like a bad idea to have the entire log read out
}
