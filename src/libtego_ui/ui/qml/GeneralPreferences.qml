import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import im.ricochet 1.0

ColumnLayout {
    anchors {
        fill: parent
        margins: 8
    }

    CheckBox {
        text: qsTr("Use a single window for conversations")
        checked: uiSettings.data.combinedChatWindow || false
        onCheckedChanged: {
            uiSettings.write("combinedChatWindow", checked)
        }

        Accessible.role: Accessible.CheckBox
        Accessible.name: text
        Accessible.onPressAction: {
            uiSettings.write("combinedChatWindow", checked)
        }
    }

    CheckBox {
        text: qsTr("Open links in default browser without prompting")
        checked: uiSettings.data.alwaysOpenBrowser || false
        onCheckedChanged: {
            uiSettings.write("alwaysOpenBrowser", checked)
        }

        Accessible.role: Accessible.CheckBox
        Accessible.name: text
        Accessible.onPressAction: {
            uiSettings.write("alwaysOpenBrowser", checked)
        }
    }

    CheckBox {
        text: qsTr("Play audio notifications")
        checked: uiSettings.data.playAudioNotification || false
        onCheckedChanged: {
            uiSettings.write("playAudioNotification", checked)
        }

        Accessible.role: Accessible.CheckBox
        Accessible.name: text
        Accessible.onPressAction: {
            uiSettings.write("playAudioNotification", checked)
        }
    }
    RowLayout {
        Item { width: 16 }

        Label {
            text: qsTr("Volume")
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }

        Slider {
            maximumValue: 1.0
            updateValueWhileDragging: false
            enabled: uiSettings.data.playAudioNotification || false
            value: uiSettings.read("notificationVolume", 0.75)
            onValueChanged: {
                uiSettings.write("notificationVolume", value)
            }

            Accessible.role: Accessible.Slider
            Accessible.name: qsTr("Volume")
            Accessible.onIncreaseAction: {
                value += 0.125 // 8 volume settings
            }
            Accessible.onDecreaseAction: {
                value -= 0.125
            }
        }
    }

    RowLayout {
        z: 2
        Label {
            text: qsTr("Language")
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }

        ComboBox {
            id: languageBox
            model: languageModel
            textRole: "nativeName"
            currentIndex: languageModel.rowForLocaleID(uiSettings.data.language)
            Layout.minimumWidth: 200

            LanguagesModel {
                id: languageModel
            }

            onActivated: {
                var localeID = languageModel.localeID(index)
                uiSettings.write("language", localeID)
                restartBubble.displayed = true
                bubbleResetTimer.start()
            }

            Bubble {
                id: restartBubble
                target: languageBox
                text: qsTr("Restart Ricochet to apply changes")
                displayed: false
                horizontalAlignment: Qt.AlignRight

                Timer {
                    id: bubbleResetTimer
                    interval: 3000
                    onTriggered: restartBubble.displayed = false
                }
            }

            Accessible.role: Accessible.ComboBox
            Accessible.name: qsTr("Languages") // todo: translation
            Accessible.description: qsTr("What language ricochet will use") // todo: translation
        }
    }


    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}
