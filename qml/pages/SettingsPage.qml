import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    readonly property var intervals: [0, 5, 15, 30, 60]

    function intervalIndex(minutes) {
        var index = page.intervals.indexOf(minutes)
        return index < 0 ? 0 : index
    }

    function intervalLabel(minutes) {
        return minutes === 0 ? qsTr("Off") : qsTr("Every %n minute(s)", "", minutes)
    }

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
                    currentIndex = Lappu.sortMode === "title" ? 1 : 0
                    ready = true
                }
                onCurrentIndexChanged: {
                    if (ready && currentIndex >= 0)
                        Lappu.sortMode = currentIndex === 1 ? "title" : "modified"
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
                text: qsTr("Synchronisation")
            }

            TextSwitch {
                width: parent.width
                text: qsTr("Refresh when the app starts")
                checked: Lappu.syncOnStart
                onCheckedChanged: Lappu.syncOnStart = checked
            }

            TextSwitch {
                width: parent.width
                text: qsTr("Refresh after saving a note")
                description: qsTr("Fetches server changes once the note has been sent.")
                checked: Lappu.syncAfterPush
                onCheckedChanged: Lappu.syncAfterPush = checked
            }

            ComboBox {
                // Guards the stored value against the index changes that happen
                // while the menu is still being built.
                property bool ready: false

                width: parent.width
                label: qsTr("Automatic refresh")
                description: qsTr("Only while Lappu is running.")
                Component.onCompleted: {
                    currentIndex = page.intervalIndex(Lappu.syncInterval)
                    ready = true
                }
                onCurrentIndexChanged: {
                    if (ready && currentIndex >= 0)
                        Lappu.syncInterval = page.intervals[currentIndex]
                }

                menu: ContextMenu {
                    Repeater {
                        model: page.intervals

                        MenuItem {
                            text: page.intervalLabel(modelData)
                        }
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
                text: Lappu.serverUrl !== "" ? Lappu.serverUrl : qsTr("Not signed in")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: Lappu.username !== ""
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("Signed in as %1").arg(Lappu.username)
            }

            TextSwitch {
                width: parent.width
                text: qsTr("Ignore certificate errors")
                description: qsTr("Only for a server with a self-signed certificate on your own network.")
                checked: Lappu.ignoreSslErrors
                onCheckedChanged: Lappu.ignoreSslErrors = checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Sign out")
                onClicked: remorse.execute(qsTr("Signing out"), function() {
                    Lappu.logout()
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
