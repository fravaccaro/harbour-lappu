/*!
    \qmltype MarkdownTable
    \brief A GitHub-style Markdown table drawn as a grid.

    Takes the lines of one table as \c content, pipes and all, and reads them
    itself: the first row is the header, the row of dashes under it sets each
    column's alignment, the rest are the body. Anything that is not a table
    draws nothing, so the component is safe to instantiate on any block.

    Columns are as wide as their widest cell and no wider than the note, and
    the grid flicks sideways when the columns together want more room than
    that. A cell keeps its inline Markdown, links included, and a tapped link
    is only re-emitted through \c linkActivated: the text came from the
    server, so the owner decides what happens to it.

    \qml
    MarkdownTable {
        width: parent.width
        content: "| Name | Qty |\n| --- | ---: |\n| Apples | 3 |"
        onLinkActivated: L.LinkHandler.openOrCopyUrl(link)
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0
import "../notes/NoteFormat.js" as Format

Item {
    id: root

    property string content
    property var columnWidths: []
    readonly property var table: Format.tableOf(root.content)
    readonly property var rows: root.table !== null ? root.table.rows : []
    readonly property var alignments: root.table !== null ? root.table.alignments : []
    readonly property int columnCount: root.alignments.length

    signal linkActivated(string link)

    // Text cannot be measured declaratively, and nothing outside this file
    // reads the widths, so they are the one thing assigned rather than bound.
    function remeasure() {
        var widths = []
        for (var c = 0; c < root.columnCount; ++c)
            widths.push(0)
        for (var r = 0; r < root.rows.length; ++r)
            widths = widerRow(widths, root.rows[r])
        root.columnWidths = capped(widths)
    }

    // Every column is measured in the header's bold, since a column sized to
    // its lighter body text would clip the name standing above it.
    function widerRow(widths, cells) {
        for (var c = 0; c < widths.length; ++c) {
            ruler.text = c < cells.length ? Format.stripMarkdown(cells[c]) : ""
            widths[c] = Math.max(widths[c], ruler.width + 2 * Theme.paddingMedium)
        }
        return widths
    }

    // No column is wider than the note itself, so a long cell wraps instead of
    // pushing every column after it out of reach.
    function capped(widths) {
        var limit = root.width - 2 * Theme.horizontalPageMargin
        for (var c = 0; c < widths.length; ++c)
            widths[c] = Math.min(widths[c], limit)
        return widths
    }

    function columnWidth(column) {
        return column < root.columnWidths.length ? root.columnWidths[column] : 0
    }

    function gridWidth() {
        var total = 0
        for (var c = 0; c < root.columnWidths.length; ++c)
            total += root.columnWidths[c]
        return total
    }

    // A row the writer left short still fills the width of the table, so the
    // grid stays rectangular however the Markdown was typed.
    function cell(row, column) {
        var cells = root.rows[row]
        return cells !== undefined && column < cells.length ? cells[column] : ""
    }

    function alignmentOf(column) {
        var at = column < root.alignments.length ? root.alignments[column] : "left"
        if (at === "right")
            return Text.AlignRight
        return at === "center" ? Text.AlignHCenter : Text.AlignLeft
    }

    width: parent ? parent.width : Screen.width
    height: viewport.height

    onContentChanged: root.remeasure()
    onWidthChanged: root.remeasure()
    Component.onCompleted: root.remeasure()

    // A Label rather than TextMetrics, so the ruler carries the very font the
    // cells are drawn in instead of a copy of it kept in step by hand.
    Label {
        id: ruler

        font.pixelSize: Theme.fontSizeSmall
        font.bold: true
        visible: false
    }

    Flickable {
        id: viewport

        width: root.width
        height: grid.height
        contentWidth: Math.max(grid.width + 2 * Theme.horizontalPageMargin, width)
        contentHeight: grid.height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Column {
            id: grid

            x: Theme.horizontalPageMargin
            width: root.gridWidth()

            Repeater {
                model: root.rows.length

                delegate: Column {
                    id: rowBlock

                    property int rowIndex: index

                    width: grid.width

                    Row {
                        width: parent.width

                        Repeater {
                            model: root.columnCount

                            delegate: Item {
                                width: root.columnWidth(index)
                                height: cellText.height + 2 * Theme.paddingSmall

                                Label {
                                    id: cellText

                                    x: Theme.paddingMedium
                                    y: Theme.paddingSmall
                                    width: parent.width - 2 * Theme.paddingMedium
                                    wrapMode: Text.WordWrap
                                    textFormat: Text.RichText
                                    horizontalAlignment: root.alignmentOf(index)
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: rowBlock.rowIndex === 0
                                    color: Theme.primaryColor
                                    linkColor: Theme.highlightColor
                                    text: Format.cellHtml(root.cell(rowBlock.rowIndex, index))
                                    onLinkActivated: root.linkActivated(link)
                                }
                            }
                        }
                    }

                    Separator {
                        width: parent.width
                        visible: rowBlock.rowIndex === 0
                        color: Theme.secondaryColor
                        horizontalAlignment: Qt.AlignHCenter
                    }
                }
            }
        }
    }
}
