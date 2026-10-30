---
layout: default
title: Architecture
parent: Developers
nav_order: 1
---

# Architecture

## Source map

| Part | Purpose |
|---|---|
| `src/lappu/config.*` | Server URL, user name, TLS preference, the sort and sync settings and the pinned note, in `~/.config/org.frapps/harbour-lappu/settings.conf` |
| `src/lappu/secretsstore.*` | The app password in a device-lock collection of Sailfish Secrets |
| `src/lappu/loginflow.*` | Nextcloud login flow v2: ask for a login page, then poll until the server hands over an app password or twenty minutes pass |
| `src/lappu/httpclient.*` | `QNetworkAccessManager` wrapper: basic auth, custom methods, TLS preference |
| `src/lappu/notesapi.*` | Nextcloud Notes API v1 client under `/index.php/apps/notes/api/v1/notes` |
| `src/lappu/notescache.*` | SQLite copy of the notes, in `notes.sqlite` |
| `src/lappu/notesmodel.*` | Holds the notes of the last pull, and hands them to QML as plain rows through `notes()` |
| `src/lappu/session.*` | QML-facing controller: login, sync, save, favourite, delete |
| `src/lappu/url.*` | Server URL normalisation |
| `src/lappu/types.h` | Plain data structs (`LocalNote`) |
| `qml/pages/*` | Silica UI |
| `qml/pages/ConflictPage.qml` | The two versions of a note that changed on both sides, and the three answers to it |
| `qml/notes/NotesStore.qml` | The QML-side view of the synced notes: filtering, search, sort, category and cover state |
| `qml/notes/NoteFormat.js` | Markdown helpers: excerpt, plain text, and the subset of HTML a Silica `Label` draws |
| `qml/components/*` | Shared QML: the notification wrapper, the support dialog, the Markdown bar, the edit history, the note body, a Markdown table, one side of a conflict |
| `qml/cover/*` | The cover page and its glyph |
| `qml/modules/Opal/*` | Bundled Opal About, SupportMe and LinkHandler modules |

`src/main.cpp` publishes two context properties - `Lappu` (the session) and `Notes` - and the pages bind to those. It also adds `qml/modules` to the QML import path so the bundled Opal modules resolve at runtime. Networking stays in C++; QML holds no request and parses no server response.

## One writer

`Session` is the only thing that writes a note. `saveNote()`, `setFavorite()` and `deleteNote()` each write to `NotesCache` straight away, marking the row `dirty` - or `deleted` - and only then offer it to the server.

`NotesStore.qml` owns no notes of its own. It is filled through `ingest()` from `Notes.notes()` and hands every edit to `Lappu`, so the list is always a view of what the cache holds rather than a second copy that could drift from it.

## Sync rules

`Session::sync()` **sends what is owed before it pulls**, so a note edited on the phone is the one that stands rather than being overwritten by the older copy coming back. `pushNext()` drains `m_pending` one note at a time, choosing the verb from the row itself: a `deleted` row is a `DELETE`, a row whose id is not a server id yet is a `POST`, anything else is a `PUT`.

A `PUT` is **conditional**. `NotesApi::updateNote()` sends the stored ETag as `If-Match`, wrapped in the double quotes the server compares against - the JSON `etag` attribute arrives bare, and a bare header would be refused by every write. An empty ETag sends no header at all, so a server too old to hand one out is written to unconditionally as before. A `POST` has nothing to match, and a `DELETE` is not checked server-side, so neither carries the header.

A refused write answers `412` with the server's whole note as the body, which `NotesApi::reportConflict()` parses and reports on `conflicted()` rather than `failed()`: the server is healthy and nothing is lost, so `Session::onConflict()` ends in `pushNext()` like a success and leaves `m_pushed` alone. It settles the note by itself where it can - a row that is gone or already clean, a row marked `deleted` whose `DELETE` will go out unconditionally, or two sides that agree on content, category and favourite - and otherwise records the pair in `m_conflicts` and says so through `notice()`. `pushNext()` skips any id in that list, or every run would earn the same `412` again and add a second pair.

`m_conflicts` is held in memory alone. The local side of a pair is the `dirty` row in the cache, so an app closed before the user answers loses only the question, and the next run's `412` asks it again. The three resolvers each settle the original row: `resolveKeepMine()` adopts the server's ETag through `NotesCache::setEtag()` and leaves the row `dirty` for another conditional write, `resolveKeepTheirs()` calls `markClean()` with the server's note, and `resolveKeepBoth()` writes the local text to a fresh negative id through `m_cache.upsertLocal()` - never through `store()`, which would re-enter the push - and `markClean()`s the original with the server's note. A branch that left the original `dirty` with its stale ETag would fork another copy on every sync, so settling it is what the feature rests on. The cover keeps its pin on the original id: that is the note the user pinned, and the server's version is the one that goes on holding it.

Two paths lead into the push, and they differ in how loudly they fail:

* `sync()` sets `m_announce` and `m_pullAfter`, so it reports a server it could not reach and pulls afterwards.
* `pushDirty()`, which runs whenever credentials are applied, clears both. An owed note is offered as soon as it is made, but a flush the user did not ask for stays quiet and leaves the note waiting.

