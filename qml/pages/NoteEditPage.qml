import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Dialog {
    id: page

    property int noteId: 0
    property string content
    property string category
    property bool favorite: false
    property string inheritCategory

    function pickCategory() {
        var dialog = pageStack.push(Qt.resolvedUrl("CategoryPickerPage.qml"), {
            "category": page.category
        })
        dialog.accepted.connect(function() {
            page.category = dialog.category
        })
    }

    allowedOrientations: Orientation.All
    Component.onCompleted: {
        if (page.noteId === 0 && page.inheritCategory !== "")
            page.category = page.inheritCategory
        editHistory.reset(bodyField.text, bodyField.cursorPosition)
    }
    canAccept: true
    onAccepted: app.notes.saveNote(page.noteId, bodyField.text, page.category,
                                   favoriteSwitch.checked)

    EditHistory {
        id: editHistory

        target: bodyField
    }

    SilicaFlickable {
        id: flick

        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: page.width

            DialogHeader {
                acceptText: page.noteId === 0 ? qsTr("Add") : qsTr("Save")
                title: page.noteId === 0 ? qsTr("New note") : qsTr("Edit note")
            }

            ValueButton {
                width: parent.width
                label: qsTr("Category")
                value: page.category === "" ? qsTr("Uncategorized") : app.notes.categoryLabel(page.category)
                onClicked: page.pickCategory()
            }

            TextSwitch {
                id: favoriteSwitch

                width: parent.width
                text: qsTr("Favorite")
                checked: page.favorite
            }

            MarkdownToolbar {
                width: parent.width
                target: bodyField
                history: editHistory
            }

            TextArea {
                id: bodyField

                width: parent.width
                // The strip the arrows stand in is added to the field, so the
                // writing never runs under them.
                height: Math.max(implicitHeight + undoBar.height, Screen.height / 3)
                text: page.content
                label: qsTr("The first line is the title")
                placeholderText: qsTr("Write a note...")
                wrapMode: TextEdit.Wrap
                color: Theme.primaryColor
                onTextChanged: editHistory.watch(text, cursorPosition)

                // Silica sends a TextArea's children into its content area,
                // which the field's own mouse area sits over. Parenting to the
                // field instead, as Silica does for its own slot items, leaves
                // the arrows on top where taps can reach them.
                Row {
                    id: undoBar

                    parent: bodyField
                    z: 1

                    // The foot of the content area, so the arrows stand inside
                    // the field and above the line that closes it.
                    anchors {
                        left: parent.left
                        leftMargin: bodyField.textLeftMargin
                        bottom: bodyField.contentItem.bottom
                    }

                    BackgroundItem {
                        width: Theme.itemSizeSmall
                        height: Theme.itemSizeSmall
                        enabled: editHistory.canUndo
                        onClicked: editHistory.undo()

                        Label {
                            anchors.centerIn: parent
                            font.pixelSize: Theme.fontSizeLarge
                            color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                            opacity: parent.enabled ? 1.0 : Theme.opacityLow
                            text: "\u21a9"
                        }
                    }

                    BackgroundItem {
                        width: Theme.itemSizeSmall
                        height: Theme.itemSizeSmall
                        enabled: editHistory.canRedo
                        onClicked: editHistory.redo()

                        Label {
                            anchors.centerIn: parent
                            font.pixelSize: Theme.fontSizeLarge
                            color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                            opacity: parent.enabled ? 1.0 : Theme.opacityLow
                            text: "\u21aa"
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {
        }
    }
}
