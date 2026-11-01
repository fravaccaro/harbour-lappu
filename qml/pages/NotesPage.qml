import "../components"
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property bool searchVisible: false

    allowedOrientations: Orientation.All
    onStatusChanged: {
        if (status === PageStatus.Active && pageStack.nextPage() === null)
            pageStack.pushAttached(Qt.resolvedUrl("CategoriesPage.qml"));

    }

    // The two layouts are different kinds of view rather than one view drawn
    // two ways, so only the chosen one is built. Changing the setting starts
    // the notes again from the top, which is where a view just asked for
    // belongs anyway.
    Loader {
        anchors.fill: parent
        sourceComponent: Lappu.viewMode === "grid" ? gridComponent : listComponent
    }

    Component {
        id: listComponent

        SilicaListView {
            id: listView

            anchors.fill: parent
            model: app.notes.visibleNotes
            currentIndex: -1

            NotesPullDownMenu {
                busy: app.notes.busy
                searchVisible: page.searchVisible
                onAboutClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
                onSettingsClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
                onRefreshClicked: app.syncNotes()
                onSearchToggled: page.searchVisible = !page.searchVisible
                onNewNoteClicked: app.openNewNote()
            }

            ViewPlaceholder {
                enabled: app.notes.visibleNotes.count === 0 && !app.notes.busy
                text: qsTr("No notes")
                hintText: app.notes.searchText !== "" ? qsTr("No matches") : qsTr("Pull down to add a note")
            }

            VerticalScrollDecorator {
            }

            header: NotesListHeader {
                width: listView.width
                title: app.notes.listTitle
                conflictCount: Lappu.conflictCount
                searchVisible: page.searchVisible
                searchText: app.notes.searchText
                onConflictClicked: pageStack.push(Qt.resolvedUrl("ConflictPage.qml"))
                onSearchRequested: app.notes.searchText = text
            }

            delegate: ListItem {
                id: item

                function remove() {
                    remorseAction(qsTr("Deleting"), function() {
                        app.notes.removeNote(noteId);
                    });
                }

                contentHeight: block.height + 2 * Theme.paddingMedium
                onClicked: app.openNote(noteId)

                Column {
                    id: block

                    spacing: Theme.paddingSmall

                    anchors {
                        left: parent.left
                        right: parent.right
                        leftMargin: Theme.horizontalPageMargin
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }

                    Item {
                        width: parent.width
                        height: titleLabel.height

                        Label {
                            id: titleLabel

                            truncationMode: TruncationMode.Fade
                            color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                            text: title + (favorite ? " \u2605" : "")

                            anchors {
                                left: parent.left
                                right: timeLabel.left
                                rightMargin: Theme.paddingMedium
                            }

                        }

                        Label {
                            id: timeLabel

                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            text: app.notes.relativeTime(modified)

                            anchors {
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }

                        }

                    }

                    Label {
                        width: parent.width
                        visible: excerpt !== ""
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeSmall
                        color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        text: excerpt
                    }

                    Label {
                        width: parent.width
                        visible: app.notes.filterKind === "all" && categoryLabel !== ""
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryHighlightColor
                        text: categoryLabel
                    }

                }

                menu: NoteContextMenu {
                    favorite: model.favorite
                    locked: model.locked
                    pinned: app.notes.pinnedNoteId === model.noteId
                    onFavoriteToggled: app.notes.toggleFavorite(model.noteId)
                    onPinToggled: app.togglePin(model.noteId)
                    onEditClicked: app.editNote(model.noteId)
                    onDeleteClicked: item.remove()
                }

            }

        }

    }

    Component {
        id: gridComponent

        SilicaGridView {
            id: gridView

            // Two columns across the narrow side of the screen, so turning the
            // phone gives more of them rather than wider ones.
            readonly property int columns: Math.max(1, Math.round(width / (Screen.width / 2)))

            anchors.fill: parent
            model: app.notes.visibleNotes
            currentIndex: -1
            cellWidth: Math.floor(width / columns)
            // Shorter than it is wide, or two columns of near-square cards
            // would fit fewer notes on the screen than the rows they replaced,
            // which is the one thing a grid is asked for. What is left over
            // after the title, the time and the category goes to the body.
            cellHeight: Math.round(cellWidth * 0.85)

            NotesPullDownMenu {
                busy: app.notes.busy
                searchVisible: page.searchVisible
                onAboutClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
                onSettingsClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
                onRefreshClicked: app.syncNotes()
                onSearchToggled: page.searchVisible = !page.searchVisible
                onNewNoteClicked: app.openNewNote()
            }

            ViewPlaceholder {
                enabled: app.notes.visibleNotes.count === 0 && !app.notes.busy
                text: qsTr("No notes")
                hintText: app.notes.searchText !== "" ? qsTr("No matches") : qsTr("Pull down to add a note")
            }

            VerticalScrollDecorator {
            }

            header: NotesListHeader {
                width: gridView.width
                title: app.notes.listTitle
                conflictCount: Lappu.conflictCount
                searchVisible: page.searchVisible
                searchText: app.notes.searchText
                onConflictClicked: pageStack.push(Qt.resolvedUrl("ConflictPage.qml"))
                onSearchRequested: app.notes.searchText = text
            }

            delegate: GridItem {
                id: cell

                function remove() {
                    remorseAction(qsTr("Deleting"), function() {
                        app.notes.removeNote(noteId);
                    });
                }

                onClicked: app.openNote(noteId)

                // A cell is too narrow to put the time beside the title, so the
                // same four things the list row carries are stacked instead:
                // the name at the top, the category at the foot, and the body
                // given whatever is left between them.
                Item {
                    id: card

                    anchors {
                        fill: parent
                        margins: Theme.paddingMedium
                    }

                    Column {
                        id: cardTop

                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                        }

                        Label {
                            width: parent.width
                            truncationMode: TruncationMode.Fade
                            color: cell.highlighted ? Theme.highlightColor : Theme.primaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            text: title + (favorite ? " \u2605" : "")
                        }

                        Label {
                            width: parent.width
                            truncationMode: TruncationMode.Fade
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            text: app.notes.relativeTime(modified)
                        }

                    }

                    Label {
                        id: cardCategory

                        visible: app.notes.filterKind === "all" && categoryLabel !== ""
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryHighlightColor
                        text: categoryLabel

                        anchors {
                            left: parent.left
                            right: parent.right
                            bottom: parent.bottom
                        }

                    }

                    Label {
                        visible: excerpt !== ""
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: cell.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        text: excerpt

                        anchors {
                            left: parent.left
                            right: parent.right
                            top: cardTop.bottom
                            topMargin: Theme.paddingSmall
                            bottom: cardCategory.visible ? cardCategory.top : parent.bottom
                        }

                    }

                }

                menu: NoteContextMenu {
                    favorite: model.favorite
                    locked: model.locked
                    pinned: app.notes.pinnedNoteId === model.noteId
                    onFavoriteToggled: app.notes.toggleFavorite(model.noteId)
                    onPinToggled: app.togglePin(model.noteId)
                    onEditClicked: app.editNote(model.noteId)
                    onDeleteClicked: cell.remove()
                }

            }

        }

    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: app.notes.busy && app.notes.visibleNotes.count === 0
    }

}
