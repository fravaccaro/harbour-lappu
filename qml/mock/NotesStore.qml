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
    property int nextId: 9
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

    function bodyHtml(content) {
        return Format.toHtml(content)
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
    }

    function setFilter(kind, category) {
        filterKind = kind
        filterCategory = category || ""
        listTitle = filterTitle()
        reloadVisible()
    }

    function toggleFavorite(noteId) {
        var i = indexOfId(noteId)
        if (i < 0)
            return
        allNotes.setProperty(i, "favorite", !allNotes.get(i).favorite)
        refreshLists()
    }

    function removeNote(noteId) {
        var i = indexOfId(noteId)
        if (i < 0)
            return
        allNotes.remove(i)
        refreshLists()
    }

    function saveNote(noteId, title, content, category, favorite) {
        var fields = noteFields(noteId, title, content, category, favorite)
        var i = indexOfId(noteId)
        if (i < 0)
            allNotes.append(fields)
        else
            replaceNote(i, fields)
        refreshLists()
        return fields.noteId
    }

    function noteFields(noteId, title, content, category, favorite) {
        var id = noteId > 0 ? noteId : nextId++
        return {
            "noteId": id,
            "title": title.trim() === "" ? qsTr("New note") : title.trim(),
            "content": content,
            "category": category,
            "favorite": favorite,
            "modified": Math.floor(Date.now() / 1000),
            "locked": false
        }
    }

    function replaceNote(index, fields) {
        fields.locked = allNotes.get(index).locked
        allNotes.set(index, fields)
    }

    function addDummy(noteId, title, category, favorite, ageSec, locked, content) {
        allNotes.append({
            "noteId": noteId,
            "title": title,
            "content": content,
            "category": category,
            "favorite": favorite,
            "modified": Math.floor(Date.now() / 1000) - ageSec,
            "locked": locked
        })
    }

    function seedWork() {
        addDummy(1, "Sprint notes", "Work", true, 20 * 60, false,
                 "# Sprint notes\n\n- Ship the login flow\n- **Do not** wire sync yet\n- Read the Notes API v1 doc")
        addDummy(3, "Standup 28 Oct", "Work/Meetings", false, 26 * 3600, false,
                 "## Standup\n\nYesterday: scaffold.\nToday: UI mock.\nBlockers: none.")
        addDummy(6, "Shared handbook", "Work", false, 12 * 86400, true,
                 "This folder was shared with you as view-only.\n\nPlease leave the wording as it is.")
    }

    function seedHome() {
        addDummy(2, "Shopping", "", true, 2 * 3600, false,
                 "- Bread\n- Oat milk\n- Lemons\n- **Focaccia flour**")
        addDummy(4, "Focaccia", "Recipes", false, 3 * 86400, false,
                 "# Focaccia\n\n500g flour, 400ml water, 10g salt.\n\nSee [the Notes app](https://apps.nextcloud.com/apps/notes).")
        addDummy(5, "Wifi codes", "", false, 8 * 86400, false,
                 "Home: sea-biscuit-4\nGuest: ask at the desk")
        addDummy(7, "Tomato sauce", "Recipes", false, 30 * 86400, false,
                 "Onion, garlic, tomatoes. Simmer *slowly*.")
        addDummy(8, "Packing list", "", false, 5 * 60, false,
                 "Passport, charger, headphones.")
    }

    function seed() {
        if (allNotes.count > 0)
            return
        seedWork()
        seedHome()
        refreshLists()
    }
}
