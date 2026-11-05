/*!
    \qmltype MarkdownToolbar
    \brief Inserts basic markdown around the caret of a TextArea.

    The bar does not own the note. It only wraps or prefixes the current
    selection of \c target. An empty selection inserts the markers and leaves
    the caret between them. Heading buttons replace an existing ATX prefix
    rather than stacking hashes. The table button is the one that writes
    several lines: it lays down an empty two-column table on lines of its own
    and hands back the first column name selected, ready to be typed over.

    The footnote button writes in two places at once: a reference at the caret
    and its note at the foot of the text, numbered with the lowest the note has
    not spent. The caret is left after the note's colon, ready for what it says.

    Every button restores focus to \c target before it edits, so the keyboard a
    writer had open stays open. The field keeps that focus through the edit with
    \c {focusOutBehavior: FocusBehavior.KeepFocus}.

    The buttons stand on one line, dragged sideways when they want more room
    than the screen has. The bar is made of nothing but buttons, so a drag
    begins on one: Silica's \c DragFilter hands it to the flickable and cancels
    the press, which is why a drag scrolls rather than writing a mark. Neither
    the flickable nor a button takes focus, so dragging the bar leaves the
    keyboard up just as tapping it does.

    Because a bar that is wider than the screen does not say so, it nudges
    itself once when it is built: a stop's worth of slide and back again. The
    nudge travels \c hintDistance, which is nothing at all when every button
    already fits, so a wide screen sees no movement.

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
    // A square stop, sized to the finger rather than to the screen: the bar
    // is dragged when the buttons want more room than there is.
    readonly property int buttonWidth: Theme.itemSizeSmall
    // How far the opening nudge travels: one stop, or whatever is hidden if
    // less than a stop is. Nothing is hidden, nothing moves.
    readonly property real hintDistance: Math.min(buttonWidth, Math.max(0, viewport.contentWidth - viewport.width))

    function remember() {
        if (!target)
            return ;

        if (history)
            history.seal();

        savedStart = target.selectionStart;
        savedEnd = target.selectionEnd;
    }

    function restore() {
        target.forceActiveFocus();
        target.select(savedStart, savedEnd);
    }

    function wrap(before, after) {
        if (!target)
            return ;

        restore();
        if (savedStart === savedEnd)
            insertEmpty(before, after);
        else
            wrapRange(before, after);
    }

    function splice(from, to, chunk) {
        var t = target.text;
        target.text = t.substring(0, from) + chunk + t.substring(to);
    }

    function insertEmpty(before, after) {
        splice(savedStart, savedStart, before + after);
        target.cursorPosition = savedStart + before.length;
    }

    function wrapRange(before, after) {
        var inner = target.text.substring(savedStart, savedEnd);
        splice(savedStart, savedEnd, before + inner + after);
        target.select(savedStart + before.length, savedEnd + before.length);
    }

    function lineStart(text, pos) {
        var at = text.lastIndexOf("\n", pos - 1);
        return at < 0 ? 0 : at + 1;
    }

    function prefixLine(prefix) {
        if (!target)
            return ;

        restore();
        var pos = lineStart(target.text, savedStart);
        splice(pos, pos, prefix);
        target.cursorPosition = savedEnd + prefix.length;
    }

    function hashes(level) {
        var marks = "";
        for (var i = 0; i < level; ++i) marks += "#"
        return marks + " ";
    }

    function setHeading(level) {
        if (!target)
            return ;

        restore();
        replaceHeading(lineStart(target.text, savedStart), hashes(level));
    }

    function replaceHeading(pos, prefix) {
        var rest = target.text.substring(pos);
        var match = rest.match(/^#{1,6}\s+/);
        var n = match ? match[0].length : 0;
        target.text = target.text.substring(0, pos) + prefix + rest.substring(n);
        target.cursorPosition = pos + prefix.length;
    }

    function apply(kind) {
        if (kind === "bold")
            wrap("**", "**");
        else if (kind === "italic")
            wrap("*", "*");
        else if (kind === "underline")
            wrap("<u>", "</u>");
        else if (kind === "strike")
            wrap("~~", "~~");
        else
            applyBlock(kind);
    }

    function applyBlock(kind) {
        if (kind === "list")
            prefixLine("- ");
        else if (kind === "task")
            prefixLine("- [ ] ");
        else if (kind === "link")
            wrap("[", "]()");
        else if (kind === "table")
            insertTable();
        else if (kind === "footnote")
            insertFootnote();
        else if (kind.length === 2 && kind.charAt(0) === "h")
            setHeading(Number(kind.charAt(1)));
    }

    // The name of a column the writer is expected to replace, which is why it
    // is handed back selected rather than merely written.
    function columnName() {
        return qsTr("Column");
    }

    function tableSkeleton() {
        return "| " + columnName() + " | " + columnName() + " |\n| --- | --- |\n|  |  |\n";
    }

    // Past the leading "| " of the first header cell.
    function insertTable() {
        if (!target)
            return ;

        restore();
        var at = insertBlock(tableSkeleton()) + 2;
        target.select(at, at + columnName().length);
    }

    // Where the block landed. A table is a block, so it begins on a line of
    // its own however the caret was placed.
    function insertBlock(chunk) {
        var lead = savedStart > 0 && target.text.charAt(savedStart - 1) !== "\n" ? "\n" : "";
        splice(savedStart, savedEnd, lead + chunk);
        return savedStart + lead.length;
    }

    // The smallest number the note has not spent, so a new reference never
    // lands on one already written. A note labelled in words keeps its words:
    // only the numbered ones are counted.
    function nextFootnoteId() {
        var used = {};
        var refs = target.text.match(/\[\^\d+\]/g) || [];
        for (var i = 0; i < refs.length; ++i)
            used[refs[i].substring(2, refs[i].length - 1)] = true;
        var id = 1;
        while (used[String(id)]) id += 1
        return id;
    }

    // A reference where the caret is and its note at the foot of the text,
    // which is where the reader is shown it.
    function insertFootnote() {
        if (!target)
            return ;

        restore();
        var id = nextFootnoteId();
        splice(savedStart, savedEnd, "[^" + id + "]");
        appendDefinition(id);
    }

    // The caret follows the note down, so the writer says what it is rather
    // than having to find the place themselves.
    function appendDefinition(id) {
        var text = target.text;
        var lead = text.charAt(text.length - 1) === "\n" ? "" : "\n\n";
        var opening = lead + "[^" + id + "]: ";
        target.text = text + opening;
        target.cursorPosition = text.length + opening.length;
    }

    height: Theme.itemSizeSmall
    Component.onCompleted: moreHint.start()

    // The bar slides off and comes back when the page opens, so a writer is
    // shown there is more of it than the screen holds. It waits for the page
    // to settle first, and a drag of the writer's own calls it off.
    SequentialAnimation {
        id: moreHint

        PauseAnimation {
            duration: 700
        }

        NumberAnimation {
            target: viewport
            property: "contentX"
            to: root.hintDistance
            duration: 500
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            target: viewport
            property: "contentX"
            to: 0
            duration: 500
            easing.type: Easing.InOutQuad
        }

    }

    Flickable {
        id: viewport

        anchors.fill: parent
        contentWidth: Math.max(bar.width + 2 * Theme.horizontalPageMargin, width)
        flickableDirection: Flickable.HorizontalFlick
        // An overscroll of the bar is the bar's own; the page below it stays
        // where the writer left it.
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        onMovementStarted: moreHint.stop()

        Row {
            id: bar

            // Against the page margin while the bar is wider than the screen,
            // and in the middle of it once the buttons all fit.
            x: Math.max(Theme.horizontalPageMargin, (viewport.contentWidth - width) / 2)
            spacing: Theme.paddingSmall

            Repeater {
                delegate: markButton

                model: ListModel {
                    ListElement {
                        label: "B"
                        kind: "bold"
                    }

                    ListElement {
                        label: "I"
                        kind: "italic"
                    }

                    ListElement {
                        label: "U"
                        kind: "underline"
                    }

                    ListElement {
                        label: "S"
                        kind: "strike"
                    }

                    ListElement {
                        label: "\ud834\udd18"
                        kind: "list"
                    }

                    ListElement {
                        label: "\ud83d\uddf8"
                        kind: "task"
                    }

                    ListElement {
                        label: "\ud83d\udd17"
                        kind: "link"
                    }

                    ListElement {
                        label: "H1"
                        kind: "h1"
                    }

                    ListElement {
                        label: "H2"
                        kind: "h2"
                    }

                    ListElement {
                        label: "H3"
                        kind: "h3"
                    }

                    ListElement {
                        label: "H4"
                        kind: "h4"
                    }

                    ListElement {
                        label: "H5"
                        kind: "h5"
                    }

                    ListElement {
                        label: "H6"
                        kind: "h6"
                    }

                    ListElement {
                        label: "\u25a6"
                        kind: "table"
                    }

                    ListElement {
                        label: "\u00b9"
                        kind: "footnote"
                    }

                }

            }

        }

    }

    Component {
        id: markButton

        BackgroundItem {
            id: stop

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
                // The stop itself: a child of a background item is handed to
                // an inner rectangle, which is never the one highlighted.
                color: stop.highlighted ? Theme.highlightColor : Theme.primaryColor
                text: label
            }

        }

    }

}
