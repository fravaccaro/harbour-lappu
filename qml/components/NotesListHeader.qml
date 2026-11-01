/*!
    \qmltype NotesListHeader
    \brief The header of the notes view: its name, anything left unsettled, and the search field.

    Stands above the notes whichever way they are laid out, so the list and the
    grid share one of these rather than a copy each. It holds no model: the
    caller passes \c title, \c conflictCount and \c searchText and answers the
    signals.

    The conflict row only appears while \c conflictCount is above zero. The
    search field follows \c searchVisible, and the field looks after its own
    focus: showing it is a request to type in it, and hiding it calls the
    search off, which empties the field and so reports an empty \c searchRequested.
    \c searchText is what the field starts from when the header is built with
    the search already open, which is the case when the layout itself is rebuilt.

    \qml
    NotesListHeader {
        width: parent.width
        title: notes.listTitle
        conflictCount: Lappu.conflictCount
        searchVisible: page.searchVisible
        searchText: notes.searchText
        onConflictClicked: pageStack.push(Qt.resolvedUrl("ConflictPage.qml"))
        onSearchRequested: notes.searchText = text
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    id: root

    property string title
    property int conflictCount: 0
    property bool searchVisible: false
    property string searchText: ""

    signal conflictClicked()
    signal searchRequested(string text)

    function applySearchVisible() {
        if (root.searchVisible) {
            if (searchField.text !== root.searchText)
                searchField.text = root.searchText;

            searchField.forceActiveFocus();
            return ;
        }
        searchField.text = "";
        searchField.focus = false;
    }

    onSearchVisibleChanged: root.applySearchVisible()
    Component.onCompleted: root.applySearchVisible()

    PageHeader {
        title: root.title
    }

    // The conflict page is never pushed on its own: a page arriving over the
    // user's work during a refresh would be an ambush. The row waits here
    // instead, for as long as anything is unsettled.
    BackgroundItem {
        id: conflictRow

        width: parent.width
        visible: root.conflictCount > 0
        onClicked: root.conflictClicked()

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            anchors.verticalCenter: parent.verticalCenter
            truncationMode: TruncationMode.Fade
            color: conflictRow.highlighted ? Theme.highlightColor : Theme.errorColor
            font.pixelSize: Theme.fontSizeSmall
            text: qsTr("%n note(s) changed on the server as well", "", root.conflictCount)
        }

    }

    SearchField {
        id: searchField

        width: parent.width
        visible: root.searchVisible
        placeholderText: qsTr("Search notes")
        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
        EnterKey.iconSource: "image://theme/icon-m-enter-close"
        EnterKey.onClicked: focus = false
        onTextChanged: root.searchRequested(text)
    }

}
