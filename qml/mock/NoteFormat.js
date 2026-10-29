.pragma library

function escapeHtml(text) {
    return String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
}

function stripMarkdown(text) {
    var t = String(text)
    t = t.replace(/^#+\s+/gm, "")
    t = t.replace(/\*\*(.*?)\*\*/g, "$1")
    t = t.replace(/\*(.*?)\*/g, "$1")
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

function categoryLabel(category) {
    if (category === "")
        return ""
    return String(category).split("/").join(" / ")
}

function lineToHtml(line) {
    var t = escapeHtml(line)
    if (t.indexOf("### ") === 0)
        return "<b>" + t.substring(4) + "</b>"
    if (t.indexOf("## ") === 0)
        return "<b>" + t.substring(3) + "</b>"
    if (t.indexOf("# ") === 0)
        return "<b>" + t.substring(2) + "</b>"
    t = t.replace(/\*\*(.+?)\*\*/g, "<b>$1</b>")
    t = t.replace(/\*(.+?)\*/g, "<i>$1</i>")
    t = t.replace(/\[([^\]]+)\]\(([^)]+)\)/g, "<a href=\"$2\">$1</a>")
    if (t.indexOf("- ") === 0)
        return "\u2022 " + t.substring(2)
    return t
}

function toHtml(text) {
    var lines = String(text).split("\n")
    var out = []
    for (var i = 0; i < lines.length; ++i)
        out.push(lineToHtml(lines[i]))
    return out.join("<br/>")
}
