import Opal.SupportMe 1.0
import QtQuick 2.0
import Sailfish.Silica 1.0
import "components"
import "notes"
import "pages"

ApplicationWindow {
    id: app

    property alias notes: store
    // Text shared from another app that has nowhere to go yet.
    property string pendingShare

    function showMain() {
        if (pageStack.busy) {
            navigateAgain.restart();
            return ;
        }
        pageStack.replaceAbove(null, Lappu.configured ? notesPage : loginPage);
    }

    function showToast(text) {
        if (text !== "")
            globalNotification.toast(text);

    }

    function syncNotes() {
        Lappu.sync();
    }

    function openNote(noteId) {
        if (!notes.noteById(noteId)) {
            showToast(qsTr("That note is no longer here."));
            return ;
        }
        pageStack.push(Qt.resolvedUrl("pages/NotePage.qml"), {
            "noteId": noteId
        });
    }

    function inheritCategory() {
        return notes.filterKind === "category" ? notes.filterCategory : "";
    }

    // Reachable from the cover, which can be tapped before anyone has signed in.
    function openNewNote() {
        if (!Lappu.configured)
            return ;

        activate();
        pageStack.push(Qt.resolvedUrl("pages/NoteEditPage.qml"), {
            "inheritCategory": inheritCategory()
        });
    }

    // A share arrives from outside, so it may find the app signed out, or on a
    // cold start still deciding what page it opens on. It waits here instead
    // of being dropped, and the latest one is the one that opens.
    function openSharedNote(body) {
        if (body === "") {
            showToast(qsTr("There was nothing in that to write down."));
            return ;
        }
        pendingShare = body;
        activate();
        writeShare();
    }

    // Held back until the app knows where it is going: until the settings have
    // been read there is no telling whether there is an account to write to,
    // and a dialog pushed over the start page is thrown away by the navigation
    // that follows it.
    function writeShare() {
        if (pendingShare === "" || !Lappu.ready)
            return ;

        if (!Lappu.configured) {
            showToast(qsTr("Sign in, and what you shared opens as a new note."));
            return ;
        }
        if (pageStack.busy) {
            shareAgain.restart();
            return ;
        }
        pushSharedNote();
    }

    // The share is let go of as the editor takes it, so a note saved and a
    // note abandoned both leave nothing behind to open a second time.
    function pushSharedNote() {
        var body = pendingShare;
        pendingShare = "";
        pageStack.push(Qt.resolvedUrl("pages/NoteEditPage.qml"), {
            "content": body,
            "inheritCategory": inheritCategory()
        });
    }

    function togglePin(noteId) {
        if (notes.pinnedNoteId === noteId)
            notes.unpinNote();
        else
            notes.pinNote(noteId);
    }

    function editNote(noteId) {
        var note = notes.noteById(noteId);
        if (!note || note.readOnly)
            return ;

        pushEditor(note);
    }

    function pushEditor(note) {
        pageStack.push(Qt.resolvedUrl("pages/NoteEditPage.qml"), {
            "noteId": note.noteId,
            "title": note.title,
            "content": note.content,
            "category": note.category,
            "favorite": note.favorite
        });
    }

    initialPage: Lappu.ready ? (Lappu.configured ? notesPage : loginPage) : startPage
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    NotesStore {
        id: store

        // The session fills its model before the window loads, so the notes
        // from the last pull are already there to take.
        Component.onCompleted: ingest(Notes.notes())
    }

    Timer {
        id: navigateAgain

        interval: 100
        onTriggered: app.showMain()
    }

    Timer {
        id: shareAgain

        interval: 100
        onTriggered: app.writeShare()
    }

    Component {
        id: startPage

        StartPage {
        }

    }

    Component {
        id: loginPage

        LoginPage {
        }

    }

    Component {
        id: notesPage

        NotesPage {
        }

    }

    Connections {
        target: Lappu
        onReadyChanged: {
            if (!Lappu.ready)
                return ;

            app.showMain();
            app.writeShare();
        }
        onConfiguredChanged: {
            if (Lappu.configured)
                app.writeShare();
            else
                app.showMain();
        }
        onNotesChanged: app.notes.ingest(Notes.notes())
        onBusyChanged: app.notes.busy = Lappu.busy
        onErrorMessageChanged: app.showToast(Lappu.errorMessage)
        onNotice: app.showToast(text)
    }

    LappuNotification {
        id: globalNotification
    }

    // Text and links shared from other apps. They are shaped into a note body
    // by the session and opened in the editor, never saved on their own: what
    // another app sent is the writer's to look at before it is kept.
    ShareReceiver {
        onTextShared: app.openSharedNote(Lappu.sharedBody(text))
        onLinkShared: app.openSharedNote(Lappu.sharedLink(url, linkTitle))
    }

    // Asks for support once the app has been used for a while, and not again
    // for a long time after the answer.
    AskForSupport {

        contents: Component {
            LappuSupportDialog {
            }

        }

    }

}
