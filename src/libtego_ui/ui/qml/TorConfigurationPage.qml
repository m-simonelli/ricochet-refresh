import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Column {
    id: setup
    spacing: 8

    property alias proxyType: proxyTypeField.selectedType
    property alias proxyAddress: proxyAddressField.text
    property alias proxyPort: proxyPortField.text
    property alias proxyUsername: proxyUsernameField.text
    property alias proxyPassword: proxyPasswordField.text
    property alias allowedPorts: allowedPortsField.text
    property alias bridges: bridgesField.text

    function reset() {
        proxyTypeField.currentIndex = 0
        proxyAddress = ''
        proxyPort = ''
        proxyUsername = ''
        proxyPassword = ''
        allowedPorts = ''
        bridges = ''
    }

    function save() {
        var conf = {};
        conf.disableNetwork = "0";
        conf.proxyType = proxyType;
        conf.proxyAddress = proxyAddress;
        conf.proxyPort = proxyPort;
        conf.proxyUsername = proxyUsername;
        conf.proxyPassword = proxyPassword;
        conf.allowedPorts = allowedPorts.trim().length > 0 ? allowedPorts.trim().split(',') : [];
        conf.bridges = bridges.trim().length > 0 ? bridges.trim().split('\n') : [];

        var command = torControl.setConfiguration(conf)
        command.finished.connect(function() {
            if (command.successful) {
                if (torControl.hasOwnership)
                    torControl.saveConfiguration()
                window.openBootstrap()
            } else
                console.log("SETCONF error:", command.errorMessage)
        })
    }

    Label {
        width: parent.width
        text: qsTr("Does this computer need a proxy to access the internet?")
        wrapMode: Text.Wrap

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    GroupBox {
        width: setup.width

        GridLayout {
            anchors.fill: parent
            columns: 2

            /* without this the top of groupbox clips into the first row */
            Item { height: 15; width: 1 } // XXX: this fixes a bug in linux, does it cause issues on other platforms?
            Item { height: 15; width: 1 }

            Label {
                text: qsTr("Proxy type:")
                color: proxyPalette.text
            }
            ComboBox {
                id: proxyTypeField
                property string none: qsTr("None")
                model: [
                    { "text": qsTr("None"), "type": "" },
                    { "text": "SOCKS 4", "type": "socks4" },
                    { "text": "SOCKS 5", "type": "socks5" },
                    { "text": "HTTPS", "type": "https" },
                ]
                textRole: "text"
                property string selectedType: currentIndex >= 0 ? model[currentIndex].type : ""

                SystemPalette {
                    id: proxyPalette
                    colorGroup: setup.proxyType == "" ? SystemPalette.Disabled : SystemPalette.Active
                }

                Accessible.role: Accessible.ComboBox
                Accessible.name: selectedType
                Accessible.description: qsTr("If you need a proxy to access the internet, select one from this list.") // todo: translation
            }

            Label {
                text: qsTr("Address:")
                color: proxyPalette.text

                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: proxyAddressField
                    Layout.fillWidth: true
                    enabled: setup.proxyType
                    placeholderText: qsTr("IP address or hostname")

                    Accessible.role: Accessible.EditableText
                    Accessible.name: placeholderText
                    Accessible.description: qsTr("Enter the IP address or hostname of the proxy you wish to connect to") // todo: translation
                }
                Label {
                    text: qsTr("Port:")
                    color: proxyPalette.text

                }
                TextField {
                    id: proxyPortField
                    Layout.preferredWidth: 50
                    enabled: setup.proxyType

                    Accessible.role: Accessible.EditableText
                    Accessible.name: qsTr("Port") // todo: translation
                    Accessible.description: qsTr("Enter the port of the proxy you wish to connect to") // todo: translation
                }
            }

            Label {
                text: qsTr("Username:")
                color: proxyPalette.text

                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            RowLayout {
                Layout.fillWidth: true

                TextField {
                    id: proxyUsernameField
                    Layout.fillWidth: true
                    enabled: setup.proxyType
                    placeholderText: qsTr("Optional")

                    Accessible.role: Accessible.EditableText
                    Accessible.name: qsTr("Username") // todo: translation
                    Accessible.description: qsTr("If required, enter the username for the proxy you wish to connect to") // todo: translation
                }
                Label {
                    text: qsTr("Password:")
                    color: proxyPalette.text

                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                }
                TextField {
                    id: proxyPasswordField
                    Layout.fillWidth: true
                    enabled: setup.proxyType
                    placeholderText: qsTr("Optional")

                    Accessible.role: Accessible.EditableText
                    Accessible.name: qsTr("Password") // todo: translation
                    Accessible.description: qsTr("If required, enter the password for the proxy you wish to connect to") // todo: translation
                }
            }
        }
    }

    Item { height: 4; width: 1 }

    Label {
        width: parent.width
        text: qsTr("Does this computer's Internet connection go through a firewall that only allows connections to certain ports?")
        wrapMode: Text.Wrap

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    GroupBox {
        width: parent.width
        // Workaround OS X visual bug
        height: Math.max(implicitHeight, 40)

        /* without this the top of groupbox clips into the first row */
        ColumnLayout {
            anchors.fill: parent

            Item { height: 15; width: 1 } // XXX: this fixes a bug in linux, does it cause issues on other platforms?

            RowLayout {
                Label {
                    text: qsTr("Allowed ports:")

                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                }
                TextField {
                    id: allowedPortsField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Example: 80,443")
                    
                    Accessible.role: Accessible.EditableText
                    Accessible.name: qsTr("Allowed ports") // todo: translations
                    Accessible.description: placeholderText
                }
            }
        }
    }

    Item { height: 4; width: 1 }

    Label {
        width: parent.width
        text: qsTr("If this computer's Internet connection is censored, you will need to obtain and use bridge relays.")
        wrapMode: Text.Wrap

        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }

    GroupBox {
        width: parent.width
        ColumnLayout {
            anchors.fill: parent

            Item { height: 15; width: 1 } // XXX: this fixes a bug in linux, does it cause issues on other platforms?

            Label {
                text: qsTr("Enter one or more bridge relays (one per line):")

                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            TextArea {
                id: bridgesField
                Layout.fillWidth: true
                Layout.preferredHeight: allowedPortsField.height * 2
                tabChangesFocus: true

                Accessible.name: qsTr("Enter one or more bridge relays (one per line):")
                Accessible.role: Accessible.EditableText
            }
        }
    }

    RowLayout {
        width: parent.width

        Button {
            text: qsTr("Back")
            onClicked: window.back()

            Accessible.name: text
            Accessible.onPressAction: window.back()
        }

        Item { height: 1; Layout.fillWidth: true }

        Button {
            text: qsTr("Connect")
            isDefault: true
            onClicked: {
                setup.save()
            }
            
            Accessible.name: text
            Accessible.onPressAction: setup.save()
        }
    }
}
