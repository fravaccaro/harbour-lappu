/*!
    \qmltype NoteContextMenu
    \brief What can be done with one note, held open behind it.

    The menu reads no model of its own: the owner says what the note currently
    is through \c favorite, \c locked and \c pinned, which is all the labels and
    the one disabled entry need, and answers the four signals.

    Deleting is only reported, never done, so the row that owns the note keeps
    its remorse timer rather than the menu growing one.

    \qml
    menu: NoteContextMenu {
        favorite: model.favorite
        locked: model.locked
        pinned: notes.pinnedNoteId === model.noteId
        onFavoriteToggled: notes.toggleFavorite(model.noteId)
        onPinToggled: app.togglePin(model.noteId)
        onEditClicked: app.editNote(model.noteId)
        onDeleteClicked: item.remove()
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

    signal favoriteToggled()
    signal pinToggled()
    signal editClicked()
    signal deleteClicked()

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
        text: qsTr("Delete")
        onClicked: root.deleteClicked()
    }

}
