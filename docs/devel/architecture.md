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
| `qml/notes/NotesStore.qml` | The QML-side view of the synced notes: filtering, search, sort, category and cover state |
| `qml/notes/NoteFormat.js` | Markdown helpers: excerpt, plain text, and the subset of HTML a Silica `Label` draws |
| `qml/components/*` | Shared QML: the notification wrapper, the support dialog, the Markdown bar, the edit history, the note body |
| `qml/cover/*` | The cover page and its glyph |
| `qml/modules/Opal/*` | Bundled Opal About, SupportMe and LinkHandler modules |

`src/main.cpp` publishes two context properties - `Lappu` (the session) and `Notes` - and the pages bind to those. It also adds `qml/modules` to the QML import path so the bundled Opal modules resolve at runtime. Networking stays in C++; QML holds no request and parses no server response.

## One writer

`Session` is the only thing that writes a note. `saveNote()`, `setFavorite()` and `deleteNote()` each write to `NotesCache` straight away, marking the row `dirty` - or `deleted` - and only then offer it to the server.

`NotesStore.qml` owns no notes of its own. It is filled through `ingest()` from `Notes.notes()` and hands every edit to `Lappu`, so the list is always a view of what the cache holds rather than a second copy that could drift from it.

## Sync rules

`Session::sync()` **sends what is owed before it pulls**, so a note edited on the phone is the one that stands rather than being overwritten by the older copy coming back. `pushNext()` drains `m_pending` one note at a time, choosing the verb from the row itself: a `deleted` row is a `DELETE`, a row whose id is not a server id yet is a `POST`, anything else is a `PUT`.

Conflicts are **last write wins**. No `If-Match` is sent, so the server does not reject a note that changed on both sides. The ETag the server sends is kept with the note so that a conditional write can be added later, which is the only way to notice such a conflict; until then nothing is decided by it, and in particular it does not say whether the server knows the note.

Two paths lead into the push, and they differ in how loudly they fail:

* `sync()` sets `m_announce` and `m_pullAfter`, so it reports a server it could not reach and pulls afterwards.
* `pushDirty()`, which runs whenever credentials are applied, clears both. An owed note is offered as soon as it is made, but a flush the user did not ask for stays quiet and leaves the note waiting.

`queueDrained()` decides whether to pull: either the run asked for one, or something was actually pushed and **Refresh after saving a note** is on. A push that sent nothing therefore cannot trigger a pull, which is what stops a note the server keeps refusing from driving an endless loop.

## Automatic refresh

`applySyncTimer()` holds a `QTimer` that only ticks while the app runs; nothing in Sailfish OS wakes Lappu for a refresh. The timer is restarted at the end of `onNotesReady()` and `onApiFailed()`, so the interval is measured from the last run rather than from when it was set.

The startup refresh is behind `syncIfStartupWanted()` and its `m_syncedOnStart` latch, because `applyCredentials()` runs again whenever credentials are re-applied and would otherwise pull more than once. A fresh sign-in is the exception: `onLoggedIn()` sets the latch and syncs regardless of the setting, since a new account has nothing to show until it does.

## A note has no title

The server names a note after the first line of its content. `noteTitle()` in `session.cpp` works the same name out locally, so a page can show it before the next pull confirms it.

A note that has not reached the server borrows a **negative id** until the server names it. That id is what the cover may be holding, so `onCreated()` remaps it: if `coverNoteId` is the local id being replaced, it is rewritten to the id the server gave. Without that, pinning a brand-new note would leave the cover blank after its first sync.

Such an id is an ordinary id everywhere else, and treating it as a stand-in for "no note" is a mistake worth guarding against. Only `0` means no note: `saveNote()` starts a new note for `0` alone, so that editing an unsent one finds its row instead of forging another, and the cover counts any non-zero id as a pin.

## Messages to the user

A passing message has one route: `app.showToast()` in `qml/harbour-lappu.qml` hands it to the single `LappuNotification`, which publishes a transient notice. Anything the session reports through `errorMessage` arrives that way, as does a note that is no longer there. No page carries its own copy of `errorMessage`.

A failed sign-in is the exception: `loginFailed` reaches the sign-in pages, which say so in place, because the reason belongs to the form the user is looking at. That is why `onLoginFailed()` in `session.cpp` does not also set `errorMessage`. A sign-in that simply runs out of time takes the same route: `LoginFlow::onDeadline()` cancels the poll and fails with a reason.

## Links

No link is handed to the browser by the app. `Opal.LinkHandler` shows it first, which matters most for the links inside a note, since those came from the server rather than from the app. `NoteBody` only re-emits `linkActivated`, so the page that holds it decides, and both the page and the component's own example call `LinkHandler.openOrCopyUrl()`.

## Sandbox layout

Sailjail gives the app three private directories, named after the `OrganizationName` and `ApplicationName` of the desktop file:

* `~/.config/org.frapps/harbour-lappu` - `settings.conf`
* `~/.local/share/org.frapps/harbour-lappu` - `notes.sqlite`
* `~/.cache/org.frapps/harbour-lappu` - whatever the web view leaves behind; nothing the app writes itself

The permissions are `Internet`, `Secrets` and `WebView`; no document or picture directory is asked for. The config and share directories are declared for the system backup through `X-HarbourBackup`.
