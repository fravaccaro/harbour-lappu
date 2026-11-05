# Agent Guidelines & Engineering Standards

This document defines architectural boundaries, coding conventions, and workflow rules for AI agents and contributors working on this Qt/QML codebase. All changes must strictly adhere to these guidelines.

---

## 1. Single Responsibility & Function Granularity

Write small, deterministic functions that perform exactly one task.

- **One purpose per unit:** A function must either transform data, query state, or trigger an action—never all three simultaneously.
- **Function size:** Keep C++ functions under 25 lines and QML/JS functions under 15 lines. If a function requires branching across multiple logical stages, decompose it into private helpers.
- **C++ Purity:** Prefer pure free functions (in anonymous namespaces or dedicated utility namespaces) or `const` member methods over state-mutating routines. Avoid hidden side effects.
- **Declarative first in QML:** Do not use JavaScript functions in QML for behavior that can be expressed declaratively with state bindings, `Binding`, or property expressions.

---

## 2. Reuse & Dependency Discipline

Never re-implement existing logic. Search the repository before adding new helpers or algorithms.

- **Check core utilities first:** Review existing utility modules, shared helpers, and base models before writing new logic.
- **Leverage Qt APIs:** Prefer standard Qt primitives (`QDir`, `QUrl`, `QRegularExpression`, `QtConcurrent`) and standard library algorithms (`<algorithm>`) over custom parsing or filtering loops.
- **DRY enforcement:** If identical logic appears in two places, extract it into a shared utility class/function or a reusable QML component immediately.
- **Minimal coupling:** New helpers must accept the narrowest required type (e.g., pass `const QUrl&` rather than entire configuration objects).

---

## 3. Code Style & Conventions

Follow official Qt and QML style guides consistently across the entire project.

### C++ Standards
- **Naming:**
 - Classes and Structs: `PascalCase`
 - Functions and Methods: `camelCase`
 - Member variables: `m_camelCase`
 - Constants and Enums: `PascalCase` or `kCamelCase`
- **Modern C++:** Target C++11, matching the Sailfish OS Qt 5.6 toolchain. Prefer `auto` only when the concrete type is explicitly repeated on the right side of the assignment. Do not use `QStringView`.
- **Memory & Object Trees:**
 - Strictly use `QObject` parent-child hierarchy for ownership where applicable.
 - Use smart pointers (`std::unique_ptr`, `QPointer`, `QSharedPointer`) when `QObject` ownership is insufficient. Never leave raw owning pointers.
- **Signals & Slots:** Use the modern compile-time syntax exclusively (`connect(sender, &Sender::signal, receiver, &Receiver::slot)`). String-based `SIGNAL()` and `SLOT()` macros are forbidden.

### QML Standards
- **Property Order:** Maintain uniform declaration order within every QML element:
 1. `id`
 2. Property declarations (`property int count`)
 3. Signal declarations
 4. JavaScript functions
 5. Object properties (`x`, `y`, `width`, `height`)
 6. Visual layout anchors (`anchors.*`)
 7. Appearance properties (`color`, `opacity`, `visible`)
 8. Signal handlers (`onClicked`, `onTextChanged`)
 9. Child items / Components
 10. States and transitions
- **Property Bindings:** Avoid breaking declarative bindings with imperative JavaScript assignments (e.g., avoid `width = parent.width` inside an imperative handler).
- **Handlers:** Avoid inline heavy JavaScript inside event handlers (`onClicked: { ... }`). Delegate non-trivial logic to dedicated C++ backend slots or well-scoped helper functions.

---

## 4. QML Component Library Development & Maintenance

Shared UI elements must live in the designated design system / component library module.

- **Atomic Design:** Keep reusable UI controls decoupled from application domain state and network models.
- **Strict Encapsulation:**
 - Never reference outer parent IDs or global singleton models directly inside library components.
 - Expose external behavior exclusively via explicit `property`, `signal`, and alias declarations.
