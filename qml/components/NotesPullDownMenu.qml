/*!
    \qmltype NotesPullDownMenu
    \brief The pulley menu of the notes view, whichever way the notes are laid out.

    The menu knows nothing about the notes: it reports a tap through one signal
    each and takes the one piece of state its own labels need, \c searchVisible,
    from the caller. \c busy drives the pulley's own indicator.

    New note sits at the handle, nearest the thumb.

    \qml
    NotesPullDownMenu {
        busy: notes.busy
        searchVisible: page.searchVisible
        onSettingsClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
        onRefreshClicked: app.syncNotes()
        onSearchToggled: page.searchVisible = !page.searchVisible
        onNewNoteClicked: app.openNewNote()
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

PullDownMenu {
    id: root

    property bool searchVisible: false

    signal aboutClicked()
    signal settingsClicked()
    signal refreshClicked()
    signal searchToggled()
    signal newNoteClicked()

    MenuItem {
        text: qsTr("About")
        onClicked: root.aboutClicked()
    }

    MenuItem {
        text: qsTr("Settings")
        onClicked: root.settingsClicked()
    }

    MenuItem {
        text: qsTr("Refresh")
        onClicked: root.refreshClicked()
    }

    MenuItem {
        text: root.searchVisible ? qsTr("Hide search") : qsTr("Search")
        onClicked: root.searchToggled()
    }

    MenuItem {
        text: qsTr("New note")
        onClicked: root.newNoteClicked()
    }

}
