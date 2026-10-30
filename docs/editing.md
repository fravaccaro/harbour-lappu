---
layout: default
title: Writing a note
parent: Using Lappu
nav_order: 3
---

# Writing a note

**New note** in the pulley menu of the notes list opens an empty note; **Edit**, in the menu of a note or on the note itself, opens one that already exists. Either way the page is a dialog: the change is kept when you accept it with **Add** or **Save**, and dropped when you navigate back.

Above the text sit the category of the note and a **Favorite** switch, so neither needs a second visit.

## The title is the first line

A note carries no title field. The server names a note after its first line, and Lappu works out the same name on the phone, which is why the field is labelled *The first line is the title*. Write the title on the first line and the body under it.

Naming it locally means a new note is listed under its real name straight away, rather than waiting for the next refresh to learn what the server decided to call it.

## The Markdown bar

The bar above the text applies Markdown to the selection, or, with nothing selected, inserts the markers and leaves the cursor between them:

| Button | Writes |
|---|---|
| **B** | `**bold**` |
| *I* | `*italic*` |
| U | `<u>underline</u>` |
| S | `~~strikethrough~~` |
| List | `- ` at the start of the line |
| Task | `- [ ] ` at the start of the line |
| Link | `[label]()`, with the cursor left in the brackets |
| H1 - H6 | `#` to `######` at the start of the line |

A heading button **replaces** whatever heading the line already had rather than adding more hashes, so tapping H2 on an H1 line makes it an H2.

## Undo and redo

The two arrows at the foot of the text field undo (↩) and redo (↪). An arrow is dimmed when there is nothing in that direction.

Typing is gathered into steps rather than recorded letter by letter, so one undo takes back the last run of typing. Each tap on the Markdown bar is sealed as its own step, so formatting can be taken back without losing what was written before it.

## Reading a note

The note page draws the Markdown rather than showing the markers: headings are set in bold, emphasis and strikethrough as written, bullets as bullets, and a link as a link.

Tapping a link does not hand it straight to the browser. Because a note may have been written anywhere and synced in, the link is shown to you first, with the address in full and the choice to open it, copy it or share it.

A **task line** - `- [ ] something` - is drawn with a checkbox, and tapping the row ticks or unticks it there and then, without opening the editor. The whole row is the tap target, so a link inside a task line keeps its label but is not followed.

A note the server marks read-only says so and cannot be edited. An empty note says that too, rather than showing a blank page.

## Categories and favourites

**Category** opens a picker holding the categories the notes already use, with room to type a new one. A note created from inside a category starts in that category.

Nextcloud writes a nested category with slashes in it, and Lappu shows it the same way with the separator spelled out.

## Deleting

**Delete** is offered in the menu of a note and on the note itself. It asks for a moment first, so a mistaken tap can be called back before the note goes. Deleting the note that is pinned to the cover gives the cover back to the notes list.
