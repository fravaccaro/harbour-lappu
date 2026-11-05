---
layout: default
title: Writing a note
parent: Using Lappu
nav_order: 3
---

# Writing a note

**New note** in the pulley menu of the notes list opens an empty note; **Edit**, in the menu of a note or on the note itself, opens one that already exists. Either way the page is a dialog: the change is kept when you accept it with **Add** or **Save**, and dropped when you navigate back.

Above the text sit the star of the note, beside its title, and the category below, so neither needs a second visit.

## The title is the note's name

The **Title** field at the top of the page is the name of the note, which on the server is the name of its file. It is not part of the text: the body below holds the whole document, and the two are kept apart exactly as Nextcloud keeps them.

Changing the title **renames the note**. Nothing else does - editing the body, ticking a task or favouriting a note all leave the name alone - so a note you renamed in Nextcloud, or in the Files app, keeps that name when you edit it here.

Leave the title empty and the note is named after the first line of the body instead, with a leading `#` or list marker taken off the way Nextcloud does. That way a note jotted down in a hurry still has a name, and it is listed under it straight away rather than waiting for the next refresh to learn what the server called it.

If the first line of the body says exactly what the title says, it is not drawn twice: reading the note, and in the list, that line is left to the heading above. Notes written before Lappu had a title field, which usually repeat their name on the first line, therefore read as they always did.

## The Markdown bar

The bar above the text applies Markdown to the selection, or, with nothing selected, inserts the markers and leaves the cursor between them. The buttons stand on one line, and when they want more room than the screen has the bar can be dragged sideways; the rest of the page stays where it is.

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
| ¹ | a footnote: `[^1]` here, and `[^1]: ` at the foot of the note |

A heading button **replaces** whatever heading the line already had rather than adding more hashes, so tapping H2 on an H1 line makes it an H2.

A tap on the bar leaves the keyboard up, so several marks can be added one after another without it closing between them, and so does dragging the bar to reach a button further along it. A drag that starts on a button scrolls the bar rather than writing anything.

The table button is the one that writes more than markers. It lays down a header row, the row of dashes under it and one empty row, always on lines of its own, and leaves the first column name selected so typing renames it:

```
| Column | Column |
| --- | --- |
|  |  |
```

Add a row by writing another line of the same shape, and a column by adding a cell to every row, the dashes included. Writing `:---`, `---:` or `:---:` in the row of dashes puts that column's text left, right or centre.

The footnote button writes in two places at once, which no other button does: a reference where the cursor is, and the note it refers to at the foot of the text, with the cursor left after the colon ready for what it says. It numbers the reference with the lowest number the note has not used, so inserting one twice gives `[^1]` and `[^2]`.

## Footnotes

A footnote lets an aside sit out of the way of the sentence that needs it. Write a reference where the aside belongs, and the note itself on a line of its own:

```
The meeting is on Tuesday.[^1]

[^1]: Moved from Monday.
```

The reference can be named anything without spaces - `[^1]`, `[^bignote]`, `[^why]` - and the note can be written anywhere, not only at the end. Lines indented under a note belong to it.

Reading the note, a reference is drawn as a small raised **number** and the notes are gathered together under the body, below a line that separates them from it, whatever order they were written in. The numbers are counted in the order the references appear, so a note that refers to `[^5]` before `[^2]` still reads 1 then 2, and you never have to renumber anything by hand. A reference the note never defines is left as you typed it, since there is nothing to point at.

Tapping a raised number does nothing: it points inside the note you are already reading.

Nextcloud writes its comment threads using the same syntax, with names like `[^comment-1]`. Lappu has no separate comment view, so these are shown as ordinary footnotes - the text is all there, under the body with the rest.

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

In the notes list and on the cover, where there is no room for any of this, the markers are simply taken out: a table reads as its cells, a quote as its text, and a code block without its fences. A footnote reference goes with them, since there is no list of notes under an excerpt for a number to point at, while what the footnote says is kept with the rest of the text.

A note the server marks read-only says so and cannot be edited. An empty note says that too, rather than showing a blank page.

## Categories and favourites

**Category** opens a picker holding the categories the notes already use, with room to type a new one. A note created from inside a category starts in that category.

Nextcloud writes a nested category with slashes in it, and Lappu shows it the same way with the separator spelled out.

The **star** at the end of the Title field marks the note a favourite, the same star the notes list draws beside the name. Tapping it fills it in or empties it, and like everything else on the page it is only written when you accept the dialog; the menu of a note offers the same thing without opening the editor.

## Deleting

**Delete** is offered in the menu of a note and on the note itself. It asks for a moment first, so a mistaken tap can be called back before the note goes. Deleting the note that is pinned to the cover gives the cover back to the notes list.
