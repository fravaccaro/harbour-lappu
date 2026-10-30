import QtQuick 2.0
import Sailfish.Silica 1.0
import "../notes/NoteFormat.js" as Format

/*!
    \brief Read-only markdown preview whose task lines are tappable.

    Renders \c content as a run of blocks. Nextcloud task lines (\c {- [ ] }
    and \c {- [x] }) become full-width tap targets, a GitHub-style table
    becomes a grid, every other line becomes rich text. The component never
    rewrites the text itself: it reports the line of the tapped task through
    \c taskClicked so the owner can flip that line. A table stands for several
    lines at once, so that number is the line within \c content rather than
    the row it was drawn in.

    Task rows are disabled while \c locked is true.

    \code
    NoteBody {
        width: parent.width
        content: note.content
        locked: note.locked
        onTaskClicked: store.toggleTask(note.noteId, lineIndex)
        onLinkActivated: L.LinkHandler.openOrCopyUrl(link)
    }
    \endcode
*/
Column {
    id: root

    property string content
    property bool locked

    signal taskClicked(int lineIndex)
    signal linkActivated(string link)

    function rebuild() {
        var rows = Format.previewBlocks(root.content)
        lineModel.clear()
        for (var i = 0; i < rows.length; ++i)
            lineModel.append(rows[i])
    }

    onContentChanged: root.rebuild()
    Component.onCompleted: root.rebuild()

    Repeater {
        model: ListModel {
            id: lineModel
        }

        delegate: Column {
            width: root.width

            BackgroundItem {
                id: taskRow

                width: parent.width
                height: Math.max(Theme.itemSizeSmall, taskLabel.height + 2 * Theme.paddingSmall)
                visible: kind === "task"
                enabled: !root.locked
                onClicked: root.taskClicked(model.lineIndex)

                Label {
                    id: checkGlyph

                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    color: taskRow.highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: done ? "\u2611" : "\u2610"
                }

                Label {
                    id: taskLabel

                    x: checkGlyph.x + checkGlyph.width + Theme.paddingMedium
                    width: parent.width - x - Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    wrapMode: Text.WordWrap
                    textFormat: Text.RichText
                    color: taskRow.highlighted ? Theme.highlightColor
                                               : (done ? Theme.secondaryColor : Theme.primaryColor)
                    text: html
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: kind === "text"
                wrapMode: Text.WordWrap
                textFormat: Text.RichText
                color: Theme.primaryColor
                linkColor: Theme.highlightColor
                text: html
                onLinkActivated: root.linkActivated(link)
            }

            // Loaded rather than hidden, unlike the rows above: a grid is a
            // heavier thing to build than a label, and a note of a hundred
            // lines would otherwise carry a hundred of them.
            Loader {
                width: root.width
                active: kind === "table"

                sourceComponent: Component {
                    MarkdownTable {
                        width: root.width
                        content: block
                        onLinkActivated: root.linkActivated(link)
                    }
                }
            }

            Item {
                width: 1
                height: Theme.paddingMedium
                visible: kind === "blank"
            }
        }
    }
}
