import QtQuick 2.0
import "NoteFormat.js" as Format

QtObject {
    id: store

    property string filterKind: "all"
    property string filterCategory: ""
    property string searchText: ""
    property string sortMode: "modified"
    property bool busy: false
    onSearchTextChanged: reloadVisible()
    onSortModeChanged: reloadVisible()
    property int pinnedNoteId: 0
    onPinnedNoteIdChanged: applyPinned()
    // Kept as plain properties so the cover follows edits to the pinned note.
    property string pinnedTitle: ""
    property string pinnedText: ""
    property int favoriteCount: 0
    property int uncategorizedCount: 0
    property string listTitle: qsTr("All")
    property ListModel allNotes: ListModel {
    }
    property ListModel visibleNotes: ListModel {
    }
    property ListModel categories: ListModel {
    }

    function categoryLabel(category) {
        return Format.categoryLabel(category)
    }

    function filterTitle() {
        if (filterKind === "favorites")
            return qsTr("Favorites")
        if (filterKind === "uncategorized")
            return qsTr("Uncategorized")
        if (filterKind === "category")
            return categoryLabel(filterCategory)
        return qsTr("All")
    }

    function relativeTime(modified) {
        var sec = Math.floor(Date.now() / 1000) - modified
        if (sec < 45)
            return qsTr("Just now")
        if (sec < 3600)
            return qsTr("%n minute(s) ago", "", Math.max(1, Math.floor(sec / 60)))
        if (sec < 86400)
            return qsTr("%n hour(s) ago", "", Math.max(1, Math.floor(sec / 3600)))
        if (sec < 172800)
            return qsTr("Yesterday")
        return qsTr("%n day(s) ago", "", Math.floor(sec / 86400))
    }

    function indexOfId(noteId) {
        for (var i = 0; i < allNotes.count; ++i) {
            if (allNotes.get(i).noteId === noteId)
                return i
        }
        return -1
    }

    function noteById(noteId) {
        var i = indexOfId(noteId)
        return i < 0 ? null : allNotes.get(i)
    }

    function countFavorites() {
        return countWhere(function(n) {
            return n.favorite
        })
    }

    function countUncategorized() {
        return countWhere(function(n) {
            return n.category === ""
        })
    }

    function countWhere(match) {
        var n = 0
        for (var i = 0; i < allNotes.count; ++i) {
            if (match(allNotes.get(i)))
                n++
        }
        return n
    }

    function countCategory(name) {
        return countWhere(function(n) {
            return n.category === name
        })
    }

    function noteMatches(note) {
        if (filterKind === "favorites" && !note.favorite)
            return false
        if (filterKind === "uncategorized" && note.category !== "")
            return false
        if (filterKind === "category" && note.category !== filterCategory)
            return false
        return searchHits(note)
    }

    function searchHits(note) {
        if (searchText === "")
            return true
        var q = searchText.toLowerCase()
        var hay = (note.title + " " + note.content).toLowerCase()
        return hay.indexOf(q) >= 0
    }

    function compareNotes(a, b) {
        if (a.favorite !== b.favorite)
            return a.favorite ? -1 : 1
        if (sortMode === "title")
            return compareTitles(a, b)
        return b.modified - a.modified
    }

    function compareTitles(a, b) {
        var ta = a.title.toLowerCase()
        var tb = b.title.toLowerCase()
        if (ta < tb)
            return -1
        if (ta > tb)
            return 1
        return 0
    }

    function snapshot(note) {
        return {
            "noteId": note.noteId,
            "title": note.title,
            "content": note.content,
            "category": note.category,
            "favorite": note.favorite,
            "modified": note.modified,
            "locked": note.locked,
            "excerpt": Format.excerpt(note.content),
            "categoryLabel": Format.categoryLabel(note.category)
        }
    }

    function matchingRows() {
        var rows = []
        for (var i = 0; i < allNotes.count; ++i) {
            var note = allNotes.get(i)
            if (noteMatches(note))
                rows.push(snapshot(note))
        }
        rows.sort(compareNotes)
        return rows
    }

    function reloadVisible() {
        var rows = matchingRows()
        visibleNotes.clear()
        for (var i = 0; i < rows.length; ++i)
            visibleNotes.append(rows[i])
    }

    function namedCategories(map) {
        var names = []
        for (var key in map)
            names.push(key)
        names.sort()
        return names
    }

    function rebuildCategories() {
        var map = categoryCounts()
        var names = namedCategories(map)
        categories.clear()
        for (var i = 0; i < names.length; ++i)
            categories.append({
                "name": names[i],
                "count": map[names[i]]
            })
    }

    function categoryCounts() {
        var map = {}
        for (var i = 0; i < allNotes.count; ++i) {
            var cat = allNotes.get(i).category
            if (cat !== "")
                map[cat] = (map[cat] || 0) + 1
        }
        return map
    }

    function categoryNames() {
        var names = []
        for (var i = 0; i < categories.count; ++i)
            names.push(categories.get(i).name)
        return names
    }

    function refreshLists() {
        rebuildCategories()
        reloadVisible()
        favoriteCount = countFavorites()
        uncategorizedCount = countUncategorized()
        applyPinned()
    }

    function setFilter(kind, category) {
        filterKind = kind
        filterCategory = category || ""
        listTitle = filterTitle()
        reloadVisible()
    }

    // Only one note sits on the cover, so pinning another replaces the pin.
    function pinNote(noteId) {
        pinnedNoteId = noteId
    }

    function unpinNote() {
        pinnedNoteId = 0
    }

    function pinnedNote() {
        return pinnedNoteId === 0 ? null : noteById(pinnedNoteId)
    }

    function applyPinned() {
        var note = pinnedNote()
        pinnedTitle = note ? note.title : ""
        pinnedText = note ? Format.plainText(note.content) : ""
    }

    function toggleTask(noteId, lineIndex) {
        var note = noteById(noteId)
        if (!note || note.locked)
            return
        var content = flipLine(note.content, lineIndex)
        if (content !== null)
            saveNote(noteId, content, note.category, note.favorite)
    }

    // Returns null when that line carries no task marker, so the caller can
    // leave the note untouched.
    function flipLine(content, lineIndex) {
        var lines = String(content).split("\n")
        if (lineIndex < 0 || lineIndex >= lines.length)
            return null
        var flipped = flipTaskLine(lines[lineIndex])
        if (flipped === null)
            return null
        lines[lineIndex] = flipped
        return lines.join("\n")
    }

    function flipTaskLine(line) {
        var task = Format.parseTask(line)
        if (!task)
            return null
        return String(line).replace(/\[[ xX]\]/, task.done ? "[ ]" : "[x]")
    }

    // The session owns the notes, so an edit is handed to it and comes back
    // through ingest(); nothing is written into allNotes here.
    function toggleFavorite(noteId) {
        var note = noteById(noteId)
        if (note)
            Muistio.setFavorite(noteId, !note.favorite)
    }

    function removeNote(noteId) {
        if (noteId === pinnedNoteId)
            unpinNote()

        Muistio.deleteNote(noteId)
    }

    function saveNote(noteId, content, category, favorite) {
        Muistio.saveNote(noteId, content, category, favorite)
    }

    // Takes the notes the session keeps, each already keyed the way the rows
    // of allNotes are named.
    function ingest(list) {
        allNotes.clear()
        for (var i = 0; i < list.length; ++i)
            allNotes.append(list[i])

        refreshLists()
    }
}
