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
| `src/lappu/config.*` | Server URL, user name, TLS preference, the sort, view and sync settings and the pinned note, in `~/.config/org.frapps/harbour-lappu/settings.conf` |
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
| `qml/components/*` | Shared QML: the notification wrapper, the support dialog, the Markdown bar and one stop of it, the nudge that tells a flickable it scrolls, the edit history, the note body, a Markdown table, a code block, one side of a conflict, and the three pieces both notes layouts share — the pulley menu, the header and a note's own menu |
| `qml/cover/*` | The cover page and its glyph |
| `qml/modules/Opal/*` | Bundled Opal About, SupportMe and LinkHandler modules |

`src/main.cpp` publishes two context properties - `Lappu` (the session) and `Notes` - and the pages bind to those. It also adds `qml/modules` to the QML import path so the bundled Opal modules resolve at runtime. Networking stays in C++; QML holds no request and parses no server response.

## One writer

`Session` is the only thing that writes a note. `saveNote()`, `setFavorite()` and `deleteNote()` each write to `NotesCache` straight away, marking the row `dirty` - or `deleted` - and only then offer it to the server.

`NotesStore.qml` owns no notes of its own. It is filled through `ingest()` from `Notes.notes()` and hands every edit to `Lappu`, so the list is always a view of what the cache holds rather than a second copy that could drift from it.

`visibleNotes` is the one model a view binds to, and `syncRows()` **edits it in place**: rows are matched by `noteId`, moved and `set()` where they already exist, inserted where they do not, and the tail that is left over is removed. Clearing it and filling it again would be simpler and is what it used to do, but it destroys every delegate, and a delegate is where Silica keeps a remorse: `RemorseItem.execute()` parents itself inside the item it covers, and a destroyed countdown never runs its callback. Deleting one note redraws the list, so a clear-and-refill silently threw away the delete the user had started on a second note. Keeping the rows also keeps the view's place on every refresh.

## Sync rules

`Session::sync()` **sends what is owed before it pulls**, so a note edited on the phone is the one that stands rather than being overwritten by the older copy coming back. `pushNext()` drains `m_pending` one note at a time, choosing the verb from the row itself: a `deleted` row is a `DELETE`, a row whose id is not a server id yet is a `POST`, anything else is a `PUT`.

A run takes its queue when it starts, so a note written or deleted while one is going out is not in it. `pushDirty()` cannot start a second run against a busy session, so it sets `m_owedWhileBusy` and leaves; `flushOwed()`, at the end of `queueDrained()` and `onNotesReady()`, sends what was marked rather than leaving it for the next sync. Only an edit of the user's sets that flag, which is what keeps a note the server keeps refusing from earning a run of its own on every drain: such a row stays `dirty` and waits, exactly as before.

The answer to a write is taken against **the note that was sent**, held in `m_inFlight`, and not against the row as it stands now. `saveNote()` writes to the cache whatever is in flight, so a second tick landing while the first `PUT` is still travelling leaves the row holding an edit the server has not been told about. `settleWritten()` compares the row with `m_inFlight`: where they still agree it settles the row as before, and where they do not - a newer edit, or a delete made meanwhile - it calls `keepOwedOver()`, which adopts the server's id and ETag but leaves the row `dirty` so `flushOwed()` has something to send and the next write carries a version the server will accept. Settling blindly would both lose the newer edit and clear the mark that was to send it, which is the one way a note can be dropped without anything going wrong.

A `PUT` answered `404` means the server **no longer has the note**, which `noteIsGone()` reports on `deleted()` along with a `DELETE` that succeeded. `Session::onDeleted()` tells the two apart by the row: one marked `deleted` is a delete the user asked for and is simply removed, while one still `dirty` is a note that was taken away under an edit that was never offered to anybody. `restartOwed()` keeps that edit as a note the server has never seen - a fresh temporary id, no ETag, appended to `m_pending` so it goes out in the same run - carrying the title unchanged, since there is no second copy for it to be told apart from, and says so through `notice()`. The cover's pin follows onto the new id, and again onto the server's when `onCreated()` arrives.

A `PUT` is **conditional**. `NotesApi::updateNote()` sends the stored ETag as `If-Match`, wrapped in the double quotes the server compares against - the JSON `etag` attribute arrives bare, and a bare header would be refused by every write. An empty ETag sends no header at all, so a server too old to hand one out is written to unconditionally as before. A `POST` has nothing to match, and a `DELETE` is not checked server-side, so neither carries the header.