- **Documentation:** Every reusable component must contain a brief QDoc/header comment explaining its properties, signals, and an example usage snippet.
- **Preview & Isolation:** Ensure shared components can instantiate independently inside a dummy test harness or `qmlscene` without crashing due to missing backend dependencies.

---

## 5. Living Documentation & Continuous Alignment

This document is a living contract. Keeping it accurate is part of the implementation definition of done.

- **Synchronized updates:** When refactoring directory layouts, changing project dependencies, deprecating utilities, or establishing new UI patterns, update this file in the exact same commit or pull request.
- **User-facing documentation:** A change the user can see belongs in `docs/` in the same commit. `docs/guide.md` and its children describe the app as it behaves, so a new setting, page or gesture is not finished until the page that covers it says so.
- **Agent Verification Step:** Before concluding any task, verify:
 1. Did I introduce any duplicate logic?
 2. Are all new QML components properly isolated and placed in the component library?
 3. Does the implementation violate any convention stated in this file?
 4. Does any page under `docs/` now describe behaviour that changed?

---

## 6. Project layout

| Path | Purpose |
|---|---|
| `src/lappu/config.*` | Server URL, user name, TLS preference, sort, view and the pinned note |
| `src/lappu/secretsstore.*` | App password in a device-lock Sailfish Secrets collection |
| `src/lappu/loginflow.*` | Nextcloud login flow v2 |
| `src/lappu/httpclient.*` | `QNetworkAccessManager` wrapper |
| `src/lappu/url.*` | Server URL normalisation |
| `src/lappu/session.*` | QML-facing controller |
| `src/lappu/notesapi.*` | Nextcloud Notes API v1 client |
| `src/lappu/notescache.*` | SQLite cache of the last pull |
| `src/lappu/notesmodel.*` | Holds the notes of the last pull; `notes()` hands them to QML as plain rows. Not a list model, despite the name: no view binds to it |
| `src/lappu/types.h` | Plain data structs (`LocalNote`) |
| `qml/pages/*` | Silica pages, including the notes the server is holding shut and how to unlock them (`LockedNotesPage.qml`) |
| `qml/notes/*` | Notes view model (`NotesStore.qml`) and markdown helpers (`NoteFormat.js`) |
| `qml/components/*` | Shared QML: the notification wrapper, the support dialog, the markdown bar, the edit history, the note body, a markdown table (`MarkdownTable.qml`), a code block (`MarkdownCode.qml`), one side of a conflict (`ConflictVersion.qml`), and the three pieces both notes layouts share — the pulley menu (`NotesPullDownMenu.qml`), the header (`NotesListHeader.qml`, which carries the conflict and locked rows) and a note's own menu (`NoteContextMenu.qml`) |
| `qml/cover/*` | Cover page |
| `qml/modules/Opal/*` | Bundled Opal About, SupportMe, and LinkHandler modules |
| `docs/*` | The documentation site, built by Jekyll from `_config.yml`; `README.md` is its home page |
| `landing/*` | The marketing page, kept out of the Jekyll build and mirrored in the portfolio site |

Networking stays in C++. Pages bind to `Lappu` and `Notes`.

Syncing notes is implemented in both directions. `Session` is the only writer: `saveNote`, `setFavorite` and `deleteNote` write to `NotesCache` straight away, marking the row `dirty` (or `deleted`), then offer it to the server. `Session::sync()` sends what is owed first and pulls afterwards, so a note edited on the phone is the one that stands. A note written while offline keeps its `dirty` row, survives a restart, and is sent again by the next sync; a flush the user did not ask for stays quiet about an unreachable server.

