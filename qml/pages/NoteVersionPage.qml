import "../components"
import Opal.LinkHandler 1.0 as L
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property int noteId
    // When the server took this version, and the whole note as it read then.
    // Both came with the listing, so nothing is asked of the server here.
    property int timestamp
    property string text

    function restore() {
        remorse.execute(qsTr("Restoring"), page.putBack);
    }

    // An ordinary edit: the text goes back the way anything else written
    // here does, and the note as it stands becomes a version of its own. The
    // name, the category and the star are the note's and are handed back
    // untouched - nothing renames a note but the writer.
    function putBack() {
        var note = app.notes.noteById(page.noteId);
        if (!note)
            return ;

        app.notes.saveNote(page.noteId, note.title, page.text, note.category, note.favorite);
        page.leaveForNote();
    }

    // Back to the note itself rather than to the list of versions: the note
    // is what has just changed, and the list no longer says what it said.
    function leaveForNote() {
        var notePage = pageStack.find(function(item) {
            return item.objectName === "notePage";
        });
        if (notePage)
            pageStack.pop(notePage);
        else
            pageStack.pop();
    }

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {

            MenuItem {
                text: qsTr("Restore this version")
                onClicked: page.restore()
            }

        }

        Column {
            id: column

            width: page.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Earlier version")
                description: app.notes.relativeTime(page.timestamp)
            }

            // The same drawing a conflict's two sides are given: the note as
            // it read then, with its tasks left untappable.
            ConflictVersion {
                width: parent.width
                label: qsTr("As it read then")
                content: page.text
                modified: page.timestamp
                // A link in an old version came from the server exactly as
                // one in the note does, so it is offered rather than opened.
                onLinkActivated: L.LinkHandler.openOrCopyUrl(link)
            }

        }

        VerticalScrollDecorator {
        }

    }

    RemorsePopup {
        id: remorse
    }

}