A refused write answers `412` with the server's whole note as the body, which `NotesApi::reportConflict()` parses and reports on `conflicted()` rather than `failed()`: the server is healthy and nothing is lost, so `Session::onConflict()` ends in `pushNext()` like a success and leaves `m_pushed` alone. It settles the note by itself where it can - a row that is gone or already clean, a row marked `deleted` whose `DELETE` will go out unconditionally, or two sides that agree on title, content, category and favourite - and otherwise records the pair in `m_conflicts` and says so through `notice()`. `pushNext()` skips any id in that list, or every run would earn the same `412` again and add a second pair.

`m_conflicts` is held in memory alone. The local side of a pair is the `dirty` row in the cache, so an app closed before the user answers loses only the question, and the next run's `412` asks it again. The three resolvers each settle the original row: `resolveKeepMine()` adopts the server's ETag through `NotesCache::setEtag()` and leaves the row `dirty` for another conditional write, `resolveKeepTheirs()` calls `markClean()` with the server's note, and `resolveKeepBoth()` writes the local text to a fresh negative id through `m_cache.upsertLocal()` - never through `store()`, which would re-enter the push - and `markClean()`s the original with the server's note. A branch that left the original `dirty` with its stale ETag would fork another copy on every sync, so settling it is what the feature rests on. The cover keeps its pin on the original id: that is the note the user pinned, and the server's version is the one that goes on holding it.

A write refused by any other status takes a third route. `NotesApi::reportRefusal()` reports it on `refused()` rather than `failed()`, because a server that answers with a status of its own is plainly working: `Session::onRefused()` ends in `pushNext()`, so the refusal costs that one note its turn and not the whole queue. Nothing is thrown away either - the row stays `dirty` and is offered again by the next run. `401` is the exception and still leaves on `unauthorized()`, since it is about the account rather than the note and nothing else owed would fare any better.

`refused(int status, const QString &error)` carries the status because **naming the note is the caller's to do**. `NotesApi` knows neither which note was sent nor what it is called; `Session` holds it in `m_inFlight`, so `refusalNotice()` builds the wording there and `statusError()` keeps no `423` branch at all - a `GET` of the list is never answered with one, so the only caller left for that sentence is the session. `403` stays in `statusError()` as the fallback for a fetch.

The refusal is **always said**, though, and said through `notice()` rather than `errorMessage`: the edit was the user's own, so a tick that did not land is worth knowing about whether or not the sync was asked for, and the `if (error == m_error) return;` guard would otherwise hold back the second of two identical refusals - which is exactly what ticking a box on a locked note twice earns. A push the user did not ask for used to say nothing at all, which is how a note came to carry a change the server had refused without a word about it anywhere.

The status worth naming is **423**. The Notes app answers it for a locked file, through `\OCP\Lock\LockedException`, and only after `Util::retryIfLocked()` has tried five times itself; read-only is **403**, by way of `NoteNotWritableException`.

Two kinds of lock arrive as the same status. A transactional lock is the one Nextcloud takes for the length of a write, and it expires. A `ManuallyLockedException` from the `files_lock` app is one a user placed - the **Lock file** entry in the Files app - and it has no expiry at all: it stands until somebody unlocks the file, and until then no client can write to it. The message therefore says the note is *locked* rather than in use, because that is the word that leads the user to the Files app, and adds that the change is waiting, which is the part that needs nothing further from them.

A message is read once, so the notes being held shut are gathered as well as announced. `heldShut()` picks out the two statuses that hold a note rather than going wrong the once - `423` and `403` - and `rememberLocked()` puts the id of the note in flight into `m_locked` against the status it earned, behind `lockedCount` and `lockedNotes()`. The id rather than the note, so the rows handed to QML are read out of the cache and no name goes stale.

`m_locked` is kept in memory alone, for the reason `m_conflicts` is: the row is still `dirty`, so the next run earns the same refusal and makes the list again. Emptying it is one rule rather than a removal at each of the places a note stops being held - `pruneLocked()`, called from `reloadModels()`, drops every entry whose row owes the server nothing, which covers the write that landed, the change discarded and the note deleted alike. `logout()` clears it by hand, since it empties the cache without going through a reload. `LockedNotesPage` lists them with the reason for each and says where a lock is lifted, and a row in `NotesListHeader` is the only way in: as with conflicts, a page arriving over the user's work during a refresh would be an ambush.

One word, one meaning: `NotesModel` hands QML the note's own permission as `readOnly`, named as `LocalNote::readOnly` is, while `lockedCount` counts notes whose *change* the server would not take. The first is a property of the note, the second of what is owed on it.

