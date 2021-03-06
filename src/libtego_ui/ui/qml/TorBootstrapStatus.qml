import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Column {
    id: page
    spacing: 8

    property var bootstrap: torControl.bootstrapStatus
    onBootstrapChanged: {
        if (bootstrap['tag'] === "done")
            window.networkReady()
    }

    Label {
        //: \u2026 is ellipsis
        text: qsTr("Connecting to the Tor network\u2026")
        font.bold: true

        Accessible.role: Accessible.StaticText
        Accessible.name: text
        Accessible.description: qsTr("Connection status");
    }

    ProgressBar {
        width: parent.width
        maximumValue: 100
        indeterminate: bootstrap.progress === undefined
        value: bootstrap.progress === undefined ? 0 : bootstrap.progress

        Accessible.role: Accessible.ProgressBar
        Accessible.description: qsTr("Connection progress");
    }

    Label {
        text: (bootstrap['warning'] !== undefined ) ? bootstrap['warning'] : bootstrap['summary']
        textFormat: Text.PlainText
    }

    TorLogDisplay {
        id: logDisplay
        width: parent.width
        height: 0
        visible: height > 0

        Behavior on height {
            SmoothedAnimation {
                easing.type: Easing.InOutQuad
                velocity: 1500
            }
        }
    }

    RowLayout {
        width: parent.width

        Button {
            text: qsTr("Back")
            onClicked: window.back()

            Accessible.name: qsTr("Back")
            Accessible.onPressAction: window.back()
        }

        Item { height: 1; Layout.fillWidth: true }

        Button {
            text: logDisplay.height ? qsTr("Hide details") : qsTr("Show details")
            onClicked: {
                if (logDisplay.height)
                    logDisplay.height = 0
                else
                    logDisplay.height = 300
            }

            Accessible.name: text
            Accessible.role: Accessible.Button
            Accessible.description: logDisplay.height ? qsTr("Hides the tor progress log") : qsTr("Shows the tor progress log") // todo: translations
            Accessible.onPressAction: {
                if (logDisplay.height) logDisplay.height = 0
                else logDisplay.height = 300
            }
        }

        Item { height: 1; Layout.fillWidth: true }

        Button {
            text: qsTr("Done")
            isDefault: true
            enabled: bootstrap.tag === "done"
            onClicked: window.visible = false

            Accessible.name: text
            Accessible.role: Accessible.Button
            Accessible.onPressAction: window.visible = false
        }
    }
}

