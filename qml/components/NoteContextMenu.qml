/*!
    \qmltype NoteContextMenu
    \brief What can be done with one note, held open behind it.

    The menu reads no model of its own: the owner says what the note currently
    is through \c favorite, \c locked, \c pinned and \c dirty, which is all the
    labels, the one disabled entry and the one hidden entry need, and answers
    the five signals.

    Deleting and discarding are only reported, never done, so the row that owns
    the note keeps its remorse timer rather than the menu growing one.

    \qml
    menu: NoteContextMenu {
        favorite: model.favorite
        locked: model.locked
        pinned: notes.pinnedNoteId === model.noteId
        dirty: model.dirty
        onFavoriteToggled: notes.toggleFavorite(model.noteId)
        onPinToggled: app.togglePin(model.noteId)
        onEditClicked: app.editNote(model.noteId)
        onDeleteClicked: item.remove()
        onDiscardClicked: item.discard()
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

ContextMenu {
    id: root

    property bool favorite: false
    property bool locked: false
    property bool pinned: false
    // Carries an edit the server has not taken, which is the only state there
    // is anything to discard from.
    property bool dirty: false

    signal favoriteToggled()
    signal pinToggled()
    signal editClicked()
    signal deleteClicked()
    signal discardClicked()

    MenuItem {
        text: root.favorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
        onClicked: root.favoriteToggled()
    }

    MenuItem {
        text: root.pinned ? qsTr("Unpin from cover") : qsTr("Pin to cover")
        onClicked: root.pinToggled()
    }

    MenuItem {
        enabled: !root.locked
        text: qsTr("Edit")
        onClicked: root.editClicked()
    }

    MenuItem {
        visible: root.dirty
        text: qsTr("Discard waiting change")
        onClicked: root.discardClicked()
    }

    MenuItem {
        text: qsTr("Delete")
        onClicked: root.deleteClicked()
    }

}
