import Opal.LinkHandler 1.0 as L
import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    property int noteId
    property string title
    property string content
    property string category
    property bool favorite
    property bool locked
    property int modified

    function load() {
        var note = app.notes.noteById(page.noteId)
        if (!note)
            return
        applyNote(note)
    }

    function applyNote(note) {
        page.title = note.title
        page.content = note.content
        page.category = note.category
        page.favorite = note.favorite
        page.locked = note.locked
        page.modified = note.modified
    }

    function headerDescription() {
        var cat = page.category === "" ? qsTr("Uncategorized") : app.notes.categoryLabel(page.category)
        return cat + " \u00b7 " + app.notes.relativeTime(page.modified)
    }

    function toggleTask(lineIndex) {
        app.notes.toggleTask(page.noteId, lineIndex)
        page.load()
    }

    function remove() {
        remorse.execute(qsTr("Deleting"), function() {
            app.notes.removeNote(page.noteId)
            pageStack.pop()
        })
    }

    allowedOrientations: Orientation.All
    Component.onCompleted: page.load()
    onStatusChanged: {
        if (status === PageStatus.Active)
            page.load()
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
                text: page.favorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
                onClicked: {
                    app.notes.toggleFavorite(page.noteId)
                    page.load()
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
                title: page.title
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
}
