import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    allowedOrientations: Orientation.All
    onStatusChanged: {
        if (status === PageStatus.Active && pageStack.nextPage() === null)
            pageStack.pushAttached(Qt.resolvedUrl("CategoriesPage.qml"))
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: app.notes.visibleNotes
        currentIndex: -1

        PullDownMenu {
            busy: app.notes.busy

            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }

            MenuItem {
                text: qsTr("Refresh")
                onClicked: app.syncNotes()
            }

            MenuItem {
                text: qsTr("New note")
                onClicked: app.openNewNote()
            }
        }

        header: Column {
            width: listView.width

            PageHeader {
                title: app.notes.listTitle
            }

            SearchField {
                width: parent.width
                placeholderText: qsTr("Search notes")
                onTextChanged: app.notes.searchText = text
            }
        }

        delegate: ListItem {
            id: item

            contentHeight: block.height + 2 * Theme.paddingMedium
            onClicked: app.openNote(noteId)

            menu: ContextMenu {
                MenuItem {
                    text: favorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
                    onClicked: app.notes.toggleFavorite(noteId)
                }

                MenuItem {
                    text: app.notes.pinnedNoteId === noteId ? qsTr("Unpin from cover") : qsTr("Pin to cover")
                    onClicked: app.togglePin(noteId)
                }

                MenuItem {
                    enabled: !locked
                    text: qsTr("Edit")
                    onClicked: app.editNote(noteId)
                }

                MenuItem {
                    text: qsTr("Delete")
                    onClicked: item.remove()
                }
            }

            function remove() {
                remorseAction(qsTr("Deleting"), function() {
                    app.notes.removeNote(noteId)
                })
            }

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
        }

        ViewPlaceholder {
            enabled: listView.count === 0 && !app.notes.busy
            text: qsTr("No notes")
            hintText: app.notes.searchText !== "" ? qsTr("No matches") : qsTr("Pull down to add a note")
        }

        VerticalScrollDecorator {
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: app.notes.busy && listView.count === 0
    }
}