The row keeps waiting rather than being retried harder or given up on. There is nothing the app can do about a lock from this side, and **a file nobody can write is also a file nobody can change**: the only version an owed row keeps `applyRemote()` from fetching is the one the edit was made from, so waiting costs the user nothing and dropping the edit would cost them the work. The server names the holder in its log:

```
ManuallyLockedException "files/Notes/Home/Weekly meal plan.md" is locked,
existing lock on file: files_lock/f989125b-...
```

`Session::discardLocal()` is the way out for a user who would rather have the note back than go on waiting - a change made offline, one thought better of, or one a lock is holding indefinitely. `abandonOwed()` drops any question recorded for the note, calls `NotesCache::abandonOwed()` to clear `dirty` and `deleted` without settling the row against any version, and sets `m_pullAfter`; `discardLocal()` then pulls itself when nothing is in flight, there being no drain to wait for. It refuses a note the server has never seen - there is no version to fall back on, so letting go of the edit would be letting go of the note - and a row that owes nothing, there being nothing to discard.

The pages offer it as **Discard waiting change**, shown only where `dirty` reaches QML through `NotesModel`, and `NotePage` carries a line of its own saying a change has not reached the server. That line is what keeps waiting from reading as breakage: it is hidden while `busy`, or every ordinary save would flash it on the way through. Without it the note looked untouched while holding a change the server had refused, which is what sent the user to the conflict page for an answer.

A pull is the other place an owed row is reconsidered. `applyRemote()` asks `stillOwed()` rather than reading `dirty` straight: a row whose title, content, category and favourite already match what the server sent is a write that **landed and whose answer never arrived** - the watchdog cut the reply off, or the network dropped between the server writing and the client hearing - and taking the server's note settles it. Left owed it would be offered again against an ETag that has since moved on and earn a `412` the user would have to answer for no reason. A `deleted` row is never settled this way: the note still being listed is the whole of what is owed.

Two paths lead into the push, and they differ in how loudly they fail:

* `sync()` sets `m_announce` and `m_pullAfter`, so it reports a server it could not reach and pulls afterwards.
* `pushDirty()`, which runs whenever credentials are applied, clears both. An owed note is offered as soon as it is made, but a flush the user did not ask for stays quiet and leaves the note waiting.

`queueDrained()` decides whether to pull: either the run asked for one, or something was actually pushed and **Refresh after saving a note** is on, or a write was refused as out of date, which is proof the server holds something newer. A push that sent nothing cannot otherwise trigger a pull, which is what stops a note the server keeps refusing from driving an endless loop; the conflict clause is the one exception, and it stays bounded at one pull per run because a pull carries no ETag and so cannot itself be refused.

## Automatic refresh

`applySyncTimer()` holds a `QTimer` that only ticks while the app runs; nothing in Sailfish OS wakes Lappu for a refresh. The timer is restarted at the end of `onNotesReady()` and `onApiFailed()`, so the interval is measured from the last run rather than from when it was set.

The startup refresh is behind `syncIfStartupWanted()` and its `m_syncedOnStart` latch, because `applyCredentials()` runs again whenever credentials are re-applied and would otherwise pull more than once. A fresh sign-in is the exception: `onLoggedIn()` sets the latch and syncs regardless of the setting, since a new account has nothing to show until it does.

## The title is the note's name

A note's title is its **file name**, held apart from the content, which carries the whole document. The Notes **v1** API does not work the title out from the content the way the deprecated v0.2 one did: a write that carries no title has the server fall back to "New note", so the client owns the name. `bodyFor()` sends it with every `POST` and `PUT`; the server sanitises it (it is a file name, and a duplicate gains a sequential number) and hands the result back, which the cache adopts through `markClean()`, so the name a page shows before the first pull is confirmed or corrected by it.

`bodyFor()` also sends the `modified` stamp. The server bumps the file's own time on a content change but not on a title- or category-only edit, so leaving it out would have such an edit keep its old time and drop back down a list sorted by it, moments after the local save had lifted it to the top. `Session::saveNote()` stamps the edit with the current time and the server adopts what it is given; `setFavorite()` leaves the stored time be, since favourites float to the top of their own accord.

The name is the writer's. `NoteEditPage` gives it a field of its own above the body, and `chosenTitle()` in `session.cpp` saves what was typed verbatim, so **nothing renames a note but the writer** - a name set on the server or in Files outlives an edit made on the phone. `noteTitle()` survives only as the fallback for an empty field, naming the note after the first line of its content and taking off the markers that open a heading or a list item, as the server's own clients do, so a note jotted down in a hurry is still called something.