A `PUT` carries the stored ETag as `If-Match`, in the double quotes the server compares against, while the JSON hands the same tag out bare; an empty tag sends no header, so an older server is still written to unconditionally. A `POST` has nothing to match and a `DELETE` is not checked, so neither sends it. The `412` that answers a stale write carries the server's whole note, which `NotesApi::conflicted()` reports apart from `failed()`: the server is well, so `Session::onConflict()` ends in `pushNext()` and leaves `m_pushed` alone, and the refusal earns the run a pull. It settles what it can by itself and otherwise records the pair in `m_conflicts` — in memory only, since the local side is the `dirty` row the next `412` would ask about again — tells the user through `notice()`, and is skipped by `pushNext()` until answered. Every resolver settles the original row, because one that left it `dirty` with a stale tag would fork another copy on each sync; `resolveKeepBoth()` writes its fork through `NotesCache::upsertLocal()` rather than `Session::store()`, which would re-enter the push, and labels only the title, never the content. The cover keeps its pin on the original id.

A link is never opened straight from the app. Every one goes through `Opal.LinkHandler`, including the links inside a note, which arrive from the server and so are shown to the user before anything is followed.

`LoginFlow` bounds its polling with `m_deadline`, because the poll endpoint answers 404 both while a token waits for approval and after it has expired. The deadline matches the twenty minutes Nextcloud keeps a login flow v2 token, so the attempt fails with a reason instead of polling for as long as the page is open.

Nothing bounds a request but `HttpClient::watchForStall()`. Qt 5.6 has no transfer timeout, and `Session` clears `m_busy` only when a reply arrives, so a request that hung would leave the app busy for good and every later `sync()` would return at its own `m_busy` guard. The watchdog is a `QTimer` parented to the reply, restarted by each progress signal, so it cuts off only a transfer that has stopped moving and leaves a slow one alone. Nothing else in the app aborts a reply, which is why `OperationCanceledError` is reported as the server having stopped answering.

A write refused by a status other than `412` leaves on `refused()`, not `failed()`: the server answered, so `Session::onRefused()` calls `pushNext()`, costing that note its turn rather than the rest of the queue. The row stays `dirty` and goes out with the next run. `423` is the one worth knowing: the Notes app answers it for a locked file, after retrying five times itself, while read-only is `403`. The lock is often `ManuallyLockedException` from the `files_lock` app — one a user placed, which has no expiry and is held until it is lifted on the server — so the message names the lock rather than calling the note busy, and the row simply waits. **Nothing the user wrote is ever dropped by a refusal**: a file nobody can write is also a file nobody can change, so an edit kept waiting hides no version of the note but the one it was made from, and the first run after the lock lifts sends it.

The refusal is **always** reported, and through `notice()` rather than `errorMessage`: the edit was the user's own, so a tick that did not land is worth knowing about whether or not the sync was asked for, and the `if (error == m_error) return;` guard would otherwise hold back the second of two identical refusals — which is exactly what ticking a box twice on a locked note earns. A silent push was the whole of the original bug: the note carried a change the server had refused and said nothing about it.

The refusal **names the note**, which is why `refused()` carries the status rather than only a sentence: `NotesApi` knows neither which note was sent nor what it is called, while `Session` holds it in `m_inFlight`, so `refusalNotice()` builds the wording there and `statusError()` has no `423` branch left — a `GET` of the list is never answered with one. A message is read once, so `403` and `423` — the two that hold a note shut rather than going wrong the once, by `heldShut()` — also go into `m_locked`, the id paired with the status, behind `lockedCount` and `lockedNotes()`. It is kept nowhere else for the reason `m_conflicts` is not: the row is still `dirty`, so the next run earns the same refusal and makes the list again. One rule empties it, in `pruneLocked()` off the back of `reloadModels()`: a row that owes the server nothing is a row nothing is holding back, which covers the write that landed, the change discarded and the note deleted alike. `logout()` clears it by hand, the cache being emptied without a reload. `LockedNotesPage` names them and says where a lock is lifted; a row in `NotesListHeader` is the only way in, as with conflicts, since a page arriving over the user's work during a refresh would be an ambush.

`NotesModel` hands QML the note's own permission as `readOnly`, named as the field is and kept apart from `Lappu.lockedCount`, which counts notes whose *change* the server would not take. One is a property of the note, the other of what is owed on it.

