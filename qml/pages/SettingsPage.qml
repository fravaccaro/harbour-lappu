import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: page.width

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                text: qsTr("Notes")
            }

            ComboBox {
                property bool ready: false

                width: parent.width
                label: qsTr("Sort")
                Component.onCompleted: {
                    currentIndex = app.notes.sortMode === "title" ? 1 : 0
                    ready = true
                }
                onCurrentIndexChanged: {
                    if (ready && currentIndex >= 0)
                        app.notes.sortMode = currentIndex === 1 ? "title" : "modified"
                }

                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Last modified")
                    }

                    MenuItem {
                        text: qsTr("Title")
                    }
                }
            }

            SectionHeader {
                text: qsTr("Connection")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                text: Muistio.serverUrl !== "" ? Muistio.serverUrl : qsTr("Not signed in")
            }

            TextSwitch {
                width: parent.width
                text: qsTr("Ignore certificate errors")
                description: qsTr("Only for a server with a self-signed certificate on your own network.")
                checked: Muistio.ignoreSslErrors
                onCheckedChanged: Muistio.ignoreSslErrors = checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Sign out")
                onClicked: remorse.execute(qsTr("Signing out"), function() {
                    app.showToast(qsTr("This is a mock. Nothing was signed out."))
                })
            }
        }

        VerticalScrollDecorator {
        }
    }

    RemorsePopup {
        id: remorse
    }
}