Everything else that writes a note hands the stored name back untouched. `NotesStore.toggleTask()` passes `note.title` along with the flipped content, because a save is the only way to tick a box and a derived title would quietly rename the note; `setFavorite()` and `deleteNote()` reuse the stored row and never touch it.

Because the content is the whole document, its first line is ordinary text and is drawn like any other, including a first line that repeats the title: `previewBlocks()` on the note page, `excerpt()` in the rows and cards, and `plainText()` on the cover all begin at the top, so every row carries its true `lineIndex` and a task flips the right box. The name shown above the body - by `NotePage` and the cover - is the file's, kept apart from the text, so a note that opens by repeating its name simply shows that line both places.

A note that has not reached the server borrows a **negative id** until the server names it. That id is what the cover may be holding, so `onCreated()` remaps it: if `coverNoteId` is the local id being replaced, it is rewritten to the id the server gave. Without that, pinning a brand-new note would leave the cover blank after its first sync.

Such an id is an ordinary id everywhere else, and treating it as a stand-in for "no note" is a mistake worth guarding against. Only `0` means no note: `saveNote()` starts a new note for `0` alone, so that editing an unsent one finds its row instead of forging another, and the cover counts any non-zero id as a pin.

## Messages to the user

A passing message has one route: `app.showToast()` in `qml/harbour-lappu.qml` hands it to the single `LappuNotification`, which publishes a transient notice. Anything the session reports through `errorMessage` arrives that way, as does a note that is no longer there. No page carries its own copy of `errorMessage`.

Something the user is owed but that is not an error travels on `notice()`, a plain signal, instead. `errorMessage` is a property, and its `if (error == m_error) return;` guard would hold back the second of two identical notices; it also reads as a failure, which a refused write is not. Like `unauthorized`, a notice is said whether or not the sync was asked for, because a change the user can see needs a reason.

A failed sign-in is the exception: `loginFailed` reaches the sign-in pages, which say so in place, because the reason belongs to the form the user is looking at. That is why `onLoginFailed()` in `session.cpp` does not also set `errorMessage`. A sign-in that simply runs out of time takes the same route: `LoginFlow::onDeadline()` cancels the poll and fails with a reason.

## Drawing a note

`NoteFormat.previewBlocks()` turns the content into the rows `NoteBody` draws. Most rows are one line, but a table and a fenced code block are each a single row standing for every line they were written on, so **a row is not a line** and each one carries the `lineIndex` it began at. `taskClicked` reports that number rather than the row, because `NotesStore.flipLine()` indexes the content by line and a row number would tick the wrong box under any block.

`blockAt()` asks about a fence before a table. Inside a fence nothing is Markdown: a row of pipes is code, a hash is a hash, and `MarkdownCode` draws the lines as `Text.PlainText`, which is both why nothing needs escaping there and why nothing can be read by mistake. The panel takes a `ScrollHint` like the grid and the Markdown bar, since a line too wide to show is the one thing a block that does not wrap has to admit to. An unclosed fence takes the rest of the note, as it does everywhere else.

Inside a line the same rule holds one level down. `richText()` splits the backquoted spans out first and marks up only what lies between them, so the asterisks of a snippet are not read as emphasis:

