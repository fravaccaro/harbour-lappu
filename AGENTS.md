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
| `src/lappu/config.*` | Server URL, user name, TLS preference |
| `src/lappu/secretsstore.*` | App password in a device-lock Sailfish Secrets collection |
| `src/lappu/loginflow.*` | Nextcloud login flow v2 |
| `src/lappu/httpclient.*` | `QNetworkAccessManager` wrapper |
| `src/lappu/url.*` | URL path helpers |
| `src/lappu/session.*` | QML-facing controller |
| `src/lappu/notesapi.*` | Nextcloud Notes API v1 client |
| `src/lappu/notescache.*` | SQLite cache of the last pull |
| `src/lappu/notesmodel.*` | Notes list model |
| `src/lappu/types.h` | Plain data structs (`LocalNote`) |
| `qml/pages/*` | Silica pages |
| `qml/notes/*` | Notes view model (`NotesStore.qml`) and markdown helpers (`NoteFormat.js`) |
| `qml/components/*` | Shared QML such as the notification wrapper |
| `qml/cover/*` | Cover page |
| `qml/modules/Opal/*` | Bundled Opal About, SupportMe, and LinkHandler modules |
| `docs/*` | The documentation site, built by Jekyll from `_config.yml`; `README.md` is its home page |
| `landing/*` | The marketing page, kept out of the Jekyll build and mirrored in the portfolio site |

Networking stays in C++. Pages bind to `Lappu` and `Notes`.

Syncing notes is implemented in both directions. `Session` is the only writer: `saveNote`, `setFavorite` and `deleteNote` write to `NotesCache` straight away, marking the row `dirty` (or `deleted`), then offer it to the server. `Session::sync()` sends what is owed first and pulls afterwards, so a note edited on the phone is the one that stands. A note written while offline keeps its `dirty` row, survives a restart, and is sent again by the next sync; a flush the user did not ask for stays quiet about an unreachable server. Conflicts are last-write-wins: no `If-Match` is sent.

A note has no title of its own. The server names it after the first line of the content, and `Session` works the same name out locally so the pages can show it before the next pull confirms it. An unsynced new note borrows a negative id until the server names it.

`qml/notes/NotesStore.qml` is the QML-side view of the synced notes: it owns the filtering, search, sort, category and cover state, and is filled through `ingest()` from `Notes.notes()`. It writes nothing itself; every edit is handed to `Lappu`.

Sailjail private directories follow `OrganizationName` / `ApplicationName`:

- `~/.config/org.frapps/harbour-lappu`
- `~/.local/share/org.frapps/harbour-lappu`
- `~/.cache/org.frapps/harbour-lappu`
