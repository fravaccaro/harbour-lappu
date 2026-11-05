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

// What a line is marked as: the hashes of a heading, the fence of a code
// block, the > of a quote, the bullet of a list and the brackets of a task.
// The task is taken off before the bullet, being also a bullet. A footnote
// keeps what it says and gives up the label that named it.
function stripBlocks(t) {
    t = stripTables(t)
    t = t.replace(/^[ \t]*```.*$/gm, "")
    t = t.replace(/^#+[ \t]+/gm, "")
    t = t.replace(/^[ \t]*>+[ \t]?/gm, "")
    t = t.replace(/^\[\^[^\]\s&<>"]+\]:[ \t]*/gm, "")
    t = t.replace(/^[ \t]*[-*+] \[[ xX]\][ \t]*/gm, "")
    return t.replace(/^[ \t]*[-*+][ \t]+/gm, "")
}

// What a run of text inside a line is marked with. A reference stands for a
// number the list and the cover have no room to give, so it goes entirely.
function stripSpans(t) {
    t = t.replace(/\[\^[^\]\s&<>"]+\]/g, "")
    t = t.replace(/\*\*(.*?)\*\*/g, "$1")
    t = t.replace(/\*(.*?)\*/g, "$1")
    t = t.replace(/~~(.*?)~~/g, "$1")
    t = t.replace(/<\/?u>/g, "")
    t = t.replace(/`([^`]+)`/g, "$1")
    return t.replace(/\[([^\]]+)\]\([^)]+\)/g, "$1")
}

function stripMarkdown(text) {
    var t = stripSpans(stripBlocks(String(text)))
    return t.replace(/\s+/g, " ").trim()
}

