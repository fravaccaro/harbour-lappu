---
layout: default
title: Notes and categories
parent: Using Lappu
nav_order: 2
---

# Notes and categories

The first page is the notes themselves. Each row carries the title of the note, how long ago it was last changed, and the opening of the body; a favourite is marked with a star, and in the **All** view a note that belongs to a category says which one. Tapping a row opens the note; tapping and holding it opens a menu to favourite it, pin it to the cover, edit it or delete it.

A note has **no title of its own**. The server names it after its first line, and Lappu works out the same name on the phone, so a new note is named the moment you write that line rather than after the next refresh. See [Writing a note](editing).

A row above the notes appears when a note changed on the phone and on the server at once, counting how many are waiting; tapping it opens the two versions side by side to choose between them. A note named **"... (my version)"** is the phone's side of such a choice, kept as a note of its own because **Keep both** was the answer; the name lasts until the next refresh, when the server names it after its first line like any other note. See [Sync, files and permissions](files).

## The pulley menu

Pull the list down for **About**, **Settings**, **Refresh**, **Search** and **New note**. New note sits at the handle, nearest the thumb.

## Categories

Swipe left from the notes for the categories. Four kinds of view live there:

* **All** - every note
* **Favorites** - the notes marked with a star
* **Uncategorized** - the notes that belong to no category
* One entry per category the notes actually use, with the number of notes in it

Each entry carries its own count. The categories are read from the notes rather than kept separately, so a category exists for as long as a note names it and disappears when the last note leaves it. A category whose name has slashes in it - Nextcloud writes nested categories that way - is shown with the separator spelled out.

Choosing a view takes you back to the notes with the title at the top changed to match. Creating a note from inside a category gives the new note that category to start with.

## Searching

**Search** in the pulley menu reveals the search field; choosing it again hides it. Showing the field puts the cursor straight in it, and hiding it clears whatever was typed, so the notes come back in full.

The search runs on the phone against the copy it already has, so it answers as you type and works with no server in reach. It matches the title and the body of the note, not the category.

## List or grid

**View** on the settings page draws the notes either as a **List** of full-width rows or as a **Grid** of cards. The grid fits two cards across a phone held upright and more as the screen gets wider, so turning it on its side gives you more cards rather than wider ones.

A card carries the same four things the row does - the title with its star, how long ago the note changed, the opening of the body and the category - stacked rather than laid out across, with the body given whatever room is left in the card. Tapping and holding a card opens the same menu as a row.

Everything else is the same in both: the pulley menu, the search field, the chosen category, the sort order and the row that counts unsettled notes. Changing the setting starts the notes again from the top.

## Sorting

**Sort** on the settings page orders the notes by **Last modified** or by **Title**. Favourites are kept at the top either way, sorted among themselves by the same rule.

## The cover

**Pin to cover**, in the menu of a note, puts that note on the app cover: its title and body, with the Markdown taken out so it reads as plain text. Only one note sits there, so pinning another replaces it, and **Unpin from cover** gives the cover back to the notes list. The pin is remembered, so the note is still on the cover after the app is restarted, and pinning a note that has not reached the server yet still works - the pin follows it once the server has given it an id.

With nothing pinned, the cover shows the current view and the notes in it. The two cover actions write a new note and refresh; neither does anything until an account has been signed in.
