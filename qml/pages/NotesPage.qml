import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    property bool searchVisible: false

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
                text: page.searchVisible ? qsTr("Hide search") : qsTr("Search")
                onClicked: page.searchVisible = !page.searchVisible
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
                id: searchField

                width: parent.width
                visible: page.searchVisible
                placeholderText: qsTr("Search notes")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onTextChanged: app.notes.searchText = text
            }

            // Showing the field is a request to type in it, and hiding it is
            // how a search is called off. The header is a component of its
            // own, so what reaches for the field has to live in here with it.
            Connections {
                target: page
                onSearchVisibleChanged: {
                    if (page.searchVisible) {
                        searchField.forceActiveFocus()
                        return
                    }
                    searchField.text = ""
                    searchField.focus = false
                }
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