`Session::discardLocal()` is the way out when the user would rather have the note back than go on waiting — a change made offline, one thought better of, or one a lock is holding indefinitely. It drops any question recorded for the note, clears `dirty` and `deleted` through `NotesCache::abandonOwed()` without settling the row against any version, and pulls, so the server's own note is written over it; with a run in flight `m_pullAfter` carries that instead. It refuses a note the server has never seen, which has no version to fall back on, and a row that owes nothing. The pages offer it as **Discard waiting change**, shown only where `dirty` reaches QML through `NotesModel`, and `NotePage` says on the note itself that a change has not reached the server — a note that looks ordinary while carrying one is what made a refusal feel like a fault here.

A pull reconsiders an owed row too: `applyRemote()` asks `stillOwed()` rather than reading `dirty`, so a row the server already agrees with on title, content, category and favourite is a write that landed and whose answer never arrived, and is settled by taking the server's note instead of earning a pointless `412` on the next run. A `deleted` row is never settled that way.

A refused password is not an ordinary error. `NotesApi::reportStatus()` splits `401` off onto `unauthorized()`, and `Session::onUnauthorized()` stops the timer and drops the password so the app returns to the sign-in page. It clears nothing else: the cache and its `dirty` rows stay, so a fresh sign-in sends them. Unlike every other API error it is reported whether or not the user asked for the sync, because being sent back to sign in needs a reason.

A note's title is the file name and is kept apart from the content, which holds the whole document. The Notes v1 API does not derive the title from the content (the deprecated v0.2 `createAutoTitle`/`updateAutoTitle` did), so the client owns it: `bodyFor()` sends it with every `POST` and `PUT`, the server sanitises it and hands it back for the cache to adopt. `bodyFor()` sends the `modified` stamp too: the server bumps the file's own time on a content change but not on a title- or category-only edit, so leaving it out would have such an edit keep its old time and drop back down a list sorted by it; `Session::saveNote()` stamps the edit with the current time and the server adopts it. The title is the writer's to set, through its own field in `NoteEditPage`, and `Session::chosenTitle()` saves what was typed verbatim — so **nothing renames a note but the writer**. `Session::noteTitle()` survives only as the fallback for an empty field, naming the note after its first line so quick capture still names itself. Every other writer hands the stored title back untouched: `NotesStore.toggleTask()` passes `note.title`, or ticking a box would rename the note, and `setFavorite()` reuses the stored row. A name set on the server therefore outlives an edit made here.

Because the content is the whole document, the first line is ordinary text and is drawn like any other, including a first line that repeats the title: `previewBlocks()` for the note page, `excerpt()` for the rows and cards and `plainText()` for the cover all begin at the top, so every row carries its true `lineIndex` and a task still flips the right box. The name drawn above the body by `NotePage` and the cover is the file's, kept apart from the text, so a note that opens by repeating its name shows that line in both places.

An unsynced new note borrows a negative id until the server names it. That id identifies the note like any other, and only `0` stands for no note at all: `Session::saveNote()` starts a new note for `0` alone, and what decides between `POST` and `PUT` is whether the id is a server id, never whether an ETag was stored.

A rendered row is not a line of the note. `NoteFormat.previewBlocks()` hands `NoteBody` one row per line except for a table or a fenced code block, which are one row standing for all the lines they were written on, so every row carries the `lineIndex` it began at and `taskClicked` reports that rather than the row. `NotesStore.flipLine()` indexes the content by line, and a row number would tick the wrong box under any block. A table is told from prose by the row of dashes directly beneath its header, and `MarkdownTable` reads the markdown again where it is drawn, so the row carries the block itself and nothing has to agree on a model of cells.

