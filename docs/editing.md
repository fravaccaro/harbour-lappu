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
| ▦ | an empty two-column table |

A heading button **replaces** whatever heading the line already had rather than adding more hashes, so tapping H2 on an H1 line makes it an H2.

The table button is the one that writes more than markers. It lays down a header row, the row of dashes under it and one empty row, always on lines of its own, and leaves the first column name selected so typing renames it:

```
| Column | Column |
| --- | --- |
|  |  |
```

Add a row by writing another line of the same shape, and a column by adding a cell to every row, the dashes included. Writing `:---`, `---:` or `:---:` in the row of dashes puts that column's text left, right or centre.

## Undo and redo

The two arrows at the foot of the text field undo (↩) and redo (↪). An arrow is dimmed when there is nothing in that direction.

Typing is gathered into steps rather than recorded letter by letter, so one undo takes back the last run of typing. Each tap on the Markdown bar is sealed as its own step, so formatting can be taken back without losing what was written before it.

## Reading a note

The note page draws the Markdown rather than showing the markers: emphasis and strikethrough as written, bullets as bullets, and a link as a link.

A **heading** is set in its own size as well as in bold, so `#` is larger than `##`, and `###` and below read as bold at the size of the text around them.

A **quote** - a line opening with `>` - is set in the dimmer text colour with a bar down its left side instead of the `>`. Consecutive quoted lines share one unbroken bar, and a quote written `>>` is indented a step further.

**Code** is left exactly as it was typed. A span between backquotes, `` `like this` ``, is set in a fixed-pitch font, and nothing inside it is read as Markdown, so asterisks in a snippet stay asterisks. A block fenced by ``` lines is drawn on a faint panel in the same font, with its lines unwrapped: where a line of code ends is part of what it says, so a line wider than the screen is reached by dragging the panel sideways rather than by folding it.

A **nested** list item or task is indented the way it was written, two spaces to a step.

Tapping a link does not hand it straight to the browser. Because a note may have been written anywhere and synced in, the link is shown to you first, with the address in full and the choice to open it, copy it or share it.

A **task line** is drawn with a checkbox, and tapping the row ticks or unticks it there and then, without opening the editor. The whole row is the tap target, so a link inside a task line keeps its label but is not followed. What marks a task is the brackets, not the bullet in front of them: `- [ ] something`, `* [ ] something` and `+ [ ] something` are all tasks, and ticking one writes back the bullet it was written with. The same three characters all start a plain bullet.

A **table** is drawn as a grid with its header in bold, not as the pipes it was written with. What makes a table a table is the row of dashes directly under the header, which is why a sentence with a stray `|` in it is left as the sentence it is. Each column is as wide as its widest cell, and when the columns together want more room than the screen has, the grid can be dragged sideways; the rest of the note stays where it is. A link in a cell is a link, and is offered the same way as any other.

In the notes list and on the cover, where there is no room for any of this, the markers are simply taken out: a table reads as its cells, a quote as its text, and a code block without its fences.

A note the server marks read-only says so and cannot be edited. An empty note says that too, rather than showing a blank page.

## Categories and favourites

**Category** opens a picker holding the categories the notes already use, with room to type a new one. A note created from inside a category starts in that category.

Nextcloud writes a nested category with slashes in it, and Lappu shows it the same way with the separator spelled out.

## Deleting

**Delete** is offered in the menu of a note and on the note itself. It asks for a moment first, so a mistaken tap can be called back before the note goes. Deleting the note that is pinned to the cover gives the cover back to the notes list.
