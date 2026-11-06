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
    // The keyboard takes half the screen, so the page gives up what it can
    // spare while it is open: its own name, the star and the category.
    readonly property bool writing: Qt.inputMethod.visible

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

        // Assigned once rather than bound: a bound Silica field on Qt 5.6 can
        // keep showing what was typed while its text still reads the opening
        // value, so a save would write the old title or body back.
        titleField.text = page.title;
        bodyField.text = page.content;
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
        contentHeight: column.height + Theme.paddingSmall

        Column {
            id: column

            width: page.width

            DialogHeader {
                acceptText: page.noteId === 0 ? qsTr("Add") : qsTr("Save")
                title: page.writing ? "" : (page.noteId === 0 ? qsTr("New note") : qsTr("Edit note"))

                // Silica fades the title out by itself once it says nothing.
                // The room it stood in is given back at the pace the keyboard
                // arrives, which is what the accept strip rides on.
                Behavior on height {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.InOutQuad
                    }

                }

            }

            // The name of the note, which is the name of the file on the
            // server. It is not part of the text, and left empty it is taken
            // from the first line rather than refusing the note.
            TextField {
                id: titleField

                width: parent.width
                label: qsTr("Title")
                placeholderText: qsTr("Add a title...")
                // The star is part of the field, so tapping it must not take
                // the keyboard down in the middle of naming the note.
                focusOutBehavior: FocusBehavior.KeepFocus
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: bodyField.forceActiveFocus()

                // The star leaves with the keyboard. Silica shortens the writing
                // room to the right item, and gives that room back the moment
                // the item has faded, so the slot closes with the fade and
                // the name never runs under it.
                rightItem: Item {
                    id: favoriteSlot

                    width: page.writing ? 0 : favoriteButton.width
                    height: favoriteButton.height
                    clip: true
                    opacity: page.writing ? 0 : 1

                    Behavior on width {
                        NumberAnimation {
                            duration: 200
                            easing.type: Easing.InOutQuad
                        }

                    }

                    Behavior on opacity {
                        FadeAnimation {
                        }

                    }

                    IconButton {
                        id: favoriteButton

                        icon.source: page.favorite ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
                        // Disabled would dim the icon at once, ahead of the fade.
                        onClicked: if (!page.writing) page.favorite = !page.favorite
                    }

                }

            }

            // The category stands in a slot of its own, so the button keeps
            // the height Silica gives it and the slot is what closes.
            Item {
                id: categorySlot

                width: parent.width
                height: page.writing ? 0 : categoryButton.height
                clip: true
                opacity: page.writing ? 0 : 1

                Behavior on height {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.InOutQuad
                    }

                }

                Behavior on opacity {
                    FadeAnimation {
                    }

                }

                ValueButton {
                    id: categoryButton

                    width: parent.width
                    // Nothing is to reach a stop on its way out.
                    enabled: !page.writing
                    label: qsTr("Category")
                    value: page.category === "" ? qsTr("Uncategorized") : app.notes.categoryLabel(page.category)
                    onClicked: page.pickCategory()
                }

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
                height: Math.max(implicitHeight, page.height - y - Theme.paddingMedium)
                placeholderText: qsTr("Write a note...")
                // A tap on the Markdown bar pulls focus off the field, and the
                // default behaviour clears it a moment later, taking the keyboard
                // down. Keeping focus lets the bar add a mark without the keyboard
                // slipping away; a deliberate swipe down still closes it.
                focusOutBehavior: FocusBehavior.KeepFocus
                wrapMode: TextEdit.Wrap
                // The body has no label, but Silica reserves a line for one
                // under every field and counts it into the margin below the
                // writing. Saying there is none is what shortens that margin.
                labelVisible: false
                // The field reaches the foot of the page, so the line Silica
                // draws under it would be a rule across the bottom.
                backgroundStyle: TextEditor.NoBackground
                color: Theme.primaryColor
                onTextChanged: editHistory.watch(text, cursorPosition)
            }

        }

        VerticalScrollDecorator {
        }

    }

}
