import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    // Reading lockedCount is what brings the list back, both when a refusal
    // adds a note and when a write that landed takes one away: the session
    // reports either through the one signal.
    property var held: Lappu.lockedCount > 0 ? Lappu.lockedNotes() : []
    property bool anyReadOnly: page.held.filter(function(note) {
        return note.reason === "readOnly";
    }).length > 0

    function discard(noteId) {
        remorse.execute(qsTr("Discarding"), function() {
            app.notes.discardLocal(noteId);
        });
    }

    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: page.held

        // The lock is lifted on the server, so the refresh that sends the
        // change belongs on the page the user is looking at when they lift it.
        PullDownMenu {

            MenuItem {
                text: qsTr("Refresh")
                onClicked: app.syncNotes()
            }

        }

        header: Column {
            width: listView.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Locked notes")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: listView.count > 0
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("These files are locked in Nextcloud and cannot sync. Open Nextcloud > Files > Notes, and choose Unlock for each file. Your changes are saved locally and will upload automatically as soon as they are unlocked.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.anyReadOnly
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("This note is read-only. Your changes are saved locally and will upload automatically once edit access is granted.")
            }

            // Keeps the first note off the paragraph above it.
            Item {
                width: 1
                height: Theme.paddingMedium
                visible: listView.count > 0
            }

        }

        delegate: ListItem {
            id: item

            // Read once here rather than from the menu, which is built apart
            // from the row and has no model of its own.
            property int noteId: modelData.noteId

            contentHeight: block.height + 2 * Theme.paddingMedium
            onClicked: app.openNote(item.noteId)

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
                    text: modelData.title
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: modelData.reason === "readOnly" ? qsTr("Read-only on the server") : qsTr("Locked on the server")
                }

            }

            // The way out when a lock is not going to lift: the note goes back
            // to the version the server holds and stops being held shut.
            menu: ContextMenu {

                MenuItem {
                    text: qsTr("Discard waiting change")
                    onClicked: page.discard(item.noteId)
                }

            }

        }

        // Nothing is kept between runs: a note the server is holding shut is
        // found again by the refusal the next refresh earns.
        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("Nothing is locked")
            hintText: qsTr("A file locked on Nextcloud will re-appear after the next refresh.")
        }

        VerticalScrollDecorator {
        }

    }

    // Page-level rather than in the row: the list is a plain array that is
    // made again whenever a refusal or a write changes it, so a remorse the
    // row owned would be thrown away with the row by a refresh that happened
    // to land mid-count, and the discard would quietly never happen.
    RemorsePopup {
        id: remorse
    }

}
