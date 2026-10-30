import QtQuick 2.0
import Sailfish.Silica 1.0
import "components"
import "notes"
import "pages"

ApplicationWindow {
    id: app

    property alias notes: store

    function showMain() {
        if (pageStack.busy) {
            navigateAgain.restart()
            return
        }
        pageStack.replaceAbove(null, Muistio.configured ? notesPage : loginPage)
    }

    function showToast(text) {
        if (text !== "")
            globalNotification.toast(text)
    }

    function syncNotes() {
        Muistio.sync()
    }

    function openNote(noteId) {
        if (!notes.noteById(noteId)) {
            showToast(qsTr("That note is no longer here."))
            return
        }
        pageStack.push(Qt.resolvedUrl("pages/NotePage.qml"), {
            "noteId": noteId
        })
    }

    function inheritCategory() {
        return notes.filterKind === "category" ? notes.filterCategory : ""
    }

    // Reachable from the cover, which can be tapped before anyone has signed in.
    function openNewNote() {
        if (!Muistio.configured)
            return
        activate()
        pageStack.push(Qt.resolvedUrl("pages/NoteEditPage.qml"), {
            "inheritCategory": inheritCategory()
        })
    }

    function togglePin(noteId) {
        if (notes.pinnedNoteId === noteId)
            notes.unpinNote()
        else
            notes.pinNote(noteId)
    }

    function editNote(noteId) {
        var note = notes.noteById(noteId)
        if (!note || note.locked)
            return
        pushEditor(note)
    }

    function pushEditor(note) {
        pageStack.push(Qt.resolvedUrl("pages/NoteEditPage.qml"), {
            "noteId": note.noteId,
            "content": note.content,
            "category": note.category,
            "favorite": note.favorite
        })
    }

    initialPage: Muistio.ready ? (Muistio.configured ? notesPage : loginPage) : startPage
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
        target: Muistio
        onReadyChanged: {
            if (Muistio.ready)
                app.showMain()
        }
        onConfiguredChanged: {
            if (!Muistio.configured)
                app.showMain()
        }
        onNotesChanged: app.notes.ingest(Notes.notes())
        onBusyChanged: app.notes.busy = Muistio.busy
        onErrorMessageChanged: app.showToast(Muistio.errorMessage)
    }

    MuistioNotification {
        id: globalNotification
    }
}
