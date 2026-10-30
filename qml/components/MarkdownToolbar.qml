/*!
    \qmltype MarkdownToolbar
    \brief Inserts basic markdown around the caret of a TextArea.

    The bar does not own the note. It only wraps or prefixes the current
    selection of \c target. An empty selection inserts the markers and leaves
    the caret between them. Heading buttons replace an existing ATX prefix
    rather than stacking hashes. The table button is the one that writes
    several lines: it lays down an empty two-column table on lines of its own
    and hands back the first column name selected, ready to be typed over.

    \qml
    MarkdownToolbar {
        width: parent.width
        target: bodyField
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property Item target
    // Optional. When set, each format tap is recorded as its own step rather
    // than merging into the word last typed.
    property QtObject history
    property int savedStart: 0
    property int savedEnd: 0
    readonly property int buttonWidth: Math.max(Theme.fontSizeLarge,
                                                Math.floor((width - 6 * Theme.paddingSmall) / 7))

    function remember() {
        if (!target)
            return
        if (history)
            history.seal()
        savedStart = target.selectionStart
        savedEnd = target.selectionEnd
    }

    function restore() {
        target.forceActiveFocus()
        target.select(savedStart, savedEnd)
    }

    function wrap(before, after) {
        if (!target)
            return
        restore()
        if (savedStart === savedEnd)
            insertEmpty(before, after)
        else
            wrapRange(before, after)
    }

    function splice(from, to, chunk) {
        var t = target.text
        target.text = t.substring(0, from) + chunk + t.substring(to)
    }

    function insertEmpty(before, after) {
        splice(savedStart, savedStart, before + after)
        target.cursorPosition = savedStart + before.length
    }

    function wrapRange(before, after) {
        var inner = target.text.substring(savedStart, savedEnd)
        splice(savedStart, savedEnd, before + inner + after)
        target.select(savedStart + before.length, savedEnd + before.length)
    }

    function lineStart(text, pos) {
        var at = text.lastIndexOf("\n", pos - 1)
        return at < 0 ? 0 : at + 1
    }

    function prefixLine(prefix) {
        if (!target)
            return
        restore()
        var pos = lineStart(target.text, savedStart)
        splice(pos, pos, prefix)
        target.cursorPosition = savedEnd + prefix.length
    }

    function hashes(level) {
        var marks = ""
        for (var i = 0; i < level; ++i)
            marks += "#"
        return marks + " "
    }

    function setHeading(level) {
        if (!target)
            return
        restore()
        replaceHeading(lineStart(target.text, savedStart), hashes(level))
    }

    function replaceHeading(pos, prefix) {
        var rest = target.text.substring(pos)
        var match = rest.match(/^#{1,6}\s+/)
        var n = match ? match[0].length : 0
        target.text = target.text.substring(0, pos) + prefix + rest.substring(n)
        target.cursorPosition = pos + prefix.length
    }

    function apply(kind) {
        if (kind === "bold")
            wrap("**", "**")
        else if (kind === "italic")
            wrap("*", "*")
        else if (kind === "underline")
            wrap("<u>", "</u>")
        else if (kind === "strike")
            wrap("~~", "~~")
        else
            applyBlock(kind)
    }

    function applyBlock(kind) {
        if (kind === "list")
            prefixLine("- ")
        else if (kind === "task")
            prefixLine("- [ ] ")
        else if (kind === "link")
            wrap("[", "]()")
        else if (kind === "table")
            insertTable()
        else if (kind.length === 2 && kind.charAt(0) === "h")
            setHeading(Number(kind.charAt(1)))
    }

    // The name of a column the writer is expected to replace, which is why it
    // is handed back selected rather than merely written.
    function columnName() {
        return qsTr("Column")
    }

    function tableSkeleton() {
        return "| " + columnName() + " | " + columnName() + " |\n| --- | --- |\n|  |  |\n"
    }

    // Past the leading "| " of the first header cell.
    function insertTable() {
        if (!target)
            return
        restore()
        var at = insertBlock(tableSkeleton()) + 2
        target.select(at, at + columnName().length)
    }

    // Where the block landed. A table is a block, so it begins on a line of
    // its own however the caret was placed.
    function insertBlock(chunk) {
        var lead = savedStart > 0 && target.text.charAt(savedStart - 1) !== "\n" ? "\n" : ""
        splice(savedStart, savedEnd, lead + chunk)
        return savedStart + lead.length
    }

    height: Theme.itemSizeSmall * 2

    Column {
        width: parent.width
        spacing: 0

        Row {
            spacing: Theme.paddingSmall
            anchors.horizontalCenter: parent.horizontalCenter

            Repeater {
                model: ListModel {
                    ListElement { label: "B"; kind: "bold" }
                    ListElement { label: "I"; kind: "italic" }
                    ListElement { label: "U"; kind: "underline" }
                    ListElement { label: "S"; kind: "strike" }
                    ListElement { label: "\ud834\udd18"; kind: "list" }
                    ListElement { label: "\ud83d\uddf8"; kind: "task" }
                    ListElement { label: "\ud83d\udd17"; kind: "link" }
                }

                delegate: markButton
            }
        }

        Row {
            spacing: Theme.paddingSmall
            anchors.horizontalCenter: parent.horizontalCenter

            Repeater {
                model: ListModel {
                    ListElement { label: "H1"; kind: "h1" }
                    ListElement { label: "H2"; kind: "h2" }
                    ListElement { label: "H3"; kind: "h3" }
                    ListElement { label: "H4"; kind: "h4" }
                    ListElement { label: "H5"; kind: "h5" }
                    ListElement { label: "H6"; kind: "h6" }
                    ListElement { label: "\u25a6"; kind: "table" }
                }

                delegate: markButton
            }
        }
    }

    Component {
        id: markButton

        BackgroundItem {
            width: root.buttonWidth
            height: Theme.itemSizeSmall
            onPressed: root.remember()
            onClicked: root.apply(kind)

            Label {
                anchors.centerIn: parent
                font.pixelSize: Theme.fontSizeSmall
                font.bold: kind === "bold"
                font.italic: kind === "italic"
                font.underline: kind === "underline"
                font.strikeout: kind === "strike"
                color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                text: label
            }
        }
    }
}
