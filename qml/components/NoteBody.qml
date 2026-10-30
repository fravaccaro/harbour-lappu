import QtQuick 2.0
import Sailfish.Silica 1.0
import "../notes/NoteFormat.js" as Format

/*!
    \brief Read-only markdown preview whose task lines are tappable.

    Renders \c content line by line. Nextcloud task lines (\c {- [ ] } and
    \c {- [x] }) become full-width tap targets, every other line becomes rich
    text. The component never rewrites the text itself: it reports the index of
    the tapped line through \c taskClicked so the owner can flip that line.

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
        var lines = String(root.content).split("\n")
        lineModel.clear()
        for (var i = 0; i < lines.length; ++i)
            lineModel.append(Format.previewLine(lines[i]))
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
                onClicked: root.taskClicked(index)

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

            Item {
                width: 1
                height: Theme.paddingMedium
                visible: kind === "blank"
            }
        }
    }
}