A footnote is the one mark whose meaning is not local, so `previewBlocks()` reads the note twice: `footnotesOf()` collects the `[^id]: ...` definitions and `footnoteOrder()` numbers them by the order the body first refers to them, the number rather than the label being what is drawn. A definition draws nothing where it was written but its lines are still counted by `pushRow()`, so every later row keeps its `lineIndex` and a task still flips the right box; the notes are appended as `kind: "footnote"` rows carrying `-1`. The numbering travels as an `order` parameter through `lineToHtml()`, `richText()` and `markedUp()` rather than as shared state, so `MarkdownTable`, which parses its cells in a pass of its own, leaves references alone - a footnote may not appear in a table anyway. A reference is drawn as a `footnote:` link that `NoteBody.followLink()` swallows, so it never reaches `Opal.LinkHandler`. Nextcloud's `[^comment-1]` threads are ordinary footnotes here.

`blockAt()` asks about a fence before a table, because inside a fence a row of pipes is code and a hash is a hash; `MarkdownCode` then draws the block as `Text.PlainText`, which is what keeps it unread. Inside a line the same rule holds one level down: `richText()` splits the backquoted spans out before any other mark is read, or the asterisks of a snippet would be taken for emphasis. What marks a task is the brackets and not the bullet — `-`, `*` and `+` all open one, as they do in the server's own renderer — and `NotesStore.flipTaskLine()` rewrites only the brackets, so the bullet a note was written with survives a tick. A heading carries its `level` and a nested item its `indent` rather than arriving pre-marked, since size, weight and offset are the drawing's business.

`qml/notes/NotesStore.qml` is the QML-side view of the synced notes: it owns the filtering, search, sort, category and cover state, and is filled through `ingest()` from `Notes.notes()`. It writes nothing itself; every edit is handed to `Lappu`. The two pieces of that state the user expects to outlast a restart — the sort order and the pinned note — are bindings onto `Lappu.sortMode` and `Lappu.coverNoteId`, which `Config` keeps in `settings.conf`, so neither may be assigned imperatively.

`visibleNotes`, the model every view binds to, is **edited in place** by `syncRows()` and never cleared and refilled. A clear destroys every delegate, and a delegate is where a Silica remorse lives, so redrawing the list after one delete would throw away a delete the user had already started on another note.

A run takes its queue when it starts. `pushDirty()` therefore cannot send a note marked while one is in flight; it sets `m_owedWhileBusy`, and `flushOwed()` sends it at the end of `queueDrained()` and `onNotesReady()`. Only a user's edit sets that flag, so a row the server keeps refusing still waits for the next sync rather than spinning a run on every drain.

The answer to a write is taken against the note that was sent, kept in `m_inFlight`, never against the row as it now stands: `saveNote()` writes to the cache while a request is travelling, so `settleWritten()` settles the row only where it still matches what went out and otherwise calls `keepOwedOver()`, which takes the server's id and ETag but leaves the row `dirty`. Settling blindly would lose the newer edit and clear the very mark that was to send it — the one way a note is dropped with nothing having gone wrong.

A `PUT` answered `404` reaches `onDeleted()` alongside a `DELETE` that succeeded, and the row tells them apart: one marked `deleted` is the delete the user asked for, one still `dirty` is a note taken away under an edit nobody has seen. `restartOwed()` keeps that edit as a note the server has never seen — fresh temporary id, no ETag, appended to `m_pending` so it leaves in the same run — with its title unchanged, since there is no second copy to tell it from, and says so through `notice()`.

How the notes are laid out is not part of that state, because nothing but the drawing depends on it: `NotesPage` reads `Lappu.viewMode` itself and loads either a `SilicaListView` of rows or a `SilicaGridView` of cards. They are two kinds of view rather than one drawn two ways — a `GridView` gives every cell the same height, which the rows do not have — so only the chosen one is built, and the three things they would otherwise each hold a copy of live in `qml/components/` instead. A cell measures itself off `Screen.width` rather than a column count, so two fit across the narrow side of any screen and a wider one gets more of them rather than wider ones.

Sailjail private directories follow `OrganizationName` / `ApplicationName`:

- `~/.config/org.frapps/harbour-lappu`
- `~/.local/share/org.frapps/harbour-lappu`
- `~/.cache/org.frapps/harbour-lappu`
