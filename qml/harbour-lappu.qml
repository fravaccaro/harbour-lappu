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
    // A New note asked for from the cover before the app was ready to open one.
    property bool pendingNewNote: false

    function showMain() {
        if (pageStack.busy) {
            navigateAgain.restart();
            return ;
        }
        pageStack.replaceAbove(null, Lappu.configured ? notesPage : loginPage);
        // A share or a cover New note may have been waiting on the start
        // page: try once the stack has been pointed at the notes or the
        // sign-in. Both still wait while the replace is busy, so a dialog
        // is not pushed under a wipe.
        flushPending();
    }

    function flushPending() {
        writeShare();
        writeNewNote();
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

    // Reachable from the cover, which can be tapped before the settings have
    // been read and before anyone has signed in. It waits for the notes the
    // way a share does, rather than being dropped on a cold start.
    function openNewNote() {
        pendingNewNote = true;
        activate();
        writeNewNote();
    }

    function writeNewNote() {
        if (!pendingNewNote)
            return ;

        if (!Lappu.ready) {
            pendingAgain.restart();
            return ;
        }
        // The cover offers New note before an account exists; nothing to say,
        // and nothing to keep waiting for once we know there is no account.
        if (!Lappu.configured) {
            pendingNewNote = false;
            return ;
        }
        if (pageStack.busy || shareBlocked()) {
            pendingAgain.restart();
            return ;
        }
        pendingNewNote = false;
        // An editor left open from an earlier tap is raised by activate()
        // rather than buried under a second, empty copy of itself. A share is
        // never held back this way: skipping its push would drop the body.
        if (pageStack.find(function(item) {
            return item.objectName === "noteEditPage";
        }))
            return ;

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

    // Pages whose next navigation would throw a dialog away: the start page
    // on a cold start, and the sign-in pages before the notes replace them.
    function shareBlocked() {
        var page = pageStack.currentPage;
        if (!page)
            return true;

        var name = page.objectName;
        return name === "startPage" || name === "loginPage" || name === "webLoginPage";
    }

    // Held back until the app knows where it is going: until the settings have
    // been read there is no telling whether there is an account to write to,
    // and a dialog pushed over the start or sign-in pages is thrown away by
    // the navigation that follows them. pendingShare is kept until the push
    // lands, so a wipe cannot drop the only copy of the body.
    function writeShare() {
        if (pendingShare === "" || !Lappu.ready)
            return ;

        if (!Lappu.configured) {
            showToast(qsTr("Sign in, and what you shared opens as a new note."));
            return ;
        }
        if (pageStack.busy || shareBlocked()) {
            pendingAgain.restart();
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
        id: pendingAgain

        interval: 100
        onTriggered: app.flushPending()
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
            if (Lappu.ready)
                app.showMain();

        }
        onConfiguredChanged: {
            if (Lappu.configured)
                app.flushPending();
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
