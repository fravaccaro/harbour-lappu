import QtQuick 2.0
import Sailfish.Silica 1.0
import "components"
import "mock"
import "pages"

ApplicationWindow {
    id: app

    property alias notes: store

    function showToast(text) {
        if (text !== "")
            globalNotification.toast(text)
    }

    function mockRefresh() {
        notes.busy = true
        mockBusyTimer.restart()
        showToast(qsTr("This is a mock. Nothing was synced."))
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

    function openNewNote() {
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
            "title": note.title,
            "content": note.content,
            "category": note.category,
            "favorite": note.favorite
        })
    }

    initialPage: notesPage
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    NotesStore {
        id: store

        Component.onCompleted: seed()
    }

    Timer {
        id: mockBusyTimer

        interval: 700
        onTriggered: notes.busy = false
    }

    Component {
        id: notesPage

        NotesPage {
        }
    }

    MuistioNotification {
        id: globalNotification
    }
}
