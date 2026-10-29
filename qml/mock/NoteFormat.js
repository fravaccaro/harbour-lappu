.pragma library

function escapeHtml(text) {
    return String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
}

function stripMarkdown(text) {
    var t = String(text)
    t = t.replace(/^#+\s+/gm, "")
    t = t.replace(/\*\*(.*?)\*\*/g, "$1")
    t = t.replace(/\*(.*?)\*/g, "$1")
    t = t.replace(/~~(.*?)~~/g, "$1")
    t = t.replace(/<\/?u>/g, "")
    t = t.replace(/`([^`]+)`/g, "$1")
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
    for (var i = 0; i < lines.length; ++i)
        out.push(stripMarkdown(lines[i]))
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

function inlineHtml(t) {
    t = t.replace(/~~(.+?)~~/g, "<s>$1</s>")
    t = t.replace(/&lt;u&gt;(.+?)&lt;\/u&gt;/g, "<u>$1</u>")
    t = t.replace(/\*\*(.+?)\*\*/g, "<b>$1</b>")
    t = t.replace(/\*(.+?)\*/g, "<i>$1</i>")
    t = t.replace(/\[([^\]]+)\]\(([^)]+)\)/g, "<a href=\"$2\">$1</a>")
    if (t.indexOf("- ") === 0)
        return "\u2022 " + t.substring(2)
    return t
}

function lineToHtml(line) {
    var t = escapeHtml(line)
    var heading = headingHtml(t)
    return heading !== "" ? heading : inlineHtml(t)
}

function toHtml(text) {
    var lines = String(text).split("\n")
    var out = []
    for (var i = 0; i < lines.length; ++i)
        out.push(lineToHtml(lines[i]))
    return out.join("<br/>")
}
