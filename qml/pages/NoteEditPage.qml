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
                title: page.noteId === 0 ? qsTr("New note") : qsTr("Note")
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

            Row {
                anchors.horizontalCenter: parent.horizontalCenter

                IconButton {
                    enabled: editHistory.canUndo
                    icon.source: "image://theme/icon-m-back"
                    onClicked: editHistory.undo()
                }

                IconButton {
                    enabled: editHistory.canRedo
                    icon.source: "image://theme/icon-m-forward"
                    onClicked: editHistory.redo()
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
                height: Math.max(implicitHeight, Screen.height / 3)
                text: page.content
                label: qsTr("The first line is the title")
                placeholderText: qsTr("Write a note...")
                wrapMode: TextEdit.Wrap
                color: Theme.primaryColor
                onTextChanged: editHistory.watch(text, cursorPosition)
            }
        }

        VerticalScrollDecorator {
        }
    }
}
