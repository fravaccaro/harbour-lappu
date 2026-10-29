import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

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
        })
        dialog.accepted.connect(function() {
            page.category = dialog.category
        })
    }

    allowedOrientations: Orientation.All
    Component.onCompleted: {
        if (page.noteId === 0 && page.inheritCategory !== "")
            page.category = page.inheritCategory
    }
    canAccept: true
    onAccepted: app.notes.saveNote(page.noteId, titleField.text, bodyField.text,
                                   page.category, favoriteSwitch.checked)

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

            TextField {
                id: titleField

                width: parent.width
                text: page.title
                label: qsTr("Title")
                placeholderText: qsTr("New note")
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: bodyField.forceActiveFocus()
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
            }

            TextArea {
                id: bodyField

                width: parent.width
                height: Math.max(implicitHeight, Screen.height / 3)
                text: page.content
                placeholderText: qsTr("Write a note...")
                wrapMode: TextEdit.Wrap
                color: Theme.primaryColor
            }
        }

        VerticalScrollDecorator {
        }
    }
}
