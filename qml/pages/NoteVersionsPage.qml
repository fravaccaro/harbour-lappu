import "../notes/NoteFormat.js" as Format
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property int noteId
    // The history is read when the page opens and held no longer: it is the
    // server's record of the note, so there is nothing to show until it has
    // been asked.
    property var versions: []

    function openVersion(version) {
        pageStack.push(Qt.resolvedUrl("NoteVersionPage.qml"), {
            "noteId": page.noteId,
            "timestamp": version.timestamp,
            "text": version.text
        });
    }

    function versionDate(stamp) {
        return Qt.formatDateTime(new Date(stamp * 1000), Qt.DefaultLocaleShortDate);
    }

    allowedOrientations: Orientation.All
    Component.onCompleted: Lappu.loadVersions(page.noteId)

    Connections {
        target: Lappu
        onVersionsChanged: page.versions = Lappu.noteVersions()
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: page.versions

        PullDownMenu {
            busy: Lappu.versionsBusy

            MenuItem {
                text: qsTr("Refresh")
                onClicked: Lappu.loadVersions(page.noteId)
            }

        }

        header: Column {
            width: listView.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Previous versions")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: listView.count > 0
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Previous versions of this note saved on Nextcloud. Your current version isn't listed here.")
            }

            // Keeps the first version off the paragraph above it.
            Item {
                width: 1
                height: Theme.paddingMedium
                visible: listView.count > 0
            }

        }

        delegate: ListItem {
            id: item

            contentHeight: block.height + 2 * Theme.paddingMedium
            onClicked: page.openVersion(modelData)

            Column {
                id: block

                spacing: Theme.paddingSmall

                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: app.notes.relativeTime(modelData.timestamp)
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: page.versionDate(modelData.timestamp)
                }

                // The opening of the version, so two of them can be told
                // apart without opening either.
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    visible: text !== ""
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: Format.excerpt(modelData.text)
                }

            }

        }

        ViewPlaceholder {
            enabled: listView.count === 0 && !Lappu.versionsBusy
            text: qsTr("No previous versions")
            hintText: qsTr("A version is kept each time the note is changed on the server.")
        }

        VerticalScrollDecorator {
        }

    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: Lappu.versionsBusy && listView.count === 0
    }

}