```js
var parts = String(t).split(/(`[^`]+`)/)
```

A line is then known by what it begins with, and the order of those questions matters: a task before a bullet, since a task is also a bullet, and a quote before either, since a quote may hold one. What marks a task is the brackets rather than the bullet — `-`, `*` and `+` all open a list item, which is what the server's own renderer accepts — and `NotesStore.flipTaskLine()` replaces only the brackets, so the bullet a note was written with survives a tick.

A heading row carries its `level` and a nested row its `indent`; neither arrives pre-marked, because size, weight and offset belong to the drawing. That is also why a bullet is a `text` row with a glyph in front of it rather than a kind of its own.

What makes a table a table is the row of dashes directly beneath its header, with one cell per column. That test is the whole of it, and it is deliberately strict: a sentence carrying a stray `|` has to keep reading as a sentence. A cell may hold inline Markdown, and `lineToHtml()` escapes it first, since a note may have angle brackets of its own; a cell keeps its links whole, unlike a task line, whose row is itself the tap target.

The row carries the Markdown of the block rather than a model of cells, and `MarkdownTable` parses it again where it is drawn. Nothing between the two has to agree on a shape, and the component can be handed any block at all: it draws nothing for one that is not a table.

Columns are measured, which QML offers no declarative way to do. `MarkdownTable.remeasure()` runs on a change of content or width and assigns `columnWidths`; the measuring is done in the header's **bold** so a column cannot clip the name above it, by a hidden `Label` rather than a `TextMetrics`, which keeps the ruler in the very font the cells are drawn in. No column is allowed past the width of the note, so one long cell wraps instead of pushing the rest out of reach, and the grid flicks sideways when the columns together want more room than the page has. A grid that scrolls does not say so while it sits still, so it is nudged once when it is built, by the same `ScrollHint` the Markdown bar uses - started after `remeasure()`, since until the columns have widths there is no telling whether anything is hidden, and capped at what is actually hidden, so a table the note has room for does not move. `NoteBody` puts the grid behind a `Loader`, unlike its other rows, since a note of a hundred lines should not build a hundred of them.

### Footnotes

A footnote is the one mark whose meaning is not local: `[^1]` says nothing without the `[^1]: ...` line that answers it, and that line may be written anywhere in the note. So `previewBlocks()` reads the note twice before it draws anything. `footnotesOf()` collects the definitions, stepping over a fence whole - inside one a definition is code like any other line - and `footnoteOrder()` hands out the numbers by walking the body and taking each defined reference the first time it is met.

The number, not the label, is what the reader sees. A note referring to `[^1]`, `[^bignote]` and `[^comment-1]` in that order draws 1, 2 and 3, which is how Nextcloud Text and Obsidian both behave and what keeps a hand-numbered note from reading out of order. A reference the note never defines points at nothing and is left exactly as it was written; a definition the body never refers to is still shown, numbered after the rest. The comment threads Nextcloud writes as `[^comment-1]` are footnotes like any other here: Lappu has no comment UI to give them, and the syntax is already readable as a footnote.

The numbering travels as a parameter rather than as state, so `lineToHtml()`, `richText()` and `markedUp()` all take an `order` and a caller with none leaves references alone. That is what `MarkdownTable` does, which is correct twice over: a footnote may not appear inside a table, and the cells are parsed again in a second pass that knows nothing of the note around them.

Definitions are skipped **in place** rather than cut out of the text. `pushRow()` counts the lines a definition took and draws nothing for them, so every row after it still carries the line it was written on and a task still flips the right box. The notes themselves are appended by `footnoteRows()` as `kind: "footnote"` rows ordered by number, opened by a `kind: "rule"` row that `NoteBody` draws as a Silica `Separator`. The parting is a line rather than a gap because a note with blank lines of its own would not show one. All of these stand for no line of the note, so they carry `-1`.

A reference is drawn as `<sup><a href="footnote:id">`, which makes it a link Qt will offer like any other. `NoteBody.followLink()` swallows anything addressed `footnote:` instead of re-emitting it, because the target is inside the note: nothing is opened and `Opal.LinkHandler` never sees it. An id is matched as `[^\]\s&<>"]+`, excluding the characters escaping would rewrite, so the label read in a definition is the same label read in the body.

List excerpts and the cover have no room for any of this, so `stripMarkdown()` takes the markers out: `stripBlocks()` for what marks a whole line - the dashes of a table, a fence, a `>`, a bullet, a task's brackets, the label opening a footnote - and `stripSpans()` for what marks a run of text inside one. A table row gives up its pipes only when the line is fenced by them, which again leaves prose alone. A footnote reference goes entirely rather than becoming its number: there is no list under an excerpt for a number to point at, while what the footnote says is kept, since it is still part of the note.

An excerpt is the note as written: `excerpt()` strips the markup from the whole content, so a note whose first line repeats its name shows that line here too. The cover reads the same way through `plainText()`.

## Links

No link is handed to the browser by the app. `Opal.LinkHandler` shows it first, which matters most for the links inside a note, since those came from the server rather than from the app. `NoteBody` only re-emits `linkActivated`, so the page that holds it decides, and both the page and the component's own example call `LinkHandler.openOrCopyUrl()`.

## Sandbox layout

Sailjail gives the app three private directories, named after the `OrganizationName` and `ApplicationName` of the desktop file:

* `~/.config/org.frapps/harbour-lappu` - `settings.conf`
* `~/.local/share/org.frapps/harbour-lappu` - `notes.sqlite`
* `~/.cache/org.frapps/harbour-lappu` - whatever the web view leaves behind; nothing the app writes itself

The permissions are `Internet`, `Secrets` and `WebView`; no document or picture directory is asked for. The config and share directories are declared for the system backup through `X-HarbourBackup`.
