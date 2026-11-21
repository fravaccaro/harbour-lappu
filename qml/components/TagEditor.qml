/*!
    \qmltype TagEditor
    \brief Edits the tags of a note as removable chips.

    The component owns no model: the caller hands in \c tags as an array of
    tag paths and reads the same property back when saving. \c suggestions
    names the tags the note folder already holds; the ones this note does not
    carry yet are offered below the field and are added by a tap.

    A tag is kept once whatever its case, and blank input is ignored. A name
    typed with slashes in it is one tag under another, the way the server
    nests them, and is drawn with the separator spelled out.

    \c editing says a tag is being typed, so a page that folds its fields away
    with the keyboard can leave this one open.

    \qml
    TagEditor {
        id: tagEditor

        tags: ["Work/Projects"]
        suggestions: ["Work/Projects", "Home"]
    }
    \endqml
*/
import "../notes/NoteFormat.js" as Format
import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    id: root

    property var tags: []
    property var suggestions: []
    // The tags on offer: those the folder holds that this note does not carry.
    readonly property var available: root.unusedOf(root.suggestions, root.tags)
    // Whether a tag is being typed. A page that folds its fields away while
    // the keyboard is up asks this first, or it would close the very field
    // the keyboard was opened for.
    readonly property bool editing: entry.activeFocus

    function has(path) {
        for (var i = 0; i < root.tags.length; ++i) {
            if (root.tags[i].toLowerCase() === path.toLowerCase())
                return true;

        }
        return false;
    }

    // An array is replaced rather than changed in place, so the bindings that
    // read it follow.
    function addTag(name) {
        var path = Format.tagPath(name);
        if (path === "" || root.has(path))
            return ;

        root.tags = root.tags.concat([path]);
    }

    function removeAt(index) {
        var next = root.tags.slice();
        next.splice(index, 1);
        root.tags = next;
    }

    function unusedOf(all, used) {
        var out = [];
        for (var i = 0; i < all.length; ++i) {
            if (!root.has(all[i]))
                out.push(all[i]);

        }
        return out;
    }

    spacing: Theme.paddingSmall

    IconField {
        id: chipsField

        width: parent.width
        topPadding: Theme.paddingMedium
        description: qsTr("Tags")
        icon.source: Qt.resolvedUrl("../icons/icon-m-tag.svg")
        icon.color: Theme.primaryColor

        Flow {
            x: chipsField.grooveLeftMargin
            width: parent.width - x - chipsField.grooveRightMargin
            spacing: Theme.paddingSmall

            Label {
                visible: root.tags.length === 0
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("None")
            }

            Repeater {
                model: root.tags

                TagChip {
                    text: Format.pathLabel(modelData)
                    suffix: "\u00d7"
                    onClicked: root.removeAt(index)
                }

            }

        }

    }

    Flow {
        x: chipsField.contentLeft
        width: parent.width - x - Theme.horizontalPageMargin
        spacing: Theme.paddingSmall

        Repeater {
            model: root.available

            TagChip {
                text: Format.pathLabel(modelData)
                filled: false
                onClicked: root.addTag(modelData)
            }

        }

    }

    TextField {
        id: entry

        width: parent.width
        textLeftMargin: chipsField.contentLeft
        label: qsTr("New tag")
        placeholderText: qsTr("Work/Projects")
        inputMethodHints: Qt.ImhNoAutoUppercase
        // A tap on a chip pulls focus off the field, and clearing it would
        // take the keyboard down in the middle of naming a tag.
        focusOutBehavior: FocusBehavior.KeepFocus
        EnterKey.iconSource: "image://theme/icon-m-add"
        EnterKey.onClicked: {
            root.addTag(entry.text);
            entry.text = "";
        }
    }

}