// The opening of the note as plain text, for the rows and cards. The note is
// drawn whole, its name being the file's rather than the text's, so a first
// line that repeats the name is shown like any other.
function excerpt(text) {
    var line = stripMarkdown(String(text))
    if (line.length > 100)
        return line.substring(0, 99) + "\u2026"
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

// The name of a note as it is shown above or beside it: a favourite carries
// a star, wherever it is named - the rows, the cards and the note itself.
function starredTitle(title, favorite) {
    return favorite ? title + " \u2605" : title
}

// The level of an ATX heading, or 0 for a line that is not one. The size is
// the drawing's business, so nothing is marked up here.
function headingOf(line) {
    var match = String(line).match(/^(#{1,6}) /)
    return match ? match[1].length : 0
}

function inlineMarkup(t) {
    t = t.replace(/~~(.+?)~~/g, "<s>$1</s>")
    t = t.replace(/&lt;u&gt;(.+?)&lt;\/u&gt;/g, "<u>$1</u>")
    t = t.replace(/\*\*(.+?)\*\*/g, "<b>$1</b>")
    return t.replace(/\*(.+?)\*/g, "<i>$1</i>")
}

// A reference is drawn as the number the foot of the note gave it, never as
// the label it was written with, so a note numbered 1, bignote, 1 still reads
// 1, 2, 3. One the note never defines points at nothing and is left alone.
function footnoteMarks(t, order) {
    if (!order)
        return t

    return String(t).replace(/\[\^([^\]\s&<>"]+)\]/g, function (whole, id) {
        if (!order.hasOwnProperty(id))
            return whole
        return "<sup><a href=\"footnote:" + id + "\">" + order[id] + "</a></sup>"
    })
}

function markedUp(t, order) {
    t = footnoteMarks(inlineMarkup(t), order)
    return t.replace(/\[([^\]]+)\]\(([^)]+)\)/g, "<a href=\"$2\">$1</a>")
}

function codeSpan(span) {
    return "<code>" + span.substring(1, span.length - 1) + "</code>"
}

// Takes escaped text, as every caller escapes before any markup is read: a
// note may hold angle brackets of its own. A backquoted span is read before
// every other mark and kept as it was written, or the asterisks of a snippet
// would be taken for emphasis. Splitting on a captured group leaves those
// spans at the odd places.
function richText(t, order) {
    var parts = String(t).split(/(`[^`]+`)/)
    for (var i = 0; i < parts.length; ++i)
        parts[i] = i % 2 === 1 ? codeSpan(parts[i]) : markedUp(parts[i], order)
    return parts.join("")
}

// One line of prose, escaped before any markup is read. Every row that draws
// text goes through here, cells and headings included. The numbering is handed
// down from the whole note, so a caller with none - a table cell, which is no
// place for a footnote - leaves a reference as it was written.
function lineToHtml(line, order) {
    return richText(escapeHtml(line), order)
}

// A nesting step is two spaces, a tab being four of them. Past the third step
// there would be no width left for the text itself.
function indentOf(lead) {
    var spaces = String(lead).replace(/\t/g, "    ").length
    return Math.min(Math.floor(spaces / 2), 3)
}

// Any of -, * and + opens a list item, which is what the server's own renderer
// accepts, and what decides a task is the brackets rather than the bullet.
function parseTask(line) {
    var match = String(line).match(/^([ \t]*)[-*+] \[([ xX])\] ?/)
    if (!match)
        return null
    return {
        "done": match[2] !== " ",
        "indent": indentOf(match[1]),
        "text": String(line).substring(match[0].length)
    }
}

function parseBullet(line) {
    var match = String(line).match(/^([ \t]*)[-*+] /)
    if (!match)
        return null
    return {
        "indent": indentOf(match[1]),
        "text": String(line).substring(match[0].length)
    }
}

// The > of a quote, with the ones a nested quote adds counted as nesting.
function parseQuote(line) {
    var match = String(line).match(/^[ \t]*(>+)[ \t]?/)
    if (!match)
        return null
    return {
        "indent": Math.min(match[1].length - 1, 3),
        "text": String(line).substring(match[0].length)
    }
}

// The whole row is the tap target, so a link inside a task keeps its label
// but loses the anchor. A reference keeps its number for the same reason: the
// anchor goes and the superscript stays.
function taskHtml(text, order) {
    return richText(escapeHtml(text), order).replace(/<a href="[^"]*">([^<]*)<\/a>/g, "$1")
}

// The note a definition line opens: [^id]: followed by what it says, with the
// indented lines under it belonging to the same note. An id holds no space,
// and none of the characters escaping would rewrite, so the label read here
// is the same one read in the body.
function footnoteDefAt(lines, from) {
    var match = String(lines[from]).match(/^\[\^([^\]\s&<>"]+)\]:[ \t]?/)
    if (!match)
        return null

    var said = [String(lines[from]).substring(match[0].length)]
    var at = from + 1
    while (at < lines.length && /^[ \t]+\S/.test(lines[at]))
        said.push(String(lines[at++]).trim())

    return { "id": match[1], "text": said.join(" ").trim(), "lineCount": at - from }
}

// Every footnote the note defines, in the order they were written. A fence is
// stepped over whole: inside one, a definition is code like any other line.
function footnotesOf(lines) {
    var book = { "defs": {}, "ids": [] }
    var at = 0
    while (at < lines.length) {
        var code = codeAt(lines, at)
        var def = code === null ? footnoteDefAt(lines, at) : null
        if (def !== null && !book.defs.hasOwnProperty(def.id))
            book.ids.push(def.id)
        if (def !== null)
            book.defs[def.id] = def.text
        at += code !== null ? code.lineCount : (def !== null ? def.lineCount : 1)
    }
    return book
}

// What a line owes to the footnotes rather than to the body: a fence or a
// definition, counted in lines, or 0 for a line the body keeps.
function footnoteSkipAt(lines, from) {
    var code = codeAt(lines, from)
    if (code !== null)
        return code.lineCount

    var def = footnoteDefAt(lines, from)
    return def !== null ? def.lineCount : 0
}

// A backquoted span is not read for references, as it is not read for any
// other mark, or a snippet would spend a number nothing ever draws.
function numberRefs(line, defs, order, count) {
    var bare = String(line).replace(/`[^`]+`/g, "")
    var refs = bare.match(/\[\^[^\]\s&<>"]+\]/g) || []
    for (var i = 0; i < refs.length; ++i) {
        var id = refs[i].substring(2, refs[i].length - 1)
        if (defs.hasOwnProperty(id) && !order.hasOwnProperty(id))
            order[id] = ++count
    }
    return count
}

// The number each footnote is drawn as, given out in the order the body first
// asks for them. One the body never asks for still earns a number, after the
// rest, so a definition written and forgotten is still shown.
function footnoteOrder(lines, book) {
    var order = {}
    var count = 0
    var at = 0
    while (at < lines.length) {
        var skip = footnoteSkipAt(lines, at)
        if (skip === 0)
            count = numberRefs(lines[at], book.defs, order, count)
        at += skip > 0 ? skip : 1
    }
    return numberTheRest(book, order, count)
}

function numberTheRest(book, order, count) {
    for (var i = 0; i < book.ids.length; ++i) {
        if (!order.hasOwnProperty(book.ids[i]))
            order[book.ids[i]] = ++count
    }
    return order
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

    return {
        "rows": rows,
        "alignments": alignmentsOf(lines[from + 1]),
        "lineCount": at - from
    }
}

// The table a block of lines holds, or null when those lines are not one.
function tableOf(block) {
    return tableAt(String(block).split("\n"), 0)
}

// Every row carries the line it came from, because a block stands for several
// lines at once and nothing downstream may assume one row is one line. Every
// role is set on every row: a ListModel takes its roles from the first one.
function previewRow(kind, done, html, lineIndex) {
    return {
        "kind": kind, "done": done, "html": html, "lineIndex": lineIndex,
        "level": 0, "indent": 0, "block": ""
    }
}

function taskRow(task, lineIndex, order) {
    var row = previewRow("task", task.done, taskHtml(task.text, order), lineIndex)
    row.indent = task.indent
    return row
}

function quoteRow(quote, lineIndex, order) {
    var row = previewRow("quote", false, lineToHtml(quote.text, order), lineIndex)
    row.indent = quote.indent
    return row
}

function headingRow(line, level, lineIndex, order) {
    var text = String(line).substring(level + 1)
    var row = previewRow("heading", false, lineToHtml(text, order), lineIndex)
    row.level = level
    return row
}

// A bullet is prose with a glyph in front of it, so it draws as any other line
// of text does and only its indent sets it apart.
function bulletRow(bullet, lineIndex, order) {
    var html = "\u2022 " + lineToHtml(bullet.text, order)
    var row = previewRow("text", false, html, lineIndex)
    row.indent = bullet.indent
    return row
}

// The kinds a line is known by from the way it begins, or null when it begins
// with nothing in particular. A task is asked about before a bullet, being
// also a bullet, and a quote before either, since it may hold one.
function markedRow(line, lineIndex, order) {
    var task = parseTask(line)
    if (task !== null)
        return taskRow(task, lineIndex, order)

    var quote = parseQuote(line)
    if (quote !== null)
        return quoteRow(quote, lineIndex, order)

    return headingOrBullet(line, lineIndex, order)
}

function headingOrBullet(line, lineIndex, order) {
    var level = headingOf(line)
    if (level > 0)
        return headingRow(line, level, lineIndex, order)

    var bullet = parseBullet(line)
    return bullet !== null ? bulletRow(bullet, lineIndex, order) : null
}

function previewLine(line, lineIndex, order) {
    var marked = markedRow(line, lineIndex, order)
    if (marked !== null)
        return marked
    if (String(line).trim() === "")
        return previewRow("blank", false, "", lineIndex)
    return previewRow("text", false, lineToHtml(line, order), lineIndex)
}

// The markdown travels whole and the table is read again where it is drawn,
// so a row stays a flat set of values rather than a model of cells.
function tableBlock(lines, from, count) {
    var row = previewRow("table", false, "", from)
    row.block = lines.slice(from, from + count).join("\n")
    return row
}

function isFence(line) {
    return /^[ \t]*```/.test(String(line))
}

// A fence takes every line to its closing fence, or to the end of the note
// when the writer never closed it. The fences themselves draw nothing.
function codeAt(lines, from) {
    if (!isFence(lines[from]))
        return null

    var at = from + 1
    while (at < lines.length && !isFence(lines[at]))
        at += 1

    return {
        "text": lines.slice(from + 1, at).join("\n"),
        "lineCount": Math.min(at + 1, lines.length) - from
    }
}

function codeRow(code, lineIndex) {
    var row = previewRow("code", false, "", lineIndex)
    row.block = code.text
    return row
}

// A run of lines that is not read one line at a time, or null for one that is.
// Code is asked about first: inside a fence, a row of pipes is code and a hash
// is a hash.
function blockAt(lines, from) {
    var code = codeAt(lines, from)
    if (code !== null)
        return { "row": codeRow(code, from), "lineCount": code.lineCount }

    var table = tableAt(lines, from)
    if (table === null)
        return null

    return {
        "row": tableBlock(lines, from, table.lineCount),
        "lineCount": table.lineCount
    }
}

// What the lines beginning at \a at draw, and where the next row begins. A
// definition draws nothing here - it is shown at the foot of the note - but
// the lines it took are still counted, so every row that follows keeps the
// line it was written on and a task still flips the line it names.
function pushRow(rows, lines, at, order) {
    var block = blockAt(lines, at)
    if (block !== null) {
        rows.push(block.row)
        return at + block.lineCount
    }

    var def = footnoteDefAt(lines, at)
    if (def !== null)
        return at + def.lineCount

    rows.push(previewLine(lines[at], at, order))
    return at + 1
}

// The notes themselves, gathered under the body in the order of their numbers
// rather than of their writing, each opening with the number the body shows.
// They stand for no line of their own, so they carry none. A rule opens them,
// since what follows it is no longer the note being read but its asides.
function footnoteRows(book, order) {
    if (book.ids.length === 0)
        return []

    var ids = book.ids.slice().sort(function (a, b) { return order[a] - order[b] })
    var rows = [previewRow("rule", false, "", -1)]
    for (var i = 0; i < ids.length; ++i) {
        var html = order[ids[i]] + ". " + lineToHtml(book.defs[ids[i]], order)
        rows.push(previewRow("footnote", false, html, -1))
    }
    return rows
}

// The note is drawn whole: every line becomes a row carrying the line it was
// written on, so a task still flips the right box.
function previewBlocks(text) {
    var lines = String(text).split("\n")
    var book = footnotesOf(lines)
    var order = footnoteOrder(lines, book)
    var rows = []
    var at = 0
    while (at < lines.length)
        at = pushRow(rows, lines, at, order)
    return rows.concat(footnoteRows(book, order))
}