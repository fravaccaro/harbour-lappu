import "../components"
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: page

    property int noteId: 0
    property string title
    property string content
    property string category
    property bool favorite: false
    property string inheritCategory

    function pickCategory() {
        var dialog = pageStack.push(Qt.resolvedUrl("CategoryPickerPage.qml"), {
            "category": page.category
        });
        dialog.accepted.connect(function() {
            page.category = dialog.category;
        });
    }

    allowedOrientations: Orientation.All
    Component.onCompleted: {
        if (page.noteId === 0 && page.inheritCategory !== "")
            page.category = page.inheritCategory;

        editHistory.reset(bodyField.text, bodyField.cursorPosition);
    }
    canAccept: true
    onAccepted: app.notes.saveNote(page.noteId, titleField.text, bodyField.text, page.category, page.favorite)

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

            // The name of the note, which is the name of the file on the
            // server. It is not part of the text, and left empty it is taken
            // from the first line rather than refusing the note.
            TextField {
                id: titleField

                width: parent.width
                text: page.title
                label: qsTr("Title")
                placeholderText: qsTr("Named after the first line")
                // The star is part of the field, so tapping it must not take
                // the keyboard down in the middle of naming the note.
                focusOutBehavior: FocusBehavior.KeepFocus
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: bodyField.forceActiveFocus()

                // Silica sets a field's right item against the text line and
                // shortens the writing room to match, so the name never runs
                // under the star.
                rightItem: IconButton {
                    icon.source: page.favorite ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
                    onClicked: page.favorite = !page.favorite
                }

            }

            ValueButton {
                width: parent.width
                label: qsTr("Category")
                value: page.category === "" ? qsTr("Uncategorized") : app.notes.categoryLabel(page.category)
                onClicked: page.pickCategory()
            }

            MarkdownToolbar {
                width: parent.width
                target: bodyField
                history: editHistory
            }

            TextArea {
                id: bodyField

                width: parent.width
                // What is left of the page, so a short note is still written
                // on the whole of it and the field grows with a longer one.
                // The strip the arrows stand in is added to the writing, so
                // the text never runs under them.
                height: Math.max(implicitHeight + undoBar.height, page.height - y - Theme.paddingLarge)
                text: page.content
                placeholderText: qsTr("Write a note...")
                // A tap on the Markdown bar pulls focus off the field, and the
                // default behaviour clears it a moment later, taking the keyboard
                // down. Keeping focus lets the bar add a mark without the keyboard
                // slipping away; a deliberate swipe down still closes it.
                focusOutBehavior: FocusBehavior.KeepFocus
                wrapMode: TextEdit.Wrap
                // The field reaches the foot of the page, so the line Silica
                // draws under it would be a rule across the bottom.
                backgroundStyle: TextEditor.NoBackground
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
                            opacity: parent.enabled ? 1 : Theme.opacityLow
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
                            opacity: parent.enabled ? 1 : Theme.opacityLow
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
