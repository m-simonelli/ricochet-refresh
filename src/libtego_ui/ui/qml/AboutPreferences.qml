import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.15
import "utils.js" as Utils

ColumnLayout {
    anchors {
        fill: parent
        margins: 8
    }

    Text {
        Layout.fillWidth: true
        horizontalAlignment: Qt.AlignHCenter
        
        // this has to be split into 2 labels so that for accessibility,
        // it will read as <ricoString> version <versString with .'s replaced
        // with "point"s. i.e. here, accessibility will read as:
        //      Ricochet Refresh version 3 point 0 point 0
        // todo: translations for "version" and "point"
        RowLayout {
            anchors.centerIn: parent

            Text {
                id: ricoString
                text: "Ricochet Refresh"
                Accessible.ignored: true
            }
            Text {
                id: versString
                //: %1 version, e.g. 3.0.0
                text: qsTr("%1").arg(uiMain.version)
                Accessible.ignored: true
            }
            
            Accessible.description: qsTr("Current Ricochet Refresh version") // todo: translation
            Accessible.role: Accessible.StaticText
            Accessible.name: ricoString.text + qsTr("version ") + Utils.replace(versString.text, ".", qsTr(" point "))
        }
    }

    Label {
        horizontalAlignment: Qt.AlignHCenter
        Layout.fillWidth: true

        Label {
            id: homePageLink

            anchors.centerIn: parent

            text: "<a href='https://ricochetrefresh.net/'>ricochetrefresh.net</a>"
            onLinkActivated: Qt.openUrlExternally("https://ricochetrefresh.net")
        }

        Accessible.name: qsTr("Ricochet Refresh web home page") // todo: translation
        Accessible.role: Accessible.StaticText
    }

    Flickable {
        // this is so that the scroll bars doesn't spill over the border
        Layout.rightMargin: 8
        Layout.bottomMargin: 8

        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter

        TextArea.flickable : TextArea {
            selectByMouse: true

            readOnly: true
            text: uiMain.aboutText
        }

        ScrollBar.vertical: ScrollBar { }
        ScrollBar.horizontal: ScrollBar { } // openssl license has some long lines that need to be accounted for

        // todo: translation
        Accessible.description: qsTr("The license of ricochet refresh and it's dependencies")
        Accessible.name: qsTr("License") // todo: translation
        Accessible.role: Accessible.StaticText
    }

    Accessible.role: Accessible.Window
    Accessible.name: qsTr("About") // todo: translation
    Accessible.description: qsTr("About page, contains license and version information") // todo: translation
}