`queueDrained()` decides whether to pull: either the run asked for one, or something was actually pushed and **Refresh after saving a note** is on, or a write was refused as out of date, which is proof the server holds something newer. A push that sent nothing cannot otherwise trigger a pull, which is what stops a note the server keeps refusing from driving an endless loop; the conflict clause is the one exception, and it stays bounded at one pull per run because a pull carries no ETag and so cannot itself be refused.

## Automatic refresh

`applySyncTimer()` holds a `QTimer` that only ticks while the app runs; nothing in Sailfish OS wakes Lappu for a refresh. The timer is restarted at the end of `onNotesReady()` and `onApiFailed()`, so the interval is measured from the last run rather than from when it was set.

The startup refresh is behind `syncIfStartupWanted()` and its `m_syncedOnStart` latch, because `applyCredentials()` runs again whenever credentials are re-applied and would otherwise pull more than once. A fresh sign-in is the exception: `onLoggedIn()` sets the latch and syncs regardless of the setting, since a new account has nothing to show until it does.

## A note has no title

The server names a note after the first line of its content. `noteTitle()` in `session.cpp` works the same name out locally, so a page can show it before the next pull confirms it.

A note that has not reached the server borrows a **negative id** until the server names it. That id is what the cover may be holding, so `onCreated()` remaps it: if `coverNoteId` is the local id being replaced, it is rewritten to the id the server gave. Without that, pinning a brand-new note would leave the cover blank after its first sync.

Such an id is an ordinary id everywhere else, and treating it as a stand-in for "no note" is a mistake worth guarding against. Only `0` means no note: `saveNote()` starts a new note for `0` alone, so that editing an unsent one finds its row instead of forging another, and the cover counts any non-zero id as a pin.

## Messages to the user

A passing message has one route: `app.showToast()` in `qml/harbour-lappu.qml` hands it to the single `LappuNotification`, which publishes a transient notice. Anything the session reports through `errorMessage` arrives that way, as does a note that is no longer there. No page carries its own copy of `errorMessage`.

Something the user is owed but that is not an error travels on `notice()`, a plain signal, instead. `errorMessage` is a property, and its `if (error == m_error) return;` guard would hold back the second of two identical notices; it also reads as a failure, which a refused write is not. Like `unauthorized`, a notice is said whether or not the sync was asked for, because a change the user can see needs a reason.

A failed sign-in is the exception: `loginFailed` reaches the sign-in pages, which say so in place, because the reason belongs to the form the user is looking at. That is why `onLoginFailed()` in `session.cpp` does not also set `errorMessage`. A sign-in that simply runs out of time takes the same route: `LoginFlow::onDeadline()` cancels the poll and fails with a reason.

## Drawing a note

`NoteFormat.previewBlocks()` turns the content into the rows `NoteBody` draws. Most rows are one line, but a table is a single row standing for every line it was written on, so **a row is not a line** and each one carries the `lineIndex` it began at. `taskClicked` reports that number rather than the row, because `NotesStore.flipLine()` indexes the content by line and a row number would tick the wrong box under any table.

What makes a table a table is the row of dashes directly beneath its header, with one cell per column. That test is the whole of it, and it is deliberately strict: a sentence carrying a stray `|` has to keep reading as a sentence. A cell may hold inline Markdown, and `cellHtml()` escapes it first, since a note may have angle brackets of its own; a cell keeps its links whole, unlike a task line, whose row is itself the tap target.

The row carries the Markdown of the block rather than a model of cells, and `MarkdownTable` parses it again where it is drawn. Nothing between the two has to agree on a shape, and the component can be handed any block at all: it draws nothing for one that is not a table.

Columns are measured, which QML offers no declarative way to do. `MarkdownTable.remeasure()` runs on a change of content or width and assigns `columnWidths`; the measuring is done in the header's **bold** so a column cannot clip the name above it, by a hidden `Label` rather than a `TextMetrics`, which keeps the ruler in the very font the cells are drawn in. No column is allowed past the width of the note, so one long cell wraps instead of pushing the rest out of reach, and the grid flicks sideways when the columns together want more room than the page has. `NoteBody` puts the grid behind a `Loader`, unlike its other rows, since a note of a hundred lines should not build a hundred of them.

List excerpts and the cover have no room for a grid, so `stripMarkdown()` drops the row of dashes and turns a row's pipes into spaces. It only does that to a line fenced by pipes, which again leaves prose alone.

## Links

No link is handed to the browser by the app. `Opal.LinkHandler` shows it first, which matters most for the links inside a note, since those came from the server rather than from the app. `NoteBody` only re-emits `linkActivated`, so the page that holds it decides, and both the page and the component's own example call `LinkHandler.openOrCopyUrl()`.

## Sandbox layout

Sailjail gives the app three private directories, named after the `OrganizationName` and `ApplicationName` of the desktop file:

* `~/.config/org.frapps/harbour-lappu` - `settings.conf`
* `~/.local/share/org.frapps/harbour-lappu` - `notes.sqlite`
* `~/.cache/org.frapps/harbour-lappu` - whatever the web view leaves behind; nothing the app writes itself

The permissions are `Internet`, `Secrets` and `WebView`; no document or picture directory is asked for. The config and share directories are declared for the system backup through `X-HarbourBackup`.
