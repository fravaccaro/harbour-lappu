import QtQuick 2.0
import Sailfish.Silica 1.0
import "../notes/NoteFormat.js" as Format

/*!
    \brief Read-only markdown preview whose task lines are tappable.

    Renders \c content as a run of blocks. A task line - any of \c {- [ ] },
    \c {* [x] } or \c {+ [ ] } - becomes a full-width tap target, a heading is
    set in its own size, a quote gets a bar beside it, a fenced code block is
    drawn as code and a GitHub-style table as a grid; every other line becomes
    rich text. A nested item is indented as it was written.

    The component never rewrites the text itself: it reports the line of the
    tapped task through \c taskClicked so the owner can flip that line. A
    table or a code block stands for several lines at once, so that number is
    the line within \c content rather than the row it was drawn in.

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

    // A heading is told apart by size as much as by weight. Past the second
    // level there is no size left to give one, so the rest read as bold.
    function headingSize(level) {
        if (level === 1)
            return Theme.fontSizeExtraLarge
        return level === 2 ? Theme.fontSizeLarge : Theme.fontSizeMedium
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

                    x: Theme.horizontalPageMargin + indent * Theme.paddingLarge
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

            // Prose, a bullet and a heading are one label: a heading differs
            // only in the weight and size it is set in.
            Label {
                x: Theme.horizontalPageMargin + indent * Theme.paddingLarge
                width: parent.width - x - Theme.horizontalPageMargin
                visible: kind === "text" || kind === "heading"
                wrapMode: Text.WordWrap
                textFormat: Text.RichText
                font.bold: kind === "heading"
                font.pixelSize: kind === "heading" ? root.headingSize(level)
                                                   : Theme.fontSizeMedium
                color: Theme.primaryColor
                linkColor: Theme.highlightColor
                text: html
                onLinkActivated: root.linkActivated(link)
            }

            // A quote is marked by a bar beside it rather than by the > it was
            // written with. The row is exactly as tall as its text, so the
            // bars of a quote running over several lines join into one.
            Item {
                width: parent.width
                height: quoteText.height
                visible: kind === "quote"

                Rectangle {
                    id: quoteBar

                    x: Theme.horizontalPageMargin + indent * Theme.paddingLarge
                    width: Math.round(Theme.paddingSmall / 3)
                    height: parent.height
                    color: Theme.secondaryColor
                    opacity: 0.6
                }

                Label {
                    id: quoteText

                    x: quoteBar.x + quoteBar.width + Theme.paddingMedium
                    width: parent.width - x - Theme.horizontalPageMargin
                    wrapMode: Text.WordWrap
                    textFormat: Text.RichText
                    color: Theme.secondaryColor
                    linkColor: Theme.highlightColor
                    text: html
                    onLinkActivated: root.linkActivated(link)
                }
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

            Loader {
                width: root.width
                active: kind === "code"

                sourceComponent: Component {
                    MarkdownCode {
                        width: root.width
                        content: block
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
