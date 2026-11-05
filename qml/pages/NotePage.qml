import "../components"
import Opal.LinkHandler 1.0 as L
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property int noteId
    property string title
    property string content
    property string category
    property bool favorite
    property bool locked
    // Carries an edit the server has not taken, which is the only state there
    // is anything to discard from.
    property bool dirty
    property int modified

    function load() {
        var note = app.notes.noteById(page.noteId);
        if (!note)
            return ;

        applyNote(note);
    }

    function applyNote(note) {
        page.title = note.title;
        page.content = note.content;
        page.category = note.category;
        page.favorite = note.favorite;
        page.locked = note.locked;
        page.dirty = note.dirty;
        page.modified = note.modified;
    }

    function headerDescription() {
        var cat = page.category === "" ? qsTr("Uncategorized") : app.notes.categoryLabel(page.category);
        return cat + " \u00b7 " + app.notes.relativeTime(page.modified);
    }

    function toggleTask(lineIndex) {
        app.notes.toggleTask(page.noteId, lineIndex);
        page.load();
    }

    function remove() {
        remorse.execute(qsTr("Deleting"), function() {
            app.notes.removeNote(page.noteId);
            pageStack.pop();
        });
    }

    // The page stays open: the note is not going anywhere, it is only going
    // back to the version the server holds, which the pull brings in.
    function discard() {
        remorse.execute(qsTr("Discarding"), function() {
            app.notes.discardLocal(page.noteId);
        });
    }

    allowedOrientations: Orientation.All
    Component.onCompleted: page.load()
    onStatusChanged: {
        if (status === PageStatus.Active)
            page.load();

    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Delete")
                onClicked: page.remove()
            }

            MenuItem {
                visible: page.dirty
                text: qsTr("Discard waiting change")
                onClicked: page.discard()
            }

            MenuItem {
                text: page.favorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
                onClicked: {
                    app.notes.toggleFavorite(page.noteId);
                    page.load();
                }
            }

            MenuItem {
                text: app.notes.pinnedNoteId === page.noteId ? qsTr("Unpin from cover") : qsTr("Pin to cover")
                onClicked: app.togglePin(page.noteId)
            }

            MenuItem {
                visible: !page.locked
                text: qsTr("Edit")
                onClicked: app.editNote(page.noteId)
            }

        }

        Column {
            id: column

            width: page.width
            spacing: Theme.paddingMedium

            PageHeader {
                // The star stands after the name here as it does in the list,
                // so a favourite says so while it is being read too.
                title: app.notes.starredTitle(page.title, page.favorite)
                description: page.headerDescription()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.locked
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("This note is read-only.")
            }

            // A change the server has not taken is said on the note itself and
            // not only in its menu. A note that looks ordinary while carrying
            // one is what made a refused edit feel like a fault here rather
            // than an answer from the server. Nothing is said while a run is
            // going out, or every ordinary save would show this on its way
            // through.
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.dirty && !app.notes.busy
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("A change to this note has not reached the server yet.")
            }

            NoteBody {
                width: parent.width
                visible: page.content !== ""
                content: page.content
                locked: page.locked
                onTaskClicked: page.toggleTask(lineIndex)
                // A link in a note came from the server, so it is offered
                // rather than opened, the way every other link in the app is.
                // No title is passed: the markdown label is not carried by
                // onLinkActivated, and the note's own title would be a lie.
                onLinkActivated: L.LinkHandler.openOrCopyUrl(link)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.content === ""
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("This note is empty.")
            }

        }

        VerticalScrollDecorator {
        }

    }

    RemorsePopup {
        id: remorse
    }

    // A note is read while the server is still being talked to, so a version
    // that arrives while the page is open is drawn where it stands: a pull
    // putting back what a refused edit was discarded from is the whole point
    // of discarding it, and waiting for the page to be left and opened again
    // would leave the note showing a change that is no longer there.
    Connections {
        target: Lappu
        onNotesChanged: page.load()
    }

}
