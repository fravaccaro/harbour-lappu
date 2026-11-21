---
layout: default
title: Notes and views
parent: Using Lappu
nav_order: 2
---

# Notes and views

The first page is the notes themselves. Each row carries the title of the note, how long ago it was last changed, and the opening of the body; a favourite is marked with a star, and in the **All** view a note that belongs to a category says which one. Tapping a row opens the note; tapping and holding it opens a menu to favourite it, pin it to the cover, edit it or delete it.

A note's title is its **name on the server**, kept apart from the text and set in its own field when you write the note. Left empty, it is taken from the first line of the body, so a note is named the moment you write that line rather than after the next refresh. See [Writing a note](editing).

A row above the notes appears when a note changed on the phone and on the server at once, counting how many are waiting; tapping it opens the two versions side by side to choose between them. A note named **"... (my version)"** is the phone's side of such a choice, kept as a note of its own because **Keep both** was the answer; it keeps that name - the two notes would otherwise share one - until you change its first line. See [Sync, files and permissions](files).

## The pulley menu

Pull the list down for **About**, **Settings**, **Refresh**, **Search** and **New note**. New note sits at the handle, nearest the thumb.

## Views

Swipe left from the notes for **Views**, the ways the notes are narrowed down. Four kinds live there:

* **All** - every note
* **Favorites** - the notes marked with a star
* **Uncategorized** - the notes that belong to no category
* One entry per category the notes actually use, with the number of notes in it

Each entry carries its own count. The categories are read from the notes rather than kept separately, so a category exists for as long as a note names it and disappears when the last note leaves it. A category whose name has slashes in it - Nextcloud writes nested categories that way - is shown with the separator spelled out.

Choosing a view takes you back to the notes with the title at the top changed to match. Creating a note from inside a category gives the new note that category to start with. Tags make a fifth kind of view on a server that has them, and are listed under the categories on the same page.

**Trash** appears under Uncategorized on a server that keeps one, and is a place rather than a view: it opens a page of its own instead of narrowing the notes. See [The trash](#the-trash) below.

## Tags

A note belongs to one category but can carry any number of tags. Tags are a feature of the **QOwnNotes** app for Nextcloud rather than of the Notes app, so everything about them is there only on a server that carries that app and can read the tag database of the note folder. Everywhere else Lappu says nothing about tags at all.

They are listed on the same page, under the categories, one entry per tag with the number of notes carrying it. Choosing one takes you back to the notes showing only those, with the title at the top changed to match, exactly as a category does.

The tags are the note folder's own, the same ones QOwnNotes shows on the desktop and on Android, so a tag made in any of them is a tag here. A tag nested under another is written with slashes - `Work/Projects` - and is shown with the separator spelled out.

## The trash

A deleted note is not always gone: where Nextcloud keeps deleted files, the notes among them can be looked at from **Trash** on the views page. Like tags, this needs the **QOwnNotes** app, and it needs the **Deleted files** app on top of it; where either is missing there is no Trash entry at all.

The page lists the deleted notes with the date each went, and the folder it was deleted from where that was not the note folder itself. It is read when you open it and nothing of it is kept on the phone, so **Refresh** in the pulley menu is how you see what somebody has deleted or emptied since.

A deleted note cannot be opened - there is nothing to open it into until it is a note again - so tapping a row opens its menu. **Restore** puts it back where it was deleted from, with the tags it carried, and the notes are refreshed so that it comes home to the list. Emptying the trash is Nextcloud's own business and is not done from here.

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

With nothing pinned, the cover shows the current view and the notes in it. The two cover actions write a new note and refresh; neither does anything until an account has been signed in. A **New note** tapped while the app is still starting is kept until the notes are up, rather than being dropped.
