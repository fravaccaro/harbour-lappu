.pragma library

function escapeHtml(text) {
    return String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
}

// A table reads as its cells once the grid is gone: the row of dashes under
// the header carries nothing, and only a line fenced by pipes gives its own
// up, so a pipe inside ordinary prose is left where it was written.
function stripTables(t) {
    t = t.replace(/^[ \t]*\|?[ \t]*:?-+:?[ \t]*(\|[ \t]*:?-+:?[ \t]*)*\|?[ \t]*$/gm, "")
    return t.replace(/^[ \t]*\|.*\|[ \t]*$/gm, function (row) {
        return row.replace(/\|/g, " ")
    })
}

function stripMarkdown(text) {
    var t = stripTables(String(text))
    t = t.replace(/^#+\s+/gm, "")
    t = t.replace(/\*\*(.*?)\*\*/g, "$1")
    t = t.replace(/\*(.*?)\*/g, "$1")
    t = t.replace(/~~(.*?)~~/g, "$1")
    t = t.replace(/<\/?u>/g, "")
    t = t.replace(/`([^`]+)`/g, "$1")
    t = t.replace(/^\s*- \[[ xX]\]\s*/gm, "")
    t = t.replace(/^\s*[-*]\s+/gm, "")
    t = t.replace(/\[([^\]]+)\]\([^)]+\)/g, "$1")
    return t.replace(/\s+/g, " ").trim()
}

function excerpt(text) {
    var line = stripMarkdown(text)
    if (line.length > 80)
        return line.substring(0, 79) + "\u2026"
    return line
}

// The cover shows the note as written, so the line breaks are kept and only
// the markup is taken out.
function plainText(text) {
    var lines = String(text).split("\n")
    var out = []
    for (var i = 0; i < lines.length; ++i) {
        var line = stripMarkdown(lines[i])
        // The dashes under a table header strip to nothing, and leaving the
        // line behind would open a gap the note does not have.
        if (line !== "" || lines[i].trim() === "")
            out.push(line)
    }
    return out.join("\n")
}

function categoryLabel(category) {
    if (category === "")
        return ""
    return String(category).split("/").join(" / ")
}

function headingHtml(t) {
    var match = t.match(/^(#{1,6}) /)
    if (!match)
        return ""
    return "<b>" + t.substring(match[0].length) + "</b>"
}

function inlineMarkup(t) {
    t = t.replace(/~~(.+?)~~/g, "<s>$1</s>")
    t = t.replace(/&lt;u&gt;(.+?)&lt;\/u&gt;/g, "<u>$1</u>")
    t = t.replace(/\*\*(.+?)\*\*/g, "<b>$1</b>")
    return t.replace(/\*(.+?)\*/g, "<i>$1</i>")
}

// Takes escaped text, as every caller escapes before any markup is read: a
// note may hold angle brackets of its own.
function richText(t) {
    t = inlineMarkup(t)
    return t.replace(/\[([^\]]+)\]\(([^)]+)\)/g, "<a href=\"$2\">$1</a>")
}

function inlineHtml(t) {
    t = richText(t)
    if (t.indexOf("- ") === 0)
        return "\u2022 " + t.substring(2)
    return t
}

// A cell is not a tap target, so unlike a task line it keeps its links whole.
// A leading "- " is a bullet nowhere but at the start of a line.
function cellHtml(text) {
    return richText(escapeHtml(text))
}

function parseTask(line) {
    var match = String(line).match(/^\s*- \[([ xX])\] ?/)
    if (!match)
        return null
    return {
        "done": match[1] !== " ",
        "text": String(line).substring(match[0].length)
    }
}

// The whole row is the tap target, so a link inside a task keeps its label
// but loses the anchor.
function taskHtml(text) {
    var t = inlineMarkup(escapeHtml(text))
    return t.replace(/\[([^\]]+)\]\([^)]+\)/g, "$1")
}

// The cells of one row, with the outer pipes dropped. A pipe a cell needs for
// itself is written \| and is not a divider.
function splitRow(line) {
    var t = String(line).trim().replace(/\\\|/g, "\u0001")
    t = t.replace(/^\|/, "").replace(/\|$/, "")
    var cells = t.split("|")
    for (var i = 0; i < cells.length; ++i)
        cells[i] = cells[i].trim().replace(/\u0001/g, "|")
    return cells
}

function isTableRow(line) {
    return String(line).indexOf("|") >= 0 && String(line).trim() !== ""
}

// The row of dashes under a header. A line of dashes with no pipe in it is a
// rule rather than a table, which is why the pipe is asked for first.
function isDelimiterRow(line) {
    if (String(line).indexOf("|") < 0)
        return false

    var cells = splitRow(line)
    for (var i = 0; i < cells.length; ++i) {
        if (!/^:?-+:?$/.test(cells[i]))
            return false
    }
    return cells.length > 0
}

function alignmentsOf(line) {
    var cells = splitRow(line)
    var out = []
    for (var i = 0; i < cells.length; ++i) {
        var left = cells[i].charAt(0) === ":"
        var right = cells[i].charAt(cells[i].length - 1) === ":"
        out.push(left && right ? "center" : (right ? "right" : "left"))
    }
    return out
}

// A delimiter row directly under the header, with one cell per column, is the
// whole of what tells a table from a sentence that happens to hold a pipe.
function tableStarts(lines, from) {
    if (from + 1 >= lines.length)
        return false
    if (!isTableRow(lines[from]) || !isDelimiterRow(lines[from + 1]))
        return false

    return splitRow(lines[from]).length === splitRow(lines[from + 1]).length
}

function tableAt(lines, from) {
    if (!tableStarts(lines, from))
        return null

    var rows = [splitRow(lines[from])]
    var at = from + 2
    while (at < lines.length && isTableRow(lines[at]))
        rows.push(splitRow(lines[at++]))

    return { "rows": rows,
             "alignments": alignmentsOf(lines[from + 1]),
             "lineCount": at - from }
}

// The table a block of lines holds, or null when those lines are not one.
function tableOf(block) {
    return tableAt(String(block).split("\n"), 0)
}

// Every row carries the line it came from, because a table stands for several
// lines at once and nothing downstream may assume one row is one line.
function previewRow(kind, done, html, lineIndex) {
    return { "kind": kind, "done": done, "html": html,
             "lineIndex": lineIndex, "block": "" }
}

function previewLine(line, lineIndex) {
    var task = parseTask(line)
    if (task)
        return previewRow("task", task.done, taskHtml(task.text), lineIndex)
    if (String(line).trim() === "")
        return previewRow("blank", false, "", lineIndex)
    return previewRow("text", false, lineToHtml(line), lineIndex)
}

// The markdown travels whole and the table is read again where it is drawn,
// so a row stays a flat set of values rather than a model of cells.
function tableBlock(lines, from, count) {
    var row = previewRow("table", false, "", from)
    row.block = lines.slice(from, from + count).join("\n")
    return row
}

function previewBlocks(text) {
    var lines = String(text).split("\n")
    var rows = []
    var at = 0
    while (at < lines.length) {
        var table = tableAt(lines, at)
        rows.push(table !== null ? tableBlock(lines, at, table.lineCount)
                                 : previewLine(lines[at], at))
        at += table !== null ? table.lineCount : 1
    }
    return rows
}

function lineToHtml(line) {
    var t = escapeHtml(line)
    var heading = headingHtml(t)
    return heading !== "" ? heading : inlineHtml(t)
}